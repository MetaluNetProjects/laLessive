/**
 * Simple blinking fruit plus 1 button
 */

#define BOARD pico
#include "fraise.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
int ledPeriod = 250;

const uint BUT_PIN = 0;
bool button_on = false;

void setup() {
	gpio_init(BUT_PIN);
	gpio_set_dir(BUT_PIN, GPIO_IN);
	gpio_pull_up(BUT_PIN);
}

void button_update() {
	static absolute_time_t nextTime;
	static int count = 0;
	const int MAXCOUNT = 10; // 200ms
	if(!time_reached(nextTime)) return;
	nextTime = make_timeout_time_ms(10);
	if(gpio_get(BUT_PIN) == 0) {
		if(count < MAXCOUNT) count++;
		else button_on = true;
	} else {
		if(count > -MAXCOUNT) count--;
		else button_on = false;
	}
}

void loop(){
	static absolute_time_t nextLed;
	static bool led = false;

	button_update();

	if(time_reached(nextLed)) {
		gpio_put(LED_PIN, led = !led);
		nextLed = make_timeout_time_ms(ledPeriod);
	}
}

void fraise_receivebytes(const char *data, uint8_t len){
	uint8_t command = fraise_get_uint8();
	switch(command) {
		case 1: ledPeriod = (int)fraise_get_uint8() * 10; break;
		case 2: printf("b %d\n", button_on); break;
		default: ;
	}
}

void fraise_receivechars(const char *data, uint8_t len){
	if(data[0] == 'E') { // Echo
		printf("E%s\n", data + 1);
	}
}

