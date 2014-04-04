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

#define NUMBER_OF_SERVOS	2
#define NUMBER_OF_DC_MOROTS	1
#define CLOCK_FACTOR 3
#define CLOCK_FACTORED_PERIOD 333
#define PWM_PERIOD 20

const unsigned int DATA_ARRAY_SIZE = 100;

typedef struct 
{
	char HiorLo;
	int HiCnt;
	int LoCnt;
	char pin;
} pwmOutput;

typedef struct 
{
	pwmOutput servo_1;
	pwmOutput servo_2;
	pwmOutput dc_0;
	pwmOutput dc_1;
} position;

char STATE;


// PWM output compare variables
pwmOutput output_0, output_1, output_2, output_3, output_4, output_5, output_6, output_7, output_8;

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
	if (pin & PT0)
	{
		TCTL2 |= TCTL_SET_LOW;		// Make sure first output compare sets output to low
		TC0 = TCNT + 10;			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); // wait until CnF flag is set
		
		TC0 = compare_time;			// Set the output compare time
		TCTL2 |= compare_action;	// Set the output compare action
	}
	
	if (pin & PT1)
	{
		TCTL2 |= TCTL_SET_LOW << 2;		// Make sure first output compare sets output to low
		TC1 = TCNT + 10;			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); // wait until CnF flag is set
		
		TC1 = compare_time;			// Set the output compare time
		TCTL2 |= compare_action << 2;	// Set the output compare action
	}
	
	if (pin & PT2)
	{
		TCTL2 |= TCTL_SET_LOW << 4;		// Make sure first output compare sets output to low
		TC2 = TCNT + 10;			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); // wait until CnF flag is set
		
		TC2 = compare_time;			// Set the output compare time
		TCTL2 |= compare_action << 4;	// Set the output compare action
	}
	
	if (pin & PT3)
	{
		TCTL2 |= TCTL_SET_LOW << 6;		// Make sure first output compare sets output to low
		TC3 = TCNT + 10;			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); // wait until CnF flag is set
		
		TC3 = compare_time;			// Set the output compare time
		TCTL2 |= compare_action << 6;	// Set the output compare action
	}
	
	if (pin & PT4)
	{
		TCTL1 |= TCTL_SET_LOW;		// Make sure first output compare sets output to low
		TC4 = TCNT + 10;			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); // wait until CnF flag is set
		
		TC4 = compare_time;			// Set the output compare time
		TCTL1 |= compare_action;	// Set the output compare action
	}
	
	if (pin & PT5)
	{
		TCTL1 |= TCTL_SET_LOW << 2;		// Make sure first output compare sets output to low
		TC5 = TCNT + 10;			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); // wait until CnF flag is set
		
		TC5 = compare_time;			// Set the output compare time
		TCTL1 |= compare_action << 2;	// Set the output compare action
	}
	
	if (pin & PT6)
	{
		TCTL1 |= TCTL_SET_LOW << 4;		// Make sure first output compare sets output to low
		TC6 = TCNT + 10;					// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); 				// wait until CnF flag is set
		
		TC6 = compare_time;			// Set the output compare time
		TCTL1 |= compare_action << 4;	// Set the output compare action
	}
	
	if (pin & PT7)
	{
		TCTL1 |= TCTL_SET_LOW << 6;		// Make sure first output compare sets output to low
		TC7 = TCNT + 10;				// Make sure output compare starts high			// Wait untill outpu pin is set low
		while(!(TFLG1 & pin)); 			// wait until CnF flag is set
		
		TC7 = compare_time;			// Set the output compare time
		TCTL1 |= compare_action << 6;	// Set the output compare action
	}
}

//----- HARD CODED CLOCK FACTOR!!-----//
/*
 * [initializePWM description]
 * @param pin    pwm output pin
 * @param period period time in milliseconds
 * @param duty   duty cycle in milliseconds
 */
void initializePWM(pwmOutput* pin, int period, int duty)
{
	long clock_period_ticks, clock_duty_ticks;
	int clock_timer_factor;

	clock_period_ticks = ((long)period * 1000000L) / CLOCK_FACTORED_PERIOD;
	clock_duty_ticks = ((long)duty * 1000000L) / CLOCK_FACTORED_PERIOD;

	pin->HiCnt = clock_duty_ticks;
	pin->LoCnt = clock_period_ticks - clock_duty_ticks;

	// Will always use interrupts for control
	// Will have to set pwm struct duty cycles hitime, lowtime
	// Setup output compare on pin
	setup_timer(CLOCK_FACTOR);
	setup_output_capture(pin->pin, pin->HiCnt, true, TCTL_TOGGLE);
}

void initialize(int* PWMpins, int number_of_pwm_pins, char interrupts)
{
	// Set PTT ports
	// Initialize 3 PWM systems
	// initialize interrupts
	// initialize pathData to all null position struct values
	// PWM_peroid * 75 / 100 will setup the nutural duty cycle 
	// PWM_period should be 20 ms
	// int i;
	// for(i = 0; i < number_of_pwm_pins; i++)
	// {
	// 	initializePWM(PWMpins[i], PWM_PERIOD, (PWM_PERIOD*75)/100);
	// }
	output_0.pin = 1;

	initializePWM(&output_0, PWM_PERIOD, (PWM_PERIOD*75)/100);
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

void main(void) 
{
// void initialize(int* PWMpins, int number_of_pwm_pins, char interrupts)
	char servoPins[1] = {0};
	initialize(servoPins, 1, true);
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
	if (output_0.HiorLo)
	{
		TC0 += output_0.HiCnt;
		output_0.HiorLo = 0;
	}
	else
	{
		TC0 += output_0.LoCnt;
		output_0.HiorLo = 1;
	}
}

/**
 * Interrupt to handle output compare on T1
 * @return  null
 */
void interrupt VectorNumber_Vtimch1 togglePT1(void)
{
	if (output_1.HiorLo)
	{
		TC0 += output_1.HiCnt;
		output_1.HiorLo = 0;
	}
	else
	{
		TC0 += output_1.LoCnt;
		output_1.HiorLo = 1;
	}
}

/**
 * Interrupt to handle output compare on T2
 * @return  null
 */
void interrupt VectorNumber_Vtimch2 togglePT2(void)
{
	if (output_2.HiorLo)
	{
		TC0 += output_2.HiCnt;
		output_2.HiorLo = 0;
	}
	else
	{
		TC0 += output_2.LoCnt;
		output_2.HiorLo = 1;
	}
}

/**
 * Interrupt to handle output compare on T3
 * @return  null
 */
void interrupt VectorNumber_Vtimch3 togglePT3(void)
{
	if (output_3.HiorLo)
	{
		TC0 += output_3.HiCnt;
		output_3.HiorLo = 0;
	}
	else
	{
		TC0 += output_3.LoCnt;
		output_3.HiorLo = 1;
	}
}

/**
 * Interrupt to handle output compare on T4
 * @return  null
 */
void interrupt VectorNumber_Vtimch4 togglePT4(void)
{
	if (output_1.HiorLo)
	{
		TC0 += output_4.HiCnt;
		output_1.HiorLo = 0;
	}
	else
	{
		TC0 += output_4.LoCnt;
		output_4.HiorLo = 1;
	}
}