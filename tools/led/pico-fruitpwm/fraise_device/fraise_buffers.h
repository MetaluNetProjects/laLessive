/**
 * Copyright (c) 2023 metalu.net
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

bool    txbuf_write_init();                 // Init a new message; returns false if txbuf is full.
void    txbuf_write_putc(char c);           // Add byte to the message
void    txbuf_write_finish(bool isChar);    // Finish the message

uint8_t txbuf_read_init();                  // Initialize the sender for next message. Returns the length of the next message (0 if none).
char    txbuf_read_getc();                  // Get next byte to send
void    txbuf_read_finish();                // Signal that the message has been sent successfully

