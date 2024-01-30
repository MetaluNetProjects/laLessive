/*********************************************************************
 *               Test Fraise on Versa1.0 
 *********************************************************************/

#define BOARD Versa2

#include <fruit.h>
#include <eeparams.h>
#include <ramp.h>

#define PERIOD_MAX 30000
#define PERIOD_MIN 30
unsigned char t = 0, t2 = 0;
t_delay mainDelay;

volatile int move;
long int wanted_pos = 0;
long int old_wanted_pos = 0;
unsigned int period = PERIOD_MAX;

t_ramp ramp;

int Speed;

#define SET_PERIOD(p) __critical { period = p;}

// Timer macros
#define TIMER 5
#include <timer.h>

#define INTPIN 2
#include <intpin.h>


#define TIMER_INIT() do{\
	TIMER_CON = 0; \
	TIMER_PS0 = 1;  			/* 	prescaler 8 (->2MHz at 64MHz)	*/\
	TIMER_PS1 = 1;  			/* 	...								*/\
	TIMER_16BIT = 1;			/* 	16bits							*/\
	TIMER_IP = 1			;	/* 	high/low priority interrupt 	*/\
	TIMER_ON = 0;				/* 	stop timer						*/\
	TIMER_IF = 0;   			/*  clear flag						*/\
	TIMER_IE = 1;				/* 	enable timer interrupt			*/\
} while(0)

#define	TimerCountUS(T) (0xffff - ((T * FOSC) / 32000000)) //us ; 

//#define InitTimerUS(T) do{TIMER_H = TimerCountUS(T) / 256; TIMER_L = TimerCountUS(T) % 256; TIMER_IF = 0;} while(0)
#define InitTimerUS(T) do{TIMER_H = (0xffff - T * 2) / 256; TIMER_L = (0xffff - T * 2) % 256; TIMER_IF = 0;} while(0)

void setup(void) {		
//----------- Setup ----------------
	fruitInit();
			
	pinModeDigitalOut(LED); 	// set the LED pin mode to digital out
	digitalClear(LED);		// clear the LED
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms

	digitalSet(STEP_CCW);
	digitalSet(STEP_CW);
	digitalSet(STEP_RST);
	digitalSet(STEP_F_H);
	digitalSet(STEP_ALLOFF);
	digitalSet(STEP_ECO);

	pinModeDigitalOut(STEP_CCW);
	pinModeDigitalOut(STEP_CW);
	pinModeDigitalOut(STEP_RST);
	pinModeDigitalOut(STEP_F_H);
	pinModeDigitalOut(STEP_ALLOFF);
	pinModeDigitalOut(STEP_ECO);

	pinModeDigitalIn(SW_ZERO); // = INT2 pin
	//INTPIN_IE = 1;
	INTPIN_IP = 1;
	INTPIN_EDGE = 0;

	TIMER_INIT();
	//SET_PERIOD(100);
	TIMER_ON = 1;
	rampInit(&ramp);
	EEreadMain();
}

long int distance;
int now_move;
unsigned int tmp_period;
t_time isrCost;

int move_on_intpin;
char intpin_happened = 0;

int move_add(int error)
__critical{
	move += error;
	return move;
}

void loop() {
// ---------- Main loop ------------
	fraiseService();	// listen to Fraise events

	if(delayFinished(mainDelay)) // when mainDelay triggers :
	{
		delayStart(mainDelay, 10000); 	// re-init mainDelay
		if(Speed) rampMove(&ramp, Speed);

		if(intpin_happened) {
			intpin_happened = 0;
			printf("Ci %ld %d\n", wanted_pos - move_on_intpin, move_on_intpin);
		}

		rampCompute(&ramp);
		old_wanted_pos = wanted_pos;
		wanted_pos = rampGetPos(&ramp);
		distance = wanted_pos - old_wanted_pos;
		if(ramp.length) {
			distance = (distance + ramp.length + ramp.length / 2) % ramp.length - ramp.length / 2;
		}

		now_move = move_add((int)distance);
		//printf("Cp %ld %ld %ld %d %d\n", wanted_pos, old_wanted_pos, distance, now_move/*isrCost*/ /*rampGetPos(&ramp)*/, period);
		printf("Cp %ld %d\n", wanted_pos, digitalRead(SW_ZERO) == 0);

		if(now_move != 0) {
			now_move = abs(now_move);
			tmp_period = PERIOD_MAX / now_move;
			if(tmp_period < PERIOD_MIN) tmp_period = PERIOD_MIN;
			SET_PERIOD(tmp_period);
		}
	}
}

void highInterrupts(void)
{
	static t_time cost;
	if(TIMER_IF) {
		InitTimerUS(period);
		if(move < 0) 
		{
			//digitalClear(DIR);
			//Nop();
			digitalClear(STEP_CCW);
			move++;
			digitalSet(STEP_CCW);
		}
		else if(move > 0)
		{
			//digitalSet(DIR);
			//Nop();
			digitalClear(STEP_CW);
			move--;
			digitalSet(STEP_CW);
		}
		/*cost = elapsed(timeISR());
		if(cost < 10000 && cost > isrCost) isrCost = cost;*/
	}
	else if(INTPIN_IE && INTPIN_IF) {
		INTPIN_IF = 0;
		INTPIN_IE = 0;
		intpin_happened = 1;
		move_on_intpin = move;
	}
}

// Receiving

void fraiseReceiveChar() // receive text
{
	unsigned char c;
	
	c=fraiseGetChar();
	if(c=='L'){		//switch LED on/off 
		c=fraiseGetChar();
		digitalWrite(LED, c!='0');		
	}
	else if(c=='E') { 	// echo text (send it back to host)
		printf("C");
		c = fraiseGetLen(); 			// get length of current packet
		while(c--) printf("%c",fraiseGetChar());// send each received byte
		putchar('\n');				// end of line
	}	
}

void testfunc() {
	long int a = 800010;
	unsigned long int b = 300000;
	long int c = (long)(a % b);
	printf("Cc = %ld\n", c);
}

void fraiseReceive() // receive raw bytes
{
	unsigned char c;
	long int l;
	//unsigned int i;
	
	c=fraiseGetChar();	// get the first byte

	switch(c) {
		PARAM_LONG(1, l); __critical{move = 0; old_wanted_pos = wanted_pos = l;}; SET_PERIOD(PERIOD_MAX); rampSetPos(&ramp, l); break;
		PARAM_INT(2, Speed); break;
		//PARAM_INT(3, i); SET_PERIOD(i); break;
		

		PARAM_CHAR(10, c); digitalWrite(STEP_RST, c != 0); break;
		PARAM_CHAR(11, c); digitalWrite(STEP_F_H, c != 0); break;
		PARAM_CHAR(12, c); digitalWrite(STEP_ALLOFF, c != 0); break;
		PARAM_CHAR(13, c); digitalWrite(STEP_ECO, c != 0); break;

		case 50: rampInput(&ramp); break;
		case 100: isrCost = 0; break;
		case 101: testfunc(); break;
		case 102: 
			INTPIN_IF = 0;
			INTPIN_IE = 1;
			intpin_happened = 0;
			break;
		case 255: if(fraiseGetChar() == 255) EEwriteMain(); break;
	}
}

void EEdeclareMain()
{
	rampDeclareEE(&ramp);
}
