#pragma once

#include "fraise.h"
#include "hardware/timer.h"
#include "hardware/sync.h"
#include <cmath>
#include <algorithm>

class StepMotor {
private:
	int64_t position = 0;
	int64_t destination = 0;
	int64_t delay_us = 10000;
	int dir_pin;
	int step_pin;
	float fps = 0.0;
	float fps_init = 0.0;
	int distance_init = 1;
	int steps_per_frame;
	float phase_to_speed = 0.0; // 0.0 - 1.0
	absolute_time_t next_update_time;
	enum {SPEED, POS} mode = SPEED;
	const float FPS_MIN = 0.05;
	bool on_frame = false;

	static int64_t callback(alarm_id_t id, void *user_data) {
		auto motor = static_cast<StepMotor*>(user_data);
		return motor->step();
	}
	int64_t step() {
		int incr = 0;
		on_frame = (destination == position);
		switch(mode) {
		case SPEED:
			if(fps == 0.0 || destination == position) incr = 0;
			else if(fps > 0.0) incr = 1;
			else if(fps < 0.0) incr = -1;
			break;
		case POS:
			//if(position == destination) return 10000;
			if(destination > position) incr = 1;
			else if(destination < position) incr = -1;
			//delay_us = 100000 / (1 + abs(position - destination));
			break;
		}
		if(incr == 0) return 10000;
		if(incr > 0) {
			gpio_put(dir_pin, 1);
			position++;
		} else {
			gpio_put(dir_pin, 0);
			position--;
		}
		gpio_put(step_pin, !gpio_get(step_pin));
		if(delay_us < 25) delay_us = 25;
		return delay_us;
	}
	void set_fps_private(float f) {
		fps = f;
		if(fps != 0.0) delay_us = 1000000 / (fabs(fps) * steps_per_frame);
		else delay_us = 10000;
	}
public:
	StepMotor(int dir_pin, int step_pin, int steps_per_frame): 
			dir_pin(dir_pin), step_pin(step_pin), steps_per_frame(steps_per_frame) {
		gpio_init(dir_pin);
		gpio_set_dir(dir_pin, GPIO_OUT);
		gpio_init(step_pin);
		gpio_set_dir(step_pin, GPIO_OUT);
		add_alarm_in_us(delay_us, callback, this, true);
	}

	float get_phase() {
		return (position % steps_per_frame) / (float)steps_per_frame;
	}

	float get_norm_position() {
		return position / (float)steps_per_frame;
	}

	float get_distance_to_frame() {
		return 2.0 * fabs(get_phase() - 0.5);
	}
	void update() {
		if(!time_reached(next_update_time)) return;
		next_update_time = make_timeout_time_ms(10);
		switch(mode) {
		case SPEED:
			set_fps_private(fps_init * (get_distance_to_frame() * phase_to_speed + 1 - MAX(phase_to_speed, 0)));
			break;
		case POS:
			int distance = abs(position - destination);
			if(distance == 0) return;
			float next_fps = (fps_init * distance) / distance_init;
			if(fabs(next_fps) < FPS_MIN) next_fps = next_fps > 0 ? FPS_MIN : -FPS_MIN;
			set_fps_private(next_fps);
			//delay_us = 100000 / (1 + abs(position - destination));
			break;
		}
	}
	void stop_next_frame() {
		uint32_t status = save_and_disable_interrupts();
		int64_t next_destination;
		if(fps > 0) next_destination = (position / steps_per_frame + 1) * steps_per_frame;
		else next_destination = (position / steps_per_frame) * steps_per_frame;
		destination = next_destination;
		restore_interrupts(status);
	}
	void freerun() {
		uint32_t status = save_and_disable_interrupts();
		if(fps >= 0) destination = position - 1;
		else destination = position + 1;
		restore_interrupts(status);
	}
	void set_fps(float f) {
		//uint32_t status = save_and_disable_interrupts();
		//set_fps_private(f);
		fps_init = f;
		mode = SPEED;
		//restore_interrupts(status);
	}
	void set_position(int64_t p = 0) {
		uint32_t status = save_and_disable_interrupts();
		position = p;
		set_fps(0);
		restore_interrupts(status);
	}
	void reset_position(int64_t p = 0) {
		uint32_t status = save_and_disable_interrupts();
		position = p;
		set_fps(0);
		restore_interrupts(status);
	}
	void set_phase_to_speed(float pts) {
		phase_to_speed = std::clamp(pts, -1.0f, 1.0f);
	}
	float get_fps() {
		return fps * (destination != position);
	}
	int get_onframe() {
		/*if (destination != position) return 0;
		else return 1;*/
		return on_frame;
		//return 2;
	}
};

