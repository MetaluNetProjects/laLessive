/**
 * Copyright (c) 2023 metalu.net
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "fraise_buffers.h"

#define TXBUF_SIZE 512
#define RXBUF_SIZE 512

static char txbuf[TXBUF_SIZE];
static int txbuf_write_head = 1;
static int txbuf_write_tmphead;
static int txbuf_write_len;
static int txbuf_write_checksum;

static int txbuf_read_head = 0;
static int txbuf_read_tmphead;
static int txbuf_read_len = 0;

//#define txbuf_inc_head(h) do {h++; if (h == TXBUF_SIZE) h = 0;} while(0)
static inline int txbuf_inc_head(int h) {
    h++;
    if(h == TXBUF_SIZE) h = 0;
    return h;
}
    
bool txbuf_write_init(){          // Init a new message; returns false if txbuf is full.
    int freespace = txbuf_read_head - txbuf_write_head;
    if(freespace < 0) freespace += TXBUF_SIZE;
    if(freespace < 34) return false;
    txbuf_write_tmphead = txbuf_inc_head(txbuf_write_head); // keep first byte for length byte
    txbuf_write_len = 0;
    txbuf_write_checksum = 0;
    return true;
}

void txbuf_write_putc(char c){    // Add byte to the message
    txbuf[txbuf_write_tmphead] = c;
    txbuf_write_tmphead = txbuf_inc_head(txbuf_write_tmphead);
    txbuf_write_len++;
    txbuf_write_checksum += c;
}

void txbuf_write_finish(bool isChar){ // Finish the message
    if(isChar) txbuf_write_len |= 128;
    txbuf[txbuf_write_head] = txbuf_write_len; // write length byte
    txbuf_write_checksum += txbuf_write_len;
    txbuf[txbuf_write_tmphead] = -txbuf_write_checksum; // write checksum byte
    txbuf_write_head = txbuf_inc_head(txbuf_write_tmphead);
}

//void txbuf_write_cancel();        // Abort current message

uint8_t txbuf_read_init(){           // Initialize the sender for next message. Returns the length of the next message (0 if none).
    int usedspace = txbuf_write_head - txbuf_read_head - 1;
    if(usedspace < 0) usedspace += TXBUF_SIZE;
    if(usedspace == 0) return 0;
    txbuf_read_tmphead = txbuf_inc_head(txbuf_read_head);
    txbuf_read_len = (txbuf[txbuf_read_tmphead] & 31) + 2; // total_len = data_len + 1(length byte) + 1(checksum byte)
    //txbuf_read_tmphead = txbuf_inc_head(txbuf_read_tmphead);
    return txbuf_read_len;
}

char txbuf_read_getc(){           // Get next byte to send
    if(txbuf_read_len == 0) return 0;
    char c = txbuf[txbuf_read_tmphead];
    txbuf_read_len--;
    if(txbuf_read_len) txbuf_read_tmphead = txbuf_inc_head(txbuf_read_tmphead);
    return c;
}

void txbuf_read_finish(){         // Signal that the message has been sent successfully
    txbuf_read_head = txbuf_read_tmphead;
}

//void txbuf_read_cancel();         // Abort sending the current message; will retry next time.


