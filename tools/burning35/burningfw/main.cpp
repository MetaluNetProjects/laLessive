/**
 * Simple blinking fruit
 */

#define BOARD pico
#include "fraise.h"
#include "stepmotor.hpp"
#include "lamp.hpp"
//#include "pot.hpp"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
int ledPeriod = 250;

const uint MOTOR_DIR_PIN = 0;
const uint MOTOR_STEP_PIN = 1;
const uint MOTOR_ENABLE_PIN = 2;
StepMotor motor(MOTOR_DIR_PIN, MOTOR_STEP_PIN, 400 * 32);

Lamp lamp(8);
//Pot pot(26);
float lamp_val = 0.0; // 0.0 - 1.0
float lamp_motor_influence = 0.0;

void setup() {
	gpio_init(MOTOR_ENABLE_PIN);
	gpio_set_dir(MOTOR_ENABLE_PIN, GPIO_OUT);
	gpio_put(MOTOR_ENABLE_PIN, 0);
}

void loop(){
	static absolute_time_t nextLed;
	static absolute_time_t nextInfo;
	static bool led = false;

	motor.update();
	//pot.update();
	if(lamp.update()) {
		//float val = pot.get_value();
		float val = lamp_val * (1.0 + fabs(motor.get_fps()) * lamp_motor_influence);
		lamp.set_destination(val);
	}
	if(time_reached(nextLed)) {
		gpio_put(LED_PIN, led = !led);
		nextLed = make_timeout_time_ms(ledPeriod);
		//printf("pot %f lamp %f\n", pot.get_value(), lamp.get_value());
	}
	if(time_reached(nextInfo)) {
		nextInfo = make_timeout_time_ms(10);
		fraise_put_init();
		uint8_t address = 1;
		if(motor.get_onframe()) address = 2;
		fraise_put_uint8(address);
		fraise_put_uint16(motor.get_fps() * 10000);
		fraise_put_uint16(lamp.get_value() * 10000);
		fraise_put_int32(motor.get_norm_position() * 100);
		fraise_put_send();
	}
}

void fraise_receivebytes(const char *data, uint8_t len){
	uint8_t command = fraise_get_uint8();
	switch(command) {
		case 1: ledPeriod = (int)fraise_get_uint8() * 10; break;

		case 2: motor.set_fps(fraise_get_int16() / 10000.0); break;
		case 3: motor.stop_next_frame(); break;
		case 4: motor.reset_position(fraise_get_int32()); break;
		case 5: gpio_put(MOTOR_ENABLE_PIN, fraise_get_uint8() == 0); break;
		case 6: motor.set_phase_to_speed(fraise_get_int16() / 10000.0); break;
		case 7: motor.freerun(); break;

		case 50: lamp_val = fraise_get_int16() / 10000.0; break;
		case 51: lamp.set_speed(fraise_get_int16() / 10000.0); break;
		case 52: lamp_motor_influence = fraise_get_int16() / 1000.0; break;
		case 53: lamp.set_master(fraise_get_uint16() / 10000.0); break;
		//case 10: motor.set_destination(fraise_get_int32()); break;
		//case 11: motor.set_delay_us(fraise_get_int32()); break;
		default: ;
	}
}

void fraise_receivechars(const char *data, uint8_t len){
	if(data[0] == 'E') { // Echo
		printf("E%s\n", data + 1);
	}
}

