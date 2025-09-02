#pragma once

#include "fraise.h"
#include "hardware/pwm.h"
#include <cmath>
#include <algorithm>

class Lamp {
private:
	float value = 0;		// 0.0 - 1.0
	float destination = 0;	// 0.0 - 1.0
	float speed = 0.1;		// 0.0 - 1.0
	int pwm_pin;
	float master = 1.0;
	absolute_time_t next_update_time;

public:
	Lamp(int pwm_pin): pwm_pin(pwm_pin) {
		gpio_set_function(pwm_pin, GPIO_FUNC_PWM);
		uint slice_num = pwm_gpio_to_slice_num(pwm_pin);
		pwm_set_wrap(slice_num, 65535);
		pwm_set_enabled(slice_num, true);
		pwm_set_gpio_level(pwm_pin, 0);
	}

	bool update() {
		if(!time_reached(next_update_time)) return false;
		next_update_time = make_timeout_time_ms(10);
		value += (destination - value) * speed;
		float pwm = std::clamp(master * value, 0.0f, 1.0f);
		pwm_set_gpio_level(pwm_pin, (1.0 - pwm) * 65535.0);
		return true;
	}

	void set_speed(float s) {
		speed = std::clamp(s, 0.0f, 1.0f);
	}
	void set_destination(float d) {
		destination = std::clamp(d, 0.0f, 1.0f);
	}
	float get_value() {
		return value;
	}
	void set_master(float m) {
		master = m;
	}
};

