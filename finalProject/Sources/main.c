/*
*	Program for use with POTENTIOMETERS
*/

#include <hidef.h>      	/* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "common.h"

void play();
void read_control_type(void);
void user_control(void);
void record(void);
void playback(void);
void initialize(void);

// State definitions
#define READ_CONTROL 0
#define RESET        1
#define USER_CONTROL 2
#define RECORD       3
#define PLAYBACK     4

const char CLOCK_FACTOR = 3;
const unsigned int MAX_CLK_TICKS = 60000; // for a 20 ms peroid
const unsigned int MAX_ARRAY_SIZE = 100;
char STATE;

typedef struct 
{
	char high_or_low;
	unsigned int high_count;
	unsigned int low_count;
	// char pin;
} OutputCompare;


OutputCompare servo_1;
unsigned int servo_1_array_last_index;
OutputCompare servo_1_array[100]; // can't poot MAX_ARRAY_SIZE here compile failes

void main(void) 
{
	initialize();
	servo_1_array_last_index = 0;
	STATE = READ_CONTROL;
	for(;;)
	{
		switch(STATE)
		{
			case READ_CONTROL:
				read_control_type();
				break;
			case RESET:
				//reset();
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
		// if(PTM & 0x01)
		// {
		// 	OutputCompare.high_count = 4500;
		// 	OutputCompare.low_count = 55500;
		// }
		// else
		// {
		// 	OutputCompare.high_count = 6000;
		// 	OutputCompare.low_count = 54000;
		// }
		_FEED_COP(); /* feeds the dog */
	} // loop forever
}

void play()
{
	unsigned int pot_voltage;
	pot_voltage = ATDDR0L;
	if(pot_voltage > 250) 
		pot_voltage = 250;
	pot_voltage = (pot_voltage *12) + 3000; // multiply by 8 (conversion factor)
	servo_1.high_count = pot_voltage;
	servo_1.low_count = MAX_CLK_TICKS - pot_voltage;
}

void read_control_type()
{
	if(PTM & 0x01)
	{
		STATE = USER_CONTROL;
		return;
	} 
	else if (PTM & 0x02)
	{
		STATE = RECORD;
		return;
	}
	else if( PTM & 0x04)
	{
		STATE = PLAYBACK;
		return;
	} 
}

void record()
{
	servo_1_array_last_index = 0;
	while((PTM & 0x02) && (servo_1_array_last_index < MAX_ARRAY_SIZE))
	{
		while (!(CRGFLG & 0x80)) ; // wait for RTI timeout 
 		CRGFLG = 0x80;
		play();
		servo_1_array[servo_1_array_last_index].high_count = servo_1.high_count;
		servo_1_array[servo_1_array_last_index].low_count = servo_1.low_count;
		servo_1_array_last_index += 1;
	}
	while(PTM & 0x02); // wait until recode switch is flipped
	STATE = READ_CONTROL;
}

void user_control()
{
	while(PTM & 0x01) // while switch is on
	{
		play();
	}
	STATE = READ_CONTROL;
}

void playback()
{
	unsigned int i;
	for(i = 0; i < servo_1_array_last_index; i++)
	{
		while (!(CRGFLG & 0x80)) ; // wait for RTI timeout 
 		CRGFLG = 0x80;
		servo_1.high_count = servo_1_array[i].high_count;
		servo_1.low_count = servo_1_array[i].low_count;
	}
	while(PTM & 0x04); // wait unil recode button is unpressed
	STATE = READ_CONTROL;
}

void initialize()
{
	// setup the Output compare struct
	servo_1.high_count = 6000;
	servo_1.low_count =  54000;

	// setup port PT4 for output
	DDRT 	= 0x10;
	DDRM	= 0x00; // All PTM ports are inputs

	// setup rti
	RTICTL = 0x7F;  

	// setup analog input for 1 input
	ATDCTL2 = 0xC0;
  	ATDCTL3 = 0x08;
  	ATDCTL4 = 0x85;
  	ATDCTL5 = 0xA2; 

	// setup the outputcompare timer
	TSCR1 	= 0x90; 	// enables the TCNT and fst time flag clear
	TSCR2 	= 0x03; 	// set prescaler to 8. Makes a 21.8 ms peroid

	TIOS 	|= 0x10; 	// setup channel 4 for output compare
	TCTL1	= 0x03; 	// set 0c$ action to pull high
	TC4		= TCNT + 10;// wait a bit to send a high signal
	while(!(TFLG1 & 0x10)); // wait until C4F is set

	TCTL1 	= 0x01;		// set OC4 pin action to toggle
	TC4		+= servo_1.high_count;		// setup OC4 pin to the next action // high count
	servo_1.high_or_low = 0;// indicate action for the next compare
	TIE 	= 0x10; 	// enable OC4 interrupt locally
	asm("cli"); 		// enable interrupts globally
}

//--------------INTERRUPT METHODS---------------//
/**
 * Interrupt to handle output compare on T4
 */
void interrupt VectorNumber_Vtimch4 togglePT4(void)
{
	if(servo_1.high_or_low)
	{
		TC4 += servo_1.high_count;
		servo_1.high_or_low = 0;
	}
	else
	{
		TC4	+= servo_1.low_count;
		servo_1.high_or_low = 1;
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


For AD conversion to number of ticks
00-FF
0 to 255 // mod it to be 0 to 250 
3000 to 6000

(6000 - 3000)/255

*/