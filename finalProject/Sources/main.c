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

const unsigned int DATA_ARRAY_SIZE = 100;

struct position
{
	unsigned int servo_1;	// duty cycle for PWM
	unsigned int servo_2;	// duty cycle for PWM
	unsigned int dc_motor;	// position due to feedback control (encoder)
};

char STATE;

// PWM output compare variables
char HiorLo0, HiorLo1, HiorLo2, HiorLo3, HiorLo4, HiorLo5, HiorLo6, HiorLo7;
int HiCnt0, HiCnt1, HiCnt2, HiCnt3, HiCnt4, HiCnt5, HiCnt6, HiCnt7;
int LoCnt0, LoCnt1, LoCnt2, LoCnt3, LoCnt4, LoCnt5, LoCnt6, LoCnt7;

// Struct to hold all motor position data
// position pathData[DATA_ARRAY_SIZE];
unsigned int position_index;
/**
 * Setup timer of the clock that will time the PWM
 * @param clock_factor [description]
 * 
 * Bottom three bits of TSCR2 (PR2,PR1,PR0) determine TCNT period divide at 24MHz 
 * 		000 	1 		42ns 	TOF 	2.73ms 
 * 		001 	2 		84ns 	TOF 	5.46ms 
 * 		010 	4 		167ns 	TOF 	10.9ms 
 * 		011 	8 		333ns 	TOF 	21.8ms 
 * 		100 	16 		667ns 	TOF 	43.7ms 
 * 		101 	32 		1.33us 	TOF 	87.4ms 
 * 		110 	64 		2.67us 	TOF 	174.8ms 
 * 		111 	128 	5.33us 	TOF 	349.5ms 
 */
void setup_timer(char clock_factor)
{
	TSCR1 = 0x90;
	TSCR2 = clock_factor & 0x07;
}

/*
 * setup output capture sets up output compare on an array of T port pins.
 * 	The function will not effect any past output compare register declerations
 * 	since all changes are only made by way of masking.
 * @param pin            Pin or pins to setup outpu capture on
 * @param compare_time   # of clock ticks to time
 * @param interrupts     enable or disable interrupts
 * @param compare_action chose how to what action to occur at an interrupt
 *                       	TCTL_NO_ACTION, TCTL_TOGGLE, TCTL_SET_LOW, TCTL_SET_HIGH 
 */
void setup_output_capture(char pin, int compare_time, char interrupts, char compare_action)
{
	/*
	 *	Set registers 0 through 7 
	 *		0 for input captures
	 *		1 for output captures
	 */
	TIOS |= pin;

	/*
	 *	Initialize the DDRT to the correct outpus
	 * 		0 for input
	 * 		1 for output
	 */
	DDRT |= pin;
	
	if (interrupts)
	{
		/**
		 * Enable interrupt
		 * 	0 for interrupt disabled
		 * 	1 for interrupt enabled
		 */
		TIE |= pin;
	}

	//Clear All CxF flags
	TFLG1 = 0xFF;

	// Figure out the correct pin to set up
	// Set up all the 16 bit and odd bit map registers
	/*
	 * 	TCTL1 : TCTL2
	 * 	
	 * 	Specify the action to be taken when a match occurs
	 * 		OMn Oln	output level
	 * 		0 	0 	no action (timer disconnected from outpu pin) TCTL_NO_ACTION
	 * 		0 	1 	toggle OCn pin  TCTL_TOGGLE
	 * 		1 	0 	clear OCn pin to 0 	TCTL_SET_LOW
	 * 		1 	1 	set OCn pin to HIGH  TCTL_SET_HIGH
	 */
	if (pin && PT0)
	{
		TC0 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC0 = compare_time;
		TCTL2 |= compare_action;
	}
	
	if (pin && PT1)
	{
		TC1 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC1 = compare_time;
		TCTL2 |= compare_action << 2;
	}
	
	if (pin && PT2)
	{
		TC2 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC2 = compare_time;
		TCTL2 |= compare_action << 4;
	}
	
	if (pin && PT3)
	{
		TC3 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC3 = compare_time;
		TCTL2 |= compare_action << 6;
	}
	
	if (pin && PT4)
	{
		TC4 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC4 = compare_time;
		TCTL1 |= compare_action;
	}
	
	if (pin && PT5)
	{
		TC5 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC5 = compare_time;
		TCTL1 |= compare_action << 2;
	}
	
	if (pin && PT6)
	{
		TC6 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC6 = compare_time;
		TCTL1 |= compare_action << 4;
	}
	
	if (pin && PT7)
	{
		TC7 = TCNT + 10;
		while(!(TFLG1 & pin)) // wait until CnF flag is set
		
		TC7 = compare_time;
		TCTL1 |= compare_action << 6;
	}
}

void initializePWM(int pin, int period, int duty)
{
	// Setup output compare on pin
	
}

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


//--------------INTERRUPT METHODS---------------//
//
/**
 * Interrupt to handle output compare on T0
 * @return  null
 */
void interrupt VectorNumber_Vtimch0 togglePT0(void)
{
	if (HiorLo0)
	{
		TC0 += HiCnt0;
		HiorLo0 = 0;
	}
	else
	{
		TC0 += LoCnt0;
		HiorLo0 = 1;
	}
}

/**
 * Interrupt to handle output compare on T1
 * @return  null
 */
void interrupt VectorNumber_Vtimch1 togglePT1(void)
{
	if (HiorLo1)
	{
		TC0 += HiCnt1;
		HiorLo0 = 0;
	}
	else
	{
		TC0 += LoCnt1;
		HiorLo0 = 1;
	}
}

/**
 * Interrupt to handle output compare on T2
 * @return  null
 */
void interrupt VectorNumber_Vtimch2 togglePT2(void)
{
	if (HiorLo2)
	{
		TC0 += HiCnt2;
		HiorLo0 = 0;
	}
	else
	{
		TC0 += LoCnt2;
		HiorLo0 = 1;
	}
}

/**
 * Interrupt to handle output compare on T3
 * @return  null
 */
void interrupt VectorNumber_Vtimch3 togglePT3(void)
{
	if (HiorLo3)
	{
		TC0 += HiCnt3;
		HiorLo0 = 0;
	}
	else
	{
		TC0 += LoCnt3;
		HiorLo0 = 1;
	}
}

/**
 * Interrupt to handle output compare on T4
 * @return  null
 */
void interrupt VectorNumber_Vtimch4 togglePT4(void)
{
	if (HiorLo4)
	{
		TC0 += HiCnt4;
		HiorLo0 = 0;
	}
	else
	{
		TC0 += LoCnt4;
		HiorLo0 = 1;
	}
}