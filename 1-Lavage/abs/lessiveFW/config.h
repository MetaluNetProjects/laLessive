#ifndef _CONFIG_H_
#define _CONFIG_H_

#define ANALOG_FILTER 3 

#define MOTC_END KZ1
#define MOTC_A KZ1
#define MOTC_B KZ1
#define MOTC_ZERO K7

#define MOTD_END KZ1
#define MOTD_A KZ1
#define MOTD_B KZ1
#define MOTD_ZERO K6

#define COIL K8
#define START_SWITCH K9

//DMX config:
#define DMX_UART_NUM 	AUXSERIAL_NUM
#define DMX_UART_PIN	AUXSERIAL_TX  // K5 for 8X2A
#define DMX_NBCHAN 	256

#endif // _CONFIG_H_

