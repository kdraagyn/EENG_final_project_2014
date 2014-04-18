#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "servo_location.h"

#define WREN 0X06  	// wright enable
#define WR 0x02 	// write instruction 
#define RE 0x03		// read insturction

// SPISR flags for checking SPI slave flags
#define SPTEF 0x20
#define SPIF 0x80

//01011000 00011010
#define ENTER 0x0D
#define CS_PIN 0x80

typedef struct 
{
	char high_or_low;
	unsigned int high_count;
	unsigned int low_count;
} OutputCompare;

typedef struct 
{
	OutputCompare servo1;
	OutputCompare servo2;
	// DC MOTOR
} motionControlRig;

void initializePLL(void) {
	CLKSEL_PLLSEL = 0; 		// turn off PLL for source clock
	PLLCTL_PLLON = 1; 			// turn on PLL
	// Assuming OscFreq = $8000, we mult by synr+1=3 (to get 24MHz)
	SYNR_SYN = 2; 			// set PLL multiplier
	REFDV_REFDV = 0; 			// set PLL divider (divide by REFDV+1)
	CRGFLG_LOCKIF = 1; 		// clear the lock interrupt flag
	while(!CRGFLG_LOCK); 		// wait for the PLL to lock before continuing
	CLKSEL_PLLSEL = 1; 		// select the PLL for source clock
}

void initializeSCI(void)
{
	SCIBDH = 0x00;
	SCIBDL = 156;
	// configure word length, parity, and other figs
	SCICR1 = 0b00000100;
	// enable transmitter and receiver
	SCICR2 = 0b00001100;
}

void initializeSPI(void) {
	// Set baud rate. Baud rate divisor = (SPPR+1) * 2^(SPR+1).
	// for baud = 1 MHz divisor = 24 SPPR=5, SPR=1 -> 0x51
	SPIBR = 0x51; // Bits: 0 SPPR2 SPPR1 SPPR0 0 SPR2 SPR1 SPR0
	// SPICR1 bits: SPIE SPTIE SPE MSTR CPOL CPHA SSOE LSBFE
	// SPIE = 0  no interrupts
	 // SPE = 1  enable SPI system
	// SPTIE = 0 no interrupts
	// MSTR = 1  we are master
	// CPOL = 0 data latched on rising edge of clock
	// CPHA = 0 data starts with first edge
	// SSOE = 0 set to 1 to enable SS output
	// LSBF = 0 transfer most signif bit first
	SPICR1 = 0x50;

	// SPICR2 bits: 0 0 0 MODFEN BIDIROE 0 SPSWAI SPC0
	// MODFEN = 0 set to 1 to allow SS to go low on transmission
	// BIDIROE = 0 disable bidirectional mode
	// SPSWAI = 0 SPI clock operates normally in wait mode
	// SPC0 = 0 only used for bidirectional mode
	SPICR2 = 0x00;
}

void putcharSCI(char c) { 
 	// wait till transmit data register is empty 
 	while (!(SCISR1 & 0x80)) ; 
 	SCIDRL = c; // send character 
} 


char getCharSCI() {
	while (!(SCISR1 & 0x20)) ; // Wait for RDRF to be set 
 	return SCIDRL; // Then read the data 
}

void DelayuSec(int delayTime){
	asm
	{
		pshx // save register x
		ldx delayTime
		loop: psha
		pula
		psha
		pula
		psha
		pula
		psha
		pula
		nop
		dbne x, loop
		pulx // restore register x
	}
}

void delay(int delayTime){
	int i = 0;
	for (i = 0; i < delayTime; ++i)
	{
		DelayuSec(1000);
	}
}

// Send a character out through SPI
void putcharSPI(char cx)
{
	char temp; 
	while(!(SPISR & SPTEF));  	// wait until SPTEF=1 (transmit reg empty)
	SPIDR = cx;  				// output the byte to the SPI
	while(!(SPISR & SPIF)); 	// wait until SPIF=1 (receive reg full)
	temp = SPIDR; 				// clear the SPIF flag
}


// Get a character from SPI
char getcharSPI(void)
{
	while(!(SPISR & SPTEF)); 	// wait until SPTEF=1 (transmit reg empty)
	SPIDR = 0x00; 				// trigger 8 SCK pulses to shift in data
	while(!(SPISR & SPIF));  	// wait until SPIF=1 (receive reg full) 
	return SPIDR; 				// return the character
}

void putbyteSPI(char wr, char Haddress, char Laddress)
{
	// Assuming SPI has already been properly ran
	PTT &= ~(CS_PIN);			// select slave
	putcharSPI(WREN);		// write "write enable" to the EEPROM device
	PTT |= (CS_PIN);				// deselect slave
	PTT &= ~(CS_PIN);			// select slave
	putcharSPI(WR);			// write "write" instruction to the EEPROM device
	putcharSPI(Haddress);	// Write high address of the SPI device
	putcharSPI(Laddress);	// write low address of the SPI device
	putcharSPI(wr);			// Send Byte
	PTT |= (CS_PIN);				// deselect slave
	delay(5);				// delay 5 milliseconds
}

char getbyteSPI(char Haddress, char Laddress)
{
	char returned;
	PTT &= ~(CS_PIN); 			// select slave
	putcharSPI(RE);				// write "read" instruction to the SPI device
	putcharSPI(Haddress);		// Write high address of the SPI device
	putcharSPI(Laddress);		// write low address of the SPI device
	returned = getcharSPI();	// Get the next char from the SPI data register
	PTT |= (CS_PIN); 				// deselect slave
	return returned;
}

void putMotorLocation(motionControlRig* rig, unsigned char Haddress, unsigned char Laddress)
{
	// Assuming SPI has already been properly ran
	PTT &= ~(CS_PIN);			// select slave
	putcharSPI(WREN);		// write "write enable" to the EEPROM device
	PTT |= (CS_PIN);				// deselect slave
	PTT &= ~(CS_PIN);			// select slave
	putcharSPI(WR);			// write "write" instruction to the EEPROM device
	putcharSPI(Haddress);	// Write high address of the SPI device
	putcharSPI(Laddress);	// write low address of the SPI device
	putcharSPI((char) ((rig->servo1.high_count >> 8) & 0x00ff));	// Send Byte
	putcharSPI((char) (rig->servo1.high_count & 0x00ff));			// Send Byte
	putcharSPI((char) ((rig->servo2.high_count >> 8) & 0x00ff));	// Send Byte
	putcharSPI((char) (rig->servo2.high_count & 0x00ff));			// Send Byte
	PTT |= (CS_PIN);		// deselect slave
	delay(5);				// delay 5 milliseconds
}

void getMotorLocation(motionControlRig* rig, unsigned char Haddress, unsigned char Laddress)
{
	unsigned char returned;
	PTT &= ~(CS_PIN); 			// select slave
	putcharSPI(RE);				// write "read" instruction to the SPI device
	putcharSPI(Haddress);		// Write high address of the SPI device
	putcharSPI(Laddress);		// write low address of the SPI device

	rig->servo1.high_count = getcharSPI();	// Get the next char from the SPI data register
	rig->servo1.high_count <<= 8;
	returned = getcharSPI();
	rig->servo1.high_count += returned;
	rig->servo1.low_count = (unsigned int) 60000 - rig->servo1.high_count;

	rig->servo2.high_count = getcharSPI();	// Get the next char from the SPI data register
	rig->servo2.high_count <<= 8;
	returned = getcharSPI();
	rig->servo2.high_count += returned;
	rig->servo2.low_count = (unsigned int) 60000 - rig->servo2.high_count;

	PTT |= (CS_PIN); 				// deselect slave
}

// void main(void) {
// 	/* put your own code here */
// 	// set PT0 to output
// 	int Haddress, Laddress, i;
// 	DDRT = 0x01;

// 	Haddress = 0x00;
// 	Laddress = 0x00;
// 	initializeSPI();
// 	initializePLL();
// 	initializeSCI();
// 	EnableInterrupts;
// 	while (1)
// 	{
// 		somethingReturned = getCharSCI();					//from putty
// 		putbyteSPI(somethingReturned, Haddress, Laddress++);	// write to memory
// 		putcharSCI(somethingReturned);						// to putty
// 		if (somethingReturned == ENTER)
// 		{
// 			putcharSCI('\r');
// 			putcharSCI('\n');
// 			for(i = 0; i < Laddress; i++)
// 			{
// 				putcharSCI(getbyteSPI(Haddress,i));	
// 			}
// 			putcharSCI('\r');
// 			putcharSCI('\n');
// 			Haddress = Laddress = 0x00;
// 		}
// 	}

// 	// somethingGiven = 0x2A;
// 	// putbyteSPI(somethingGiven, 0xAA, 0xAA);
// 	// somethingReturned = getbyteSPI(0xAA, 0xAA);