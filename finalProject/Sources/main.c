/*
*	Program for use with POTENTIOMETERS
*/

#include <hidef.h>      	/* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "common.h"

// State definitions
#define READ_CONTROL 0
#define RESET        1
#define USER_CONTROL 2
#define RECORD       3
#define PLAYBACK     4

const char CLOCK_FACTOR = 3;

struct 
{
	char high_or_low;
	int high_count;
	int low_count;
	// char pin;
} OutputCompare;

char STATE;
void initialize(void);
void main(void) 
{
	initialize();
	for(;;)
	{
		switch(STATE)
		{
			case READ_CONTROL:
				// read_control_type();
				break;
			case RESET:
				// reset();
				break;
			case USER_CONTROL:
				// user_control();
				break;
			case RECORD:
				// record();
				break;
			case PLAYBACK:
				// playback();
				break;
			default:
				// STATE = READ_CONTROL;
				break;
		}
		_FEED_COP(); /* feeds the dog */
	} // loop forever
}

void initialize()
{
	// setup the Output compare struct
	OutputCompare.high_count = 6000;
	OutputCompare.low_count =  54000;

	// setup port PT4 for output
	DDRT 	= 0x10;

	// setup the outputcompare timer
	TSCR1 	= 0x90; 	// enables the TCNT and fst time flag clear
	TSCR2 	= 0x03; 	// set prescaler to 8. Makes a 21.8 ms peroid

	TIOS 	|= 0x10; 	// setup channel 4 for output compare
	TCTL1	= 0x03; 	// set 0c$ action to pull high
	TC4		= TCNT + 10;// wait a bit to send a high signal
	while(!(TFLG1 & 0x10)); // wait until C4F is set

	TCTL1 	= 0x01;		// set OC4 pin action to toggle
	TC4		+= OutputCompare.high_count;		// setup OC4 pin to the next action // high count
	OutputCompare.high_or_low = 0;// indicate action for the next compare
	TIE 	= 0x10; 	// enable OC4 interrupt locally
	asm("cli"); 		// enable interrupts globally
}

//--------------INTERRUPT METHODS---------------//
/**
 * Interrupt to handle output compare on T4
 */
void interrupt VectorNumber_Vtimch4 togglePT4(void)
{
	if(OutputCompare.high_or_low)
	{
		TC4 += OutputCompare.high_count;
		OutputCompare.high_or_low = 0;
	}
	else
	{
		TC4	+= OutputCompare.low_count;
		OutputCompare.high_or_low = 1;
	}
}



/*  MATH
total of 60,000 clk ticks

NUTRAL POSITION
8/24 MHz = 333.333ns 
1.5ms/333.333(ns/clk tick) = 4500 clk ticks
18.5ms/333.333(ns/clk tick) = 55500 clk ticks


1ms/333.333(ns/clk tick) = 3000 clk ticks
19ms/333.333(ns/ clk tick) = 57000 clk ticks

2ms/333.333(ns/clk tick) = 6000 clk ticks
18ms/333.333(ns/clk tick) = 54000 clk ticks
*/