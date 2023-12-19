/**
 * Copyright (c) 2023 metalu.net
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/async_context_threadsafe_background.h"

#include "hardware/pio.h"
#include "fraise.pio.h"
#include "fraise.h"
#include "fraise_buffers.h"

// This program
// - Uses a PIO state machine to receive data
// - Use an interrupt to determine when the PIO FIFO has some data
// - Saves characters in a queue
// - Uses an async context to perform work when notified by the irq

//#define SERIAL_BAUD 250000
/*#define PIO_RX_PIN 0
#define PIO_TX_PIN 1
#define PIO_DRV_PIN 2*/
#define FIFO_SIZE 64
//#define MAX_COUNTER 10

static PIO pio;
static uint sm;
static int8_t pio_irq;
static queue_t fifo;
static uint offset;
static uint32_t counter;
static bool work_done;
static uint irq_index;

static uint8_t FraiseID = 10;

static void async_worker_func(async_context_t *async_context, async_when_pending_worker_t *worker);

// An async context is notified by the irq to "do some work"
static async_context_threadsafe_background_t async_context;
static async_when_pending_worker_t worker = { .do_work = async_worker_func };

typedef enum {
    FS_OFF,
    FS_LISTEN,
    FS_POLL,
    FS_RECEIVE,
    FS_SEND
} fraise_state_t;

static fraise_state_t state = FS_LISTEN;
static uint8_t rx_checksum;
static uint8_t rx_bytes;
static uint8_t rx_msg_length;

// IRQ called when the pio rx fifo is not empty, i.e. there are some characters on the uart
// This needs to run as quickly as possible or else you will lose characters (in particular don't printf!)
static void pio_irq_func(void) {
    while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
        uint16_t c = fraise_program_getc(pio, sm);
        /*if (!queue_try_add(&fifo, &c)) panic("fifo full");
        async_context_set_work_pending(&async_context.core, &worker);
        return;*/
        
        if(c > 255) state = FS_LISTEN; // bit 9 signals the start of a new message
        switch(state) {
            case FS_LISTEN:
                if((c == FraiseID + 0x100)  // if we are the destination of the message
                    || (c == 0x100)) {      // or if the message is "broadcast" (for everyone)
                    state = FS_RECEIVE;
                    rx_bytes = 1;
                    rx_checksum = c;
                    if (!queue_try_add(&fifo, &c)) panic("fifo full");
                } else if(c == FraiseID + 0x180) { // poll signal
                    state = FS_POLL;
                }
                break;
            case FS_POLL:
                if(c == FraiseID + 0x80) {
                    // send next packet, or '0' if none.
                    // state = FS_SEND;
                    // break;
                    uint8_t len = txbuf_read_init();
                    if(len == 0) {
                        fraise_program_start_tx(pio, sm, 1);
                        fraise_program_putc(pio, sm, 0);
                    } else {
                        fraise_program_start_tx(pio, sm, len);
                        //fraise_program_putc(pio, sm, len--);
                        while(len--) fraise_program_putc(pio, sm, txbuf_read_getc());
                        txbuf_read_finish();
                    }
                }
                state = FS_LISTEN;
                break;
            case FS_RECEIVE:
                rx_checksum += c;
                rx_bytes++;
                if(rx_bytes == 2) rx_msg_length = c & 31;
                else if(rx_bytes == rx_msg_length + 3) { // end of message
                    if(rx_checksum == 0) { // good checksum
                        // send 0 (ack)
                        // validate message
                        // Tell the async worker that there are some characters waiting for us:
                        /*uint16_t cksm = rx_checksum;
                        if (!queue_try_add(&fifo, &cksm)) panic("fifo full");*/
                        async_context_set_work_pending(&async_context.core, &worker);
                        fraise_program_start_tx(pio, sm, 1);
                        fraise_program_putc(pio, sm, 0);
                    } else {
                        // send 1 (nack)
                    }
                    state = FS_LISTEN;
                    break;
                }
                if (!queue_try_add(&fifo, &c)) panic("fifo full");
                break;
            case FS_SEND:
                state = FS_LISTEN;
                break;
        }
    }
}

// Process characters
static void async_worker_func(async_context_t *async_context, async_when_pending_worker_t *worker) {
    work_done = true;
    while(!queue_is_empty(&fifo)) {
        uint16_t c;
        if (!queue_try_remove(&fifo, &c)) {
            panic("fifo empty");
        }
        //putchar(c); // Display character in the console
        printf("%#05lx\n", c);
    }
}

// Find a free pio and state machine and load the program into it.
// Returns false if this fails
static bool init_pio(const pio_program_t *program, PIO *pio_hw, uint *sm, uint *offset) {
    // Find a free pio
    *pio_hw = pio1;
    if (!pio_can_add_program(*pio_hw, program)) {
        *pio_hw = pio0;
        if (!pio_can_add_program(*pio_hw, program)) {
            *offset = -1;
            return false;
        }
    }
    *offset = pio_add_program(*pio_hw, program);
    // Find a state machine
    *sm = (int8_t)pio_claim_unused_sm(*pio_hw, false);
    if (*sm < 0) {
        return false;
    }
    return true;
}

int fraise_setup(uint rxpin, uint txpin, uint drvpin) {
    /*gpio_init(PIO_TX_PIN);
    gpio_init(PIO_DRV_PIN);
    gpio_set_dir(PIO_TX_PIN, GPIO_OUT);
    gpio_set_dir(PIO_DRV_PIN, GPIO_OUT);
    gpio_put(PIO_TX_PIN, 1);
    gpio_put(PIO_DRV_PIN, 1);*/

    // create a queue so the irq can save the data somewhere
    queue_init(&fifo, 2, FIFO_SIZE);

    // Setup an async context and worker to perform work when needed
    if (!async_context_threadsafe_background_init_with_defaults(&async_context)) {
        panic("failed to setup context");
    }
    async_context_add_when_pending_worker(&async_context.core, &worker);

    // Set up the state machine we're going to use to receive them.
    // In real code you need to find a free pio and state machine in case pio resources are used elsewhere
    if (!init_pio(&fraise_program, &pio, &sm, &offset)) {
        panic("failed to setup pio");
    }
    fraise_program_init(pio, sm, offset, rxpin, txpin, drvpin);

    // Find a free irq
    static_assert(PIO0_IRQ_1 == PIO0_IRQ_0 + 1 && PIO1_IRQ_1 == PIO1_IRQ_0 + 1, "");
    pio_irq = (pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0;
    if (irq_get_exclusive_handler(pio_irq)) {
        pio_irq++;
        if (irq_get_exclusive_handler(pio_irq)) {
            panic("All IRQs are in use");
        }
    }

    // Enable interrupt
    irq_add_shared_handler(pio_irq, pio_irq_func, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
    irq_set_enabled(pio_irq, true); // Enable the IRQ
    irq_index = pio_irq - ((pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0); // Get index of the IRQ
    pio_set_irqn_source_enabled(pio, irq_index, pis_sm0_rx_fifo_not_empty + sm, true); // Set pio to tell us when the FIFO is NOT empty
}

    /*// Echo characters received from PIO to the console
    while (counter < MAX_COUNTER || work_done) {
        // Note that we could just sleep here as we're using "threadsafe_background" that uses a low priority interrupt
        // But if we changed to use a "polling" context that wouldn't work. The following works for both types of context.
        // When using "threadsafe_background" the poll does nothing. This loop is just preventing main from exiting!
        work_done = false;
        async_context_poll(&async_context.core);
        async_context_wait_for_work_ms(&async_context.core, 2000);
    }*/

void fraise_unsetup() {
    // Disable interrupt
    pio_set_irqn_source_enabled(pio, irq_index, pis_sm0_rx_fifo_not_empty + sm, false);
    irq_set_enabled(pio_irq, false);
    irq_remove_handler(pio_irq, pio_irq_func);

    // Cleanup pio
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program(pio, &fraise_program, offset);
    pio_sm_unclaim(pio, sm);

    async_context_remove_when_pending_worker(&async_context.core, &worker);
    async_context_deinit(&async_context.core);
    queue_free(&fifo);
}

bool fraise_puts(char* msg){
    if (!txbuf_write_init()) {
        printf("tx buffer full!\n");
        return false;
    }
    //txbuf_write_putc(strlen(msg) + 128); // signal 'char' message
    char c;
    while(c = *msg++) txbuf_write_putc(c);
    txbuf_write_finish(true);
    return true;
}

bool fraise_putbytes(char* data, uint8_t len){
    if (!txbuf_write_init()) {
        printf("tx buffer full!\n");
        return false;
    }
    while(len--) txbuf_write_putc(*data++);
    txbuf_write_finish(false);
    return true;
}

void fraise_debug_print_next_txmessage(){
    int len = txbuf_read_init();
    if(!len) {
        printf("no pending message\n");
        return;
    }
    while(len--) printf("%02x ", txbuf_read_getc());
    putchar('\n');
    txbuf_read_finish();
}
