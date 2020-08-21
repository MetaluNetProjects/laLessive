/*********************************************************************
 *               analog example for Versa1.0
 *	Analog capture on connectors K1, K2, K3 and K5. 
 *********************************************************************/

#define BOARD 8X2A

#include <fruit.h>
#include <switch.h>
#include <dcmotor.h>
#include <dmx.h>

t_delay mainDelay;
int loops = 0;
DCMOTOR_DECLARE(C);
DCMOTOR_DECLARE(D);

void setup(void) {	
//----------- Setup ----------------
	fruitInit();
			
	//pinModeDigitalOut(LED); 	// set the LED pin mode to digital out
	//digitalClear(LED);		// clear the LED
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms

//----------- Switch module setup ----------
	switchInit();		// init analog module
	switchSelect(0, MOTC_ZERO);
	switchSelect(1, MOTD_ZERO);
	switchSelect(2, START_SWITCH);

//----------- dcmotor setup ----------------
    dcmotorInit(C);
    dcmotorInit(D);
    
//----------- init DMX master module -------
    DMXInit();
}

void loop() {
// ---------- Main loop ------------
	fraiseService();	// listen to Fraise events
	switchService();	// analog management routine
	DMXService();		// DXM management routine

	if(delayFinished(mainDelay)) // when mainDelay triggers :
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay
		switchSend();		// send switch channels that changed
		DCMOTOR_COMPUTE(C,ASYM);
		DCMOTOR_COMPUTE(D,ASYM);
		loops++;
		if(loops == 200) { // 1 sec
			byte chan;
			loops = 0;
			for(chan = 0; chan < 3; chan++) {
				printf("Cs %d %d\n", chan, switchGet(chan) == 0);
			}
		}
	}
}

// Receiving

void fraiseReceiveChar() // receive text
{
	unsigned char c;
	
	c=fraiseGetChar();
	if(c=='L'){		//switch LED on/off 
		c=fraiseGetChar();
		//digitalWrite(LED, c!='0');		
	}
	else if(c=='E') { 	// echo text (send it back to host)
		printf("C");
		c = fraiseGetLen(); 			// get length of current packet
		while(c--) printf("%c",fraiseGetChar());// send each received byte
		putchar('\n');				// end of line
	}	
}

void fraiseReceive() // receive raw
{
	unsigned char c;
	unsigned int i;
	c=fraiseGetChar();
	
	switch(c) {
		PARAM_INT(30,i); DMXSet(i, fraiseGetChar()); break; // if first byte is 30 then get DMX channel (int) and value (char)
	    case 120 : DCMOTOR_INPUT(C) ; break;
	    case 121 : DCMOTOR_INPUT(D) ; break;
	}
}

