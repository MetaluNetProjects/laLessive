/*********************************************************************
 *
 *                Lyre for 8X2A + 2 VNH
 *
 *********************************************************************
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Antoine Rousseau  aug 28 2015     Original.
                     apr 11 2016     Update to latest Fraise & change mag hmc5883 for incremental sensor + zero
 ********************************************************************/

/*
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.
*/
#define BOARD 8X2A

#include <fruit.h>


#include <pid.h>
#include <ramp.h>
#include <dcmotor.h>
#include <servo.h>

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

DCMOTOR_DECLARE(D);
#define SERVOS_INACTIVE_TIME 200000
t_ramp servoRamps[2];
unsigned char servoRun[2];
t_delay servoDelays[2];
int servoVals[2];
unsigned char PERIOD=25;
//unsigned char t=25,t2;
unsigned char motorEndWasOn;

//long int TestVar,TestVar2;
t_delay mainDelay;

void highInterrupts()
{
	servoHighInterrupt();
	if(PIR1bits.TMR1IF) {
		DCMOTOR_CAPTURE_SERVICE(D);
		InitTimerUS(31);
	}
}




#if 0
// --------- Fraise Watchdog : ----------------//
unsigned int wdC = 0; //watchdog count

void wdReset(void)
{
	wdC = 0;
}

#define wdOK() (wdC < (200*2)) // 2 seconds
#define wdService() do {if(wdOK()) wdC++;} while(0)
#endif

//----------------------------------------------//

void setup(void)
{
	fruitInit();

// ---------- capture timer : TMR1 ------------
	T1CON=0b00110011;//src=fosc/4,ps=8,16bit r/w,on.
	PIE1bits.TMR1IE=1;  //1;
	IPR1bits.TMR1IP=1;
	
	dcmotorInit(D);
#if 0
	DCMOTOR(D).Setting.PosWindow = 100;
	DCMOTOR(D).Setting.PwmMin = 50;
	DCMOTOR(D).Setting.PosErrorGain = 1;
	
	DCMOTOR(D).PosRamp.maxSpeed = 2000;
	DCMOTOR(D).PosRamp.maxAccel = 5000;
	DCMOTOR(D).PosRamp.maxDecel = 5000;
	rampSetPos(&DCMOTOR(D).PosRamp, 16384);

	DCMOTOR(D).PosPID.GainP = 200; //90
	DCMOTOR(D).PosPID.GainI = 1;
	DCMOTOR(D).PosPID.GainD = 0;
	DCMOTOR(D).PosPID.MaxOut = 1023;

	DCMOTOR(D).VolVars.homed = 0;
#endif
	servoInit();
	servoSelect(0, SERVO1);
	servoSelect(1, SERVO2);

	analogWrite(LED1,0);
	analogWrite(LED2,0);
	analogWrite(LED3,0);
	pinModeAnalogOut(LED1);
	pinModeAnalogOut(LED2);
	pinModeAnalogOut(LED3);

	rampInit(&servoRamps[0]);
	rampInit(&servoRamps[1]);
	rampSetPos(&servoRamps[0], 10000);
	rampSetPos(&servoRamps[1], 10000);

	EEreadMain();
	DCMOTOR(D).Setting.onlyPositive = 1; // disable automatic end switch protection
	DCMOTOR(D).Setting.PosErrorGain = 4;
	DCMOTOR(D).Setting.PosWindow = 10;
	motorEndWasOn = 0;
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms
}

// ---------- Main loop ------------
void updateServo(unsigned char id)
{
	rampCompute(&servoRamps[id]);
	if(servoVals[id] != rampGetPos(&servoRamps[id])) {
		servoVals[id] = rampGetPos(&servoRamps[id]);
		servoSet(id,rampGetPos(&servoRamps[id]));
		delayStart(servoDelays[id], SERVOS_INACTIVE_TIME);
	}
	else if delayFinished(servoDelays[id]) servoSet(id,0);
}

void loop() {
	static unsigned char count = 25;
	fraiseService();
	servoService();

	if(delayFinished(mainDelay)) // when mainDelay triggers 
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay


		/*wdService();
		if(!wdOK()) {
			DCMOTOR(C).Vars.PWMConsign = 0;
			DCMOTOR(C).Setting.Mode = 0;
			DCMOTOR(D).Vars.PWMConsign = 0;
			DCMOTOR(D).Setting.Mode = 0;
		}*/

		DCMOTOR_COMPUTE(D, ASYM);
		if(DCMOTOR(D).VolVars.end && (motorEndWasOn == 0)) {
			DCMOTOR(D).Vars.PWMConsign = 0;
			DCMOTOR(D).Setting.Mode = 0;
		}
		if(DCMOTOR(D).VolVars.end) motorEndWasOn = 1;
		else motorEndWasOn = 0;

		//fraiseService();
		
		updateServo(0);
		updateServo(1);
		
		#if 0
		rampCompute(&servoRamps[0]);
		servoSet(0,rampGetPos(&servoRamps[0]));
		rampCompute(&servoRamps[1]);
		servoSet(1,rampGetPos(&servoRamps[1])); 
		#endif
		
		#if 1
		//t = t - 1;
		//count = 0;
		if(--count == 0){
			printf("CM %ld %d %d %ld\n",DCMOTOR_GETPOS(D), (int)DCMOTOR(D).VolVars.homed/*(int)motorEndWasOn*/, (int)DCMOTOR(D).VolVars.end, (long)rampGetPos(&DCMOTOR(D).PosRamp));
			count = PERIOD;
			//t2++;
			/*printf("CM %ld %ld %d %d\n",DCMOTOR_GETPOS(D),(long)rampGetPos(&DCMOTOR(D).PosRamp), DCMOTOR(D).Vars.PWMConsign,DCMOTOR(D).VolVars.homed);*/
		}
		#endif
	}
}

void fraiseReceiveChar()
{
	unsigned char c;
	unsigned char l = fraiseGetLen();	
	c=fraiseGetChar();
	if(c=='L'){	
		c=fraiseGetChar();
		/*if(c=='0') 
			{LED=0;}
		else LED=1;*/
	}
	else if(c=='E') {
		printf("C");
		for(c=1;c<l;c++) printf("%c",fraiseGetChar());
		putchar('\n');
	}
	/*else if(c=='W') { //watchdog
		wdReset();
	}*/
	else if(c=='S') { //EEPROM save
		if((fraiseGetChar() == 'A')
		&& (fraiseGetChar() == 'V')
		&& (fraiseGetChar() == 'E'))
			EEwriteMain();
	}	
}

void fraiseReceive()
{
	unsigned char c;//,c2;
	unsigned int i;
	
	c=fraiseGetChar();

	switch(c) {
		//PARAM_CHAR(1,t2); break;
		PARAM_CHAR(2,PERIOD); break;
		//case 20 : servoReceive(); break;// if first byte is 20, then call servo receive function.
		PARAM_INT(51, i) ; analogWrite(LED1,i); break;
		PARAM_INT(52, i) ; analogWrite(LED2,i); break;
		PARAM_INT(53, i) ; analogWrite(LED3,i); break;
		/*case 51 : analogWrite(LED1,fraiseGetInt()); break;
		case 52 : analogWrite(LED2,fraiseGetInt()); break;
		case 53 : analogWrite(LED3,fraiseGetInt()); break;*/
		case 101 : rampInput(&servoRamps[0]); break;
		case 102 : rampInput(&servoRamps[1]); break;
		case 120 : DCMOTOR_INPUT(D); break;
	}
}

void EEdeclareMain()
{
	DCMOTOR_DECLARE_EE(D);
	rampDeclareEE(&servoRamps[0]);
	rampDeclareEE(&servoRamps[1]);
}
