/*********************************************************************
 *
 *                5 PWM out for Versa2
 *
 *********************************************************************
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Antoine Rousseau  nov 15 2023     Original.
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
#define BOARD Versa2

#include <fruit.h>

t_delay mainDelay;

int values[5];
int target[5];

//----------------------------------------------//

void setup(void)
{
	fruitInit();

	pinModeAnalogOut(LAMP1);
	pinModeAnalogOut(LAMP2);
	pinModeAnalogOut(LAMP3);
	pinModeAnalogOut(LAMP4);
	pinModeAnalogOut(LAMP5);

	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms
}

// ---------- Main loop ------------

void loop() {
	fraiseService();

	if(delayFinished(mainDelay)) // when mainDelay triggers 
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay

		for(unsigned char n = 0 ; n < 5 ; n++) {
			values[n] = values[n] + target[n] - (values[n] >> 3);
		}
		analogWrite(LAMP1, values[0] >> 3);
		analogWrite(LAMP2, values[1] >> 3);
		analogWrite(LAMP3, values[2] >> 3);
		analogWrite(LAMP4, values[3] >> 3);
		analogWrite(LAMP5, values[4] >> 3);
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
}

void fraiseReceive()
{
	unsigned char c;//,c2;
	unsigned int i;
	
	c=fraiseGetChar();

	switch(c) {
		PARAM_INT(50, i); target[0] = i; break;
		PARAM_INT(51, i); target[1] = i; break;
		PARAM_INT(52, i); target[2] = i; break;
		PARAM_INT(53, i); target[3] = i; break;
		PARAM_INT(54, i); target[4] = i; break;
		case(100):
			printf("C%d %d %d %d %d\n",
				values[0], values[1], values[2], values[3], values[4]);
			break;
		
	}
}

void EEdeclareMain()
{
}
