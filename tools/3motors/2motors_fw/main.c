/*********************************************************************
 *				firmware for motor X
 *********************************************************************/

#define BOARD 8X2A

#include <fruit.h>
#include <analog.h>
#include <dcmotor.h>
#include <ramp.h>
#include <eeparams.h>

t_delay mainDelay;

DCMOTOR_DECLARE(C);
DCMOTOR_DECLARE(D);
//-------------  Timer1 macros :  ---------------------------------------- 
//prescaler=PS fTMR1=FOSC/(4*PS) nbCycles=0xffff-TMR1init T=nbCycles/fTMR1=(0xffff-TMR1init)*4PS/FOSC
//TMR1init=0xffff-(T*FOSC/4PS) ; max=65536*4PS/FOSC : 
//ex: PS=8 : T=0.01s : TMR1init=0xffff-15000
//Maximum 1s !!
#define	TMR1init(T) (0xffff-((T*FOSC)/32000)) //ms ; maximum: 8MHz:262ms 48MHz:43ms 64MHz:32ms
#define	TMR1initUS(T) (0xffff-((T*FOSC)/32000000)) //us ; 
#define InitTimer(T) do{ TMR1H=TMR1init(T)/256 ; TMR1L=TMR1init(T)%256; PIR1bits.TMR1IF=0; }while(0)
#define InitTimerUS(T) do{ TMR1H=TMR1initUS(T)/256 ; TMR1L=TMR1initUS(T)%256; PIR1bits.TMR1IF=0; }while(0)
#define TimerOut() (PIR1bits.TMR1IF)

void highInterrupts()
{
	if(PIR1bits.TMR1IF) {
		DCMOTOR_CAPTURE_SERVICE(C);
		DCMOTOR_CAPTURE_SERVICE(D);
		InitTimerUS(50UL);
	}
}

void setup(void) {
//----------- Setup ----------------
	fruitInit();

	//pinModeDigitalOut(LED); 	// set the LED pin mode to digital out
	//digitalClear(LED);		// clear the LED
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms

//----------- Analog setup ----------------
	analogInit();		// init analog module
	//analogSelect(0, MOTC_CURRENT);	// assign MotorC current sense to analog channel 0

//----------- dcmotor setup ----------------
	dcmotorInit(C);
	dcmotorInit(D);

	//DCMOTOR(C).Setting.onlyPositive = 1;
	DCMOTOR(C).Setting.PosWindow = 15;
	DCMOTOR(C).Setting.PwmMin = 150;
	DCMOTOR(C).Setting.PosErrorGain = 5;

	DCMOTOR(D).Setting.PosWindow = 15;
	DCMOTOR(D).Setting.PwmMin = 150;
	DCMOTOR(D).Setting.PosErrorGain = 5;

//#define HW_PARAMS
#ifdef HW_PARAMS
	DCMOTOR(C).PosRamp.maxSpeed = 1800;
	DCMOTOR(C).PosRamp.maxAccel = 2400;
	DCMOTOR(C).PosRamp.maxDecel = 2400;
	//rampSetPos(&DCMOTOR(C).PosRamp, 0);

	DCMOTOR(C).PosPID.GainP = 40;
	DCMOTOR(C).PosPID.GainI = 1;
	DCMOTOR(C).PosPID.GainD = 0;
	DCMOTOR(C).PosPID.MaxOut = 1023;

	//DCMOTOR(C).VolVars.homed = 0;
#else
	EEreadMain();
#endif
	T1CON=0b00110011;//src=fosc/4,ps=8,16bit r/w,on.
	PIE1bits.TMR1IE=1;  //1;
	IPR1bits.TMR1IP=1;
}

void sendMotorState()
{
	static unsigned char buf[20] = {'B'};
	static int ramppos;
	static unsigned len;
	static char mot = 0;

	len = 1; // 'B'
	if(mot == 0) {
		buf[len++] = 10;
		buf[len++] = DCMOTOR_GETPOS(C) >> 8;
		buf[len++] = DCMOTOR_GETPOS(C) & 255;
		buf[len++] = digitalRead(MOTC_END) == MOTC_ENDLEVEL;
		buf[len++] = DCMOTOR(C).Vars.PWMConsign >> 8;
		buf[len++] = DCMOTOR(C).Vars.PWMConsign & 255;
		ramppos = (int)rampGetPos(&(DCMOTOR(C).PosRamp));
		buf[len++] = ramppos >> 8;
		buf[len++] = ramppos & 255;
		buf[len++] = DCMOTOR(C).VolVars.homed;
	} else {
		buf[len++] = 11;
		buf[len++] = DCMOTOR_GETPOS(D) >> 8;
		buf[len++] = DCMOTOR_GETPOS(D) & 255;
		buf[len++] = digitalRead(MOTD_END) == MOTD_ENDLEVEL;
		buf[len++] = DCMOTOR(D).Vars.PWMConsign >> 8;
		buf[len++] = DCMOTOR(D).Vars.PWMConsign & 255;
		ramppos = (int)rampGetPos(&(DCMOTOR(D).PosRamp));
		buf[len++] = ramppos >> 8;
		buf[len++] = ramppos & 255;
		buf[len++] = DCMOTOR(D).VolVars.homed;
	}
	buf[len++] = '\n';
	fraiseSend(buf,len);
	mot = 1 - mot;
}

/*#define DCMOTOR_UPDATE_ASYM_PROTECT_(motID) do{ \
	DCMOTOR_FORMATPWM(motID);\
	if((dcmotor_v < 0) && (digitalRead(MOT##motID##_END) == MOT##motID##_ENDLEVEL)) dcmotor_v = 0; \
	if((dcmotor_v > 0) && (digitalRead(MOT##motID##_HIEND) == MOT##motID##_ENDLEVEL)) dcmotor_v = 0; \
	dcmotor_vabs = dcmotor_v < 0 ? 1023 + dcmotor_v : dcmotor_v; \
	SET_PWM(MOT##motID##_PWM, dcmotor_vabs); \
	if(dcmotor_v < 0) { digitalSet(M##motID##2);}\
	else { digitalClear(M##motID##2);}\
 } while(0)
#define DCMOTOR_UPDATE_ASYM_PROTECT(motID) CALL_FUN(DCMOTOR_UPDATE_ASYM_PROTECT_,motID)*/

int count;
byte endSwitchOn;

void loop() {
// ---------- Main loop ------------
	fraiseService();	// listen to Fraise events
	analogService();	// analog management routine

	if(delayFinished(mainDelay)) // when mainDelay triggers :
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay
		//analogSend();		// send analog channels that changed
		/*if(digitalRead(MOTC_END) == MOTC_ENDLEVEL) {
			if(!endSwitchOn) {
				endSwitchOn = 1;
				rampInit(&(DCMOTOR(C).PosRamp));
			}
		} else endSwitchOn = 0;*/
		DCMOTOR_COMPUTE(C, ASYM);
		DCMOTOR_COMPUTE(D, ASYM);
		if(count++ > 10) {
			count = 0;
			sendMotorState();
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
	else if(c=='W') { 	// WRITE: save eeprom
		if((fraiseGetChar() == 'R') && (fraiseGetChar() == 'I') && (fraiseGetChar() == 'T') && (fraiseGetChar() == 'E'))
			EEwriteMain();
	}
}

void fraiseReceive() // receive raw
{
	unsigned char c;
	c=fraiseGetChar();

	switch(c) {
		case 120 : DCMOTOR_INPUT(C) ; break;
		case 121 : DCMOTOR_INPUT(D) ; break;
		//case 130 : rampStop(&(DCMOTOR(C).PosRamp)); break;
	}
}

// EEPROM

void EEdeclareMain()
{
	DCMOTOR_DECLARE_EE(C);
	DCMOTOR_DECLARE_EE(D);
}
