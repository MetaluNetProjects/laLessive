/**
 * Blink example copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Modifications copyright (c) 2021 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include <stdio.h>
#include <string.h>

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

void reboot() {
	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
	watchdog_reboot(0, 0, 0);

	while (1) {
		tight_loop_contents();
	}
}

uint8_t lineBuf[256];
uint8_t lineLen;

#define startsWith(str, prefix) (!(strncmp((const char *)(str), (const char *)(prefix), strlen(prefix))))
void processSysLine() {
	switch(lineBuf[1]) {
		case 'R': printf("sID01\n"); break;
		case 'V': printf("sV UsbFraise PicoPied v0.1\n"); break;
		case 'E': puts((const char*)(lineBuf + 2)); break;
	}
}

const char __in_flash("mygroup") flashEater[8192] = "skdjhksqjdkjqskdjhscx,nb,ncbqhskfdjhkjsqjeufhkjqzskjcsn,wxmlkqskdjldsjkfkj";

void processLine() {
	if(lineBuf[0] == '#') processSysLine();
	else if(startsWith(lineBuf, "waitack")) printf("ack\n");
	else if(startsWith(lineBuf, "reboot")) {
		sleep_ms(50); // wait for the host to disconnect the USB device
		reboot();
	}
	else if(startsWith(lineBuf, "readflash")) {
		uint32_t addr;
		sscanf((const char *)lineBuf, "readflash %ld", &addr);
		printf("readflash at %#08lx:", addr);
		for(int i = 0; i < 16; i++) {
			printf("%02X", *(const uint8_t *) (XIP_BASE + addr + i));
		}
		printf("\n");
	}
	else if(startsWith(lineBuf, "getsizeof")) {
		printf("sizeof int=%d long=%d float=%d double=%d\n", sizeof(int), sizeof(long), sizeof(float), sizeof(double));
	}
	else if(startsWith(lineBuf, "zorg")) {
		printf("%s\n", flashEater);
	}
}

void stdioTask(void* unused)
{
	int c;
	static bool led;
	//unused; // don't warn
	while((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT){
		gpio_put(LED_PIN, led = !led);
		if(c == '\n') {
			lineBuf[lineLen] = 0;
			processLine();
			lineLen = 0;
		}
		else lineBuf[lineLen++] = c;
	}
}

void loop();

int main() {
	//int flashes = 0;
	stdio_init_all();
	//while(!stdio_usb_connected()); // wait until USB connection
	//stdio_set_chars_available_callback(stdioTask, NULL); // doesn't seem to work!

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	/*while (flashes < 5) {
		gpio_put(LED_PIN, 1);
		sleep_ms(50);
		gpio_put(LED_PIN, 0);
		sleep_ms(50);
		flashes++;
	}
	
	sleep_ms(5000);*/

	while(true) {
		stdioTask(NULL);
		loop();
	}
	reboot();
}

void loop(){
	static absolute_time_t next;// = make_timeout_time_ms(100);
	static bool led = false;
	if(absolute_time_min(next, get_absolute_time()) == next) {
		gpio_put(LED_PIN, led = !led);
		next = make_timeout_time_ms(100);
	}
}


