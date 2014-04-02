/*
*	Program for use with POTENTIOMETERS
*/

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */

// State definitions
#define READ_CONTROL 0
#define RESET 1
#define USER_CONTROL 2
#define RECORD 3
#define PLAYBACK 4

// Struct to hold all motor position data
struct position
{
	unsigned int servo_1;	// duty cycle for PWM
	unsigned int servo_2;	// duty cycle for PWM
	unsigned int dc_motor;	// position due to feedback control (encoder)
};

const unsigned int DATA_ARRAY_SIZE = 100;

char STATE;
position pathData[DATA_ARRAY_SIZE];
unsigned int position_index;

void initialize()
{
	// Set PTT ports
	// Initialize 3 PWM systems
	// initialize interrupts
	// initialize pathData to all null position struct values
	STATE = RESET;
}

void read_control_type()
{
	// Check that free control button is pressed
	// Check that record button is pressed
	// Check if play button is pressed
}

void reset()
{
	// reset to initial values
	// home all motors
}

void user_control()
{
	// read input (potentiometers)
	// go to position
	// If (checkStopButtonPressed)
	// {
	// 	STATE = READ_CONTROL;
	// }
}

void record()
{
	// read input
	// save position
	// increment array index
	// go to position
	// if (STOP || EndOfPositionArray)
	// {
	// 	STATE = READ_CONTROL;
	// }
}
void playback()
{
	// go to start position
	// if (!atStart)
	// {
	// 	break;
	// }
	// read next position
	// if (StopStruct || position_index >= DATA_ARRAY_SIZE)
	// {
	// 	STATE = READ_CONTROL
	// } 
	// else
	// {
	// 	go to position
	// }

}

void main(void) {
	/* put your own code here */
	initialize();
	EnableInterrupts;
	while(1)
	{
		// microDelay(100); Use interrupts for accurate timing for time slices
		switch(STATE)
		{
			case READ_CONTROL:
				read_control_type();
				break;
			case RESET:
				reset();
				break;
			case USER_CONTROL:
				user_control();
				break;
			case RECORD:
				record();
				break;
			case PLAYBACK:
				playback();
				break;
			default:
				STATE = READ_CONTROL;
				break;
		}
	}
	
	for(;;) {
		_FEED_COP(); /* feeds the dog */
	} /* loop forever */
	/* please make sure that you never leave main */
}
