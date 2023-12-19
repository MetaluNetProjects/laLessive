/**
 * Copyright (c) 2023 metalu.net
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FRAISE_DEVICE_H
#define _FRAISE_DEVICE_H

int fraise_setup(uint rxpin, uint txpin, uint drvpin);
void fraise_setID(uint8_t id);
void fraise_unsetup();

bool fraise_puts(char* msg); // returns true on success
bool fraise_putbytes(char* data, uint8_t len); // returns true on success
void fraise_debug_print_next_txmessage(); // print next pending message to send, and remove it from the list

// optional user defined callbacks:
void fraise_receivebytes(char *data, uint8_t len);
void fraise_receivechars(char *data, uint8_t len);
void fraise_receivebytes_broadcast(char *data, uint8_t len);
void fraise_receivechars_broadcast(char *data, uint8_t len);

#endif // _FRAISE_DEVICE_H
