/*
 * Lab9.c
 *
 * Created: 3/28/2017 1:52:21 PM
 * Author : Matt Spencer and Jordan Gaston
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdbool.h>
#include "acx.h"


//---------------------------------------------------
// Exec State Variables 
//---------------------------------------------------
uint16_t disable = 0xFE;     //leave Thread 0 enabled
uint16_t suspend = 0x00;
uint16_t delay = 0x00;

uint8_t x_thread_id = 0; //current thread
uint16_t x_thread_mask = 0x01;  //bit 0 set corresponds to thread 0

int global_timer = 0;

int stackSizes[8] = {STACK_SIZE0,STACK_SIZE1,STACK_SIZE2,STACK_SIZE3,STACK_SIZE4,STACK_SIZE5,STACK_SIZE6,STACK_SIZE7};
int offsets[8] = {STACK_OFFSET0,STACK_OFFSET1,STACK_OFFSET2,STACK_OFFSET3,STACK_OFFSET4,STACK_OFFSET5,STACK_OFFSET6,STACK_OFFSET7};
	
//---------------------------------------------------
// Stack Control
//---------------------------------------------------
STACKCONTROL threads[NUM_THREADS] = {{0}};

//---------------------------------------------------
// Stack Memory
//---------------------------------------------------
byte stackMem[MEM_SIZE] = {0};

//---------------------------------------------------
// Thread Delay Counters
//---------------------------------------------------
uint16_t delayCounters[MAX_THREADS] = {0};
	
//---------------------------------------------------
// Local Functions
//---------------------------------------------------
void fillCanaries()
{
	for(int i = 0; i < NUM_THREADS; i++)
	{
		stackMem[offsets[i]] = 0xAA;
	}
}

void initThreads()
{
	//threads[0].pStackBase = &stackMem[STACK_OFFSET0 + STACK_SIZE0 - 1];
	//threads[0].sp = &stackMem[STACK_OFFSET0 + STACK_SIZE0 - STACK_CONTEXT - 1];
	
	int temp = 0;
	for(int i = 0; i < NUM_THREADS; i++)
	{
		temp = offsets[i] + stackSizes[i] - 1;
		threads[i].pStackBase = &stackMem[temp];
		threads[i].sp = &stackMem[temp];
	}
}

void checkCanaries()
{
	for(int i = 0; i < NUM_THREADS; i++)
	{
		if(stackMem[offsets[i]] != 0xAA)
		{
			bufferOverflow();
		}
	}
}

void bufferOverflow()
{
	while(1)
	{
		//Blink in a specific way, for now we know infinite loop means buffer overflow
	}
}

//---------------------------------------------------
// ACX Functions
//---------------------------------------------------
void x_init(void)
{
	cli();

	fillCanaries();
	initThreads();

	TCCR0A = 0;     // set entire TCCR0A register to 0
	TCCR0B = 0;     // same for TCCR0B
	// set compare match register to desired timer count:
	OCR1A = 250;

	// Set CS10 and CS12 bits for 1024 prescaler:
	TCCR0B |= (1 << CS00);
	TCCR0B |= (1 << CS01);
	
	//Set compare on match interrupt
	TIMSK0 |= (1 << OCIE1A);
	
	int spasd = 0x21ff - SP;
	changeStack(SP,(byte*)threads[0].pStackBase,spasd);
	SP = threads[0].sp - spasd;

	sei();

	// return to caller.
}

int main(void)
{
	x_init();
	while(1){
		x_yield();
	}
}


void x_delay(unsigned int x)
{
	
}

unsigned long x_gtime()
{
	return 0;
}

void x_new(byte threadID, PTHREAD newthread, byte isEnabled)
{

}

void x_suspend(uint8_t x)
{
	
}

void x_resume(uint8_t x)
{
	
}

void x_disable(uint8_t x)
{

}

void x_enable(uint8_t x)
{
	
}

void changeStack(uint8_t * stackPointer, uint8_t * newStackBase, int num_bytes)
{
	byte *current = stackPointer;
	byte buff[num_bytes];
	
	for(int i = 0; i < num_bytes; i++)
	{
		current = current + 1;
		buff[i] = (*current);
		//current = current + 1;
	}
	
	current = newStackBase;
	
	for(int i = (num_bytes -1); i >= 0; i--){
		
		(*current) = buff[i];
		current = current - 1;
	}
}

typedef struct
{
	volatile uint8_t uscrA;
	volatile uint8_t uscrB;
	volatile uint8_t uscrC;
	volatile uint8_t reserved;
	volatile uint16_t ubrr;
	volatile uint8_t udr;
} SERIAL_REGS;
SERIAL_REGS* serial_ports[] =
{
	(SERIAL_REGS *)(0XC0),
	(SERIAL_REGS *)(0XC8),
	(SERIAL_REGS *)(0XD0),
	(SERIAL_REGS *)(0X130)
};
//Takes input parameters for 'port', 'speed' (baud rate) and 'framing' (frame parameters: number of data bits, parity, number
//of stop bits) in various combinations to configure the 2560 for communication using the specified serial port (0,1, 2, or 3)
void PSerial_open(unsigned char port, long speed, int config)
{
	serial_ports[port]->uscrA = 0x20;
	serial_ports[port]->uscrB = 0x18;
	serial_ports[port]->uscrC = config;
	serial_ports[port]->ubrr = speed;
}
//Waits for the write buffer to be available, then writes a byte value to the buffer. This function does not return anything.
void PSerial_write(unsigned char port, char data)
{
	while(!((serial_ports[port]->uscrA) & 0x20))
	{
	}
	serial_ports[port]->udr = data;
}

ISR(TIMER1_COMPA_vect)
{
	//Nothing for now
}

void x_schedule()
{
	byte isDone = 0;
	byte current_thread = x_thread_id + 1;
	do 
	{
		x_thread_mask = (1 << current_thread);	
		if((x_thread_mask & ~(disable | suspend | delay)))
		{
			isDone = true;
			x_thread_id = current_thread;
			restore();
		}
		if(current_thread < (NUM_THREADS - 1))
		{
			current_thread++;
		}
		else
		{
			current_thread = 0;
		}
		
	} while (true);
}