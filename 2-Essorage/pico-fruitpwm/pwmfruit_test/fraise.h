/**
 * Copyright (c) 2023 metalu.net
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

int fraise_setup(uint rxpin, uint txpin, uint drvpin);
void fraise_unsetup();
bool fraise_puts(char* msg); // returns true on success
bool fraise_putbytes(char* data, uint8_t len); // returns true on success
void fraise_debug_print_next_txmessage();

