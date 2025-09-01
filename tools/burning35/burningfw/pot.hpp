#pragma once

#include "fraise.h"
#include "hardware/adc.h"
#include <cmath>
#include <algorithm>

class Pot {
private:
	float value = 0;		// 0.0 - 1.0
	float speed = 0.1;		// 0.0 - 1.0
	int adc_pin;
	absolute_time_t next_update_time;

public:
	Pot(int adc_pin): adc_pin(adc_pin) {
		adc_init();
		// Make sure GPIO is high-impedance, no pullups etc
		adc_gpio_init(adc_pin);
		// Select ADC input 0 (GPIO26)
		adc_select_input(adc_pin - 26);
	}

	bool update() {
		if(!time_reached(next_update_time)) return false;
		next_update_time = make_timeout_time_ms(1);
		adc_select_input(adc_pin - 26);
		int v = adc_read();
		value += (v - value) * speed;
		return true;
	}

	void set_speed(float s) {
		speed = std::clamp(s, 0.0f, 1.0f);
	}
	
	float get_value() {
		return value / 4096.0;
	}
};

