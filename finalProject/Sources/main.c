/*
*	Program for use with POTENTIOMETERS
*/

#include <hidef.h>      	/* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "eeprom.c"
#include "servo_location.h"
// #include "common.h"

void play(void);
void read_control_type(void);
void user_control(void);
void record(void);
void playback(void);
void initialize(void);
void wait(void);

// state definitions
#define READ_CONTROL 0
#define RESET        1
#define USER_CONTROL 2
#define RECORD       3
#define PLAYBACK     4
#define WATING		 5

#define RTI_CTL 	0x6A

const char CLOCK_FACTOR           = 3;
const unsigned int MAX_CLK_TICKS  = 60000; 	// for a 20 ms peroid
const unsigned int MAX_ARRAY_SIZE = 8000;	// number of writable locations
const int RIG_MEMORY_SIZE         = 8; 		// in bytes

char state;
int test;

unsigned int rig_1_array_last_index;

motionControlRig rig;

void main(void) 
{
	initialize();
	rig_1_array_last_index = 0;
	state = READ_CONTROL;
	test = 0;
	for(;;)
	{
		switch(state)
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
			case WATING:
				wait();
				break;
			default:
				state = READ_CONTROL;
				break;
		}
		if(PTM & 0x01)
		{
			rig.servo1.high_count = 4500;
			rig.servo2.low_count = 55500;
		}
		else
		{
			rig.servo1.high_count = 6000;
			rig.servo2.low_count = 54000;
		}
		_FEED_COP(); /* feeds the dog */
	} // loop forever
}

void play()
{
	unsigned int pot_voltage_1, pot_voltage_2, pot_voltage_3;
	pot_voltage_1 = ATDDR0L;
	pot_voltage_2 = ATDDR1L;
	pot_voltage_3 = ATDDR2L;

	if(pot_voltage_1 > 250) pot_voltage_1 = 250;
	if(pot_voltage_2 > 250) pot_voltage_2 = 250;
	if(pot_voltage_3 > 250) pot_voltage_3 = 250;

	pot_voltage_1 = (pot_voltage_1 * 12) + 3000; // multiply by 8 (conversion factor)
	pot_voltage_2 = (pot_voltage_2 * 12) + 3000; // multiply by 8 (conversion factor)

	rig.servo1.high_count = (rig.servo1.high_count * 2 + pot_voltage_1) / 3;
	rig.servo1.low_count = MAX_CLK_TICKS - rig.servo1.high_count;

	rig.servo2.high_count = (rig.servo2.high_count * 2 + pot_voltage_2) / 3;
	rig.servo2.low_count = MAX_CLK_TICKS - pot_voltage_2;

	if (pot_voltage_3 > 0x7D) 
	{
		rig.dc_right.high_count = (pot_voltage_3 - 0x7D) * 10;
		rig.dc_right.low_count = 1265 - rig.dc_right.high_count;
		rig.dc_right.enable = 1;
		rig.dc_left.enable = 0;
	} 
	else if( pot_voltage_3 < 0x7D)
	{
		rig.dc_left.high_count = (0x7D - pot_voltage_3) * 10;
		rig.dc_left.low_count = 1265 - rig.dc_left.high_count;
		rig.dc_left.enable = 1;
		rig.dc_right.enable = 0;
	} 
	else 
	{
		rig.dc_left.enable = 0;
		rig.dc_right.enable = 0;
	}
}

void read_control_type()
{
	if(PTT & 0x40)
	{
		state = USER_CONTROL;
		return;
	} 
	else if (PTM & 0x01)
	{
		state = RECORD;
		return;
	}
	else if(PTM & 0x02)
	{
		state = PLAYBACK;
		return;
	} 
	else // default
	{
		state = WATING;
	}
}

void record()
{
	unsigned char high_byte, low_byte;
	high_byte = 0x00;
	low_byte = 0x00;
	rig_1_array_last_index = 0;

	//something to change is ending recording with a stop state
	while((PTM & 0x01) && (rig_1_array_last_index < MAX_ARRAY_SIZE))
	{
		while (!(CRGFLG & 0x80)) ; // wait for RTI timeout 
 		CRGFLG = 0x80;

		play();
		if(high_byte != 0xff || low_byte != 0xff)
		{
			putMotorLocation(&rig, high_byte, low_byte);
		}
		else
		{
			putMotorLocation(&rig, high_byte, low_byte);
			break;
		}
		low_byte += RIG_MEMORY_SIZE;
		if(0xff - low_byte <= RIG_MEMORY_SIZE)
		{
			low_byte += RIG_MEMORY_SIZE;
			high_byte += 1;
		}
		rig_1_array_last_index += 1;
	}

	while(PTM & 0x01); // wait until recode switch is flipped
	state = READ_CONTROL;
}

void user_control()
{	
	while(PTT & 0x40) // while switch is on PT5
	{
		play();
	}
	state = READ_CONTROL;
}

void playback()
{
	unsigned int i;
	unsigned char high_byte, low_byte;
	high_byte = 0x00;
	low_byte = 0x00;
	
	rig.dc_right.enable = 1;
	rig.dc_left.enable = 1;
	
	for (i = 0; i < rig_1_array_last_index; i++)
	{
		while (!(CRGFLG & 0x80)) ; // wait for RTI timeout 
 		CRGFLG = 0x80;				// Clear the RTI timeout flag

		
 		getMotorLocation(&rig, high_byte, low_byte);
 		
		low_byte += RIG_MEMORY_SIZE;
		if(256 - low_byte <= RIG_MEMORY_SIZE)
		{
			low_byte += RIG_MEMORY_SIZE;
			high_byte += 1;
		}
	}
	rig.dc_right.enable = 0;
	rig.dc_left.enable = 0;

	while(PTM & 0x02); // wait until recode button is unpressed
	state = READ_CONTROL;
}

void wait()
{
	rig.servo1.high_count = 4500;
	rig.servo1.low_count = 55500;
	rig.servo2.high_count = 4500;
	rig.servo2.low_count = 55500;

	rig.dc_right.enable = 0;
	rig.dc_left.enable = 0;


	while(state == WATING)
	{
		read_control_type();
	}
	
}

void initialize()
{
	initializeSPI();
	initializePLL();
	// setup the Output compare struct
	rig.servo1.high_count = 6000;
	rig.servo1.low_count =  54000;
	rig.servo2.high_count = 6000;
	rig.servo2.low_count =  54000;

	rig.dc_right.high_count = 1;
	rig.dc_right.low_count = 1279;
	rig.dc_left.high_count = 1;
	rig.dc_left.low_count = 1279;

	// setup port PT0 for output
	// PT0,1,2,3,7 outptus
	DDRT 	= 0x8F;
	// 0, 1 inputs 
	DDRM	= ~(0x03); 
	
	// setup rti
	RTICTL = RTI_CTL;  

	// sets up input for ANO2 and AN03 analog input; only AN02 and AN03 as inputs
	ATDCTL2 = 0xC0;
	ATDCTL3 = 0x18;
	ATDCTL4 = 0x85;
	ATDCTL5 = 0xB2;

	// setup the outputcompare timer
	TSCR1 	= 0x90; 	// enables the TCNT and fst time flag clear
	TSCR2 	= 0x03; 	// set prescaler to 8. Makes a 21.8 ms peroid

	TIOS 	|= 0x0F; 	// setup channel 0, channel 1, channel 2, and channel 3 for output compare
	TIOS	&= ~(0x30);
	TCTL2	= 0xFF; 	// set 0cn action to pull high

	TC0		= TCNT + 10;	// wait a bit to send a high signal
	TC1 	= TCNT + 10;	// wait a bit to send a high signal
	TC2 	= TCNT + 10;	// wait a bit to send a high signal
	TC3 	= TCNT + 10;	// wait a bit to send a high signal

	while(!(TFLG1 & 0x0F)); // wait until C0F, C1F, C2F, C3F is set

	TCTL2 	= 0x05;		// set OC0 pin action to toggle
	TC0		+= rig.servo1.high_count;		// setup OC0 pin to the next action // high count
	TC1 	+= rig.servo2.high_count;		// setup OC1 pin to the next action // high count
	TC2 	+= rig.dc_right.high_count;		// setup OC2 pin to the next action // high count
	TC3		+= rig.dc_left.high_count;		// setup OC3 pin to the next action // high count

	rig.servo1.high_or_low = 0;		// indicate action for the next compare
	rig.servo2.high_or_low = 0; 	// indicate action for the next compare
	rig.dc_right.high_or_low = 0;	// indicate action for the next compare
	rig.dc_left.high_or_low = 0;	// indicate action for the next compare

	// Setup Input Capture
	//TCTL3 |= 0x05; // Sets up both channels 4 and 5 to capture on rising edge
	TIE 	= 0x0F; 	// enable OC0, OC1, OC2, OC3, OC4, and OC5 interrupt locally
	asm("cli"); 		// enable interrupts globally
}

//--------------INTERRUPT METHODS---------------//
/**
 * Interrupt to handle output compare on T0
 */
void interrupt VectorNumber_Vtimch0 togglePT0(void)
{
	if(rig.servo1.high_or_low)
	{
		TC0 += rig.servo1.high_count;
		rig.servo1.high_or_low = 0;
	}
	else
	{
		TC0	+= rig.servo1.low_count;
		rig.servo1.high_or_low = 1;
	}
}

/**
 * Interrupt to handle output compare on T1
 * @return  null
 */
void interrupt VectorNumber_Vtimch1 togglePT1(void)
{
	if (rig.servo2.high_or_low)
	{
		TC1 += rig.servo2.high_count;
		rig.servo2.high_or_low = 0;
	}
	else
	{
		TC1 += rig.servo2.low_count;
		rig.servo2.high_or_low = 1;
	}
}

/**
 * Interrupt to handle output compare on T2
 * @return  null
 */
void interrupt VectorNumber_Vtimch2 togglePT2(void)
{
	if (rig.dc_right.high_or_low)
	{
		TC2 += rig.dc_right.high_count;

		if (rig.dc_right.enable) { PTT |= 0x04; }
		else { PTT &= ~(0x04); }

		rig.dc_right.high_or_low = 0;
	}
	else
	{
		TC2 += rig.dc_right.low_count;
		PTT &= ~(0x04);
		rig.dc_right.high_or_low = 1;
	}
}

/**
 * Interrupt to handle output compare on T3
 * @return  null
 */
void interrupt VectorNumber_Vtimch3 togglePT3(void)
{
	if (rig.dc_left.high_or_low)
	{
		TC3 += rig.dc_left.high_count;
		if (rig.dc_left.enable) { PTT |= 0x08; }
		else { PTT &= ~(0x08); }
		rig.dc_left.high_or_low = 0;
	}
	else
	{
		TC3 += rig.dc_left.low_count;
		PTT &= ~(0x08);
		rig.dc_left.high_or_low = 1;;
	}
}

// void interrupt VectorNumber_Vtimch4 encoder_timingPT4()
// {
// 	test += 1;
// }

// void interrupt VectorNumber_Vtimch5 encoder_timingPT5()
// {

// }

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