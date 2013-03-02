/*
 * MSP430 demo code for audio serial / motor driver booster pack
 * Control format is as follows
 * 	8 - both motors forward
 *	4 - turn left
 *	6 - turn right
 *	2 - both backward
 * 
 */
#define MOTOR_PERIOD 1000000
#define PWM_PERIOD 100
#define MOTOR_RATE PWM_PERIOD
//#define MOTOR_RATE (MOTOR_PERIOD/2)
#define GRN_LED BIT6
#define RED_LED BIT0
#define PWM_PIN BIT7
#define LEFT_DIR_PIN BIT4
#define RIGHT_DIR_PIN BIT5
//audio in is on hw serial pins, PORT1/PIN1&2
//PIN1 is RX
#define AUDIO_IN_PIN BIT1

#include <msp430.h>
#include <legacymsp430.h>
#include <stdint.h>
#include "msp430-ssbp.h"
#include <stdlib.h>

ringbuffer_ui8_16 usci_buffer = { 0, 0, { 0 } };

Serial<ringbuffer_ui8_16> usci0 = { usci_buffer };

interrupt(USCIAB0RX_VECTOR) USCI0RX_ISR(void) {

    usci_buffer.push_back(UCA0RXBUF);
}

int strlen(char * s)
{
	int len;
	for (len=0;*s;s++) len++;
	return len;
}

int main(void) {
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT

	BCSCTL1 = CALBC1_16MHZ; // set DCO clock for MCLK and SMCLK
	DCOCTL = CALDCO_16MHZ;

	usci0.init();
	P1OUT = 0;
	P1DIR |= RED_LED | GRN_LED | PWM_PIN | LEFT_DIR_PIN | RIGHT_DIR_PIN;
	P1OUT = 0;

	__bis_SR_register(GIE); // interrupts enabled
	long int MOTOR_LEFT=0;
	while(true) {
		uint8_t c;
		if (MOTOR_LEFT>0) {MOTOR_LEFT--;}
			else {P1OUT &= ~(PWM_PIN | LEFT_DIR_PIN | RIGHT_DIR_PIN);}
		while (!usci0.empty()) {
			c = (uint8_t) (usci0.recv());
			usci0.xmit((uint8_t)'[');
			usci0.xmit((uint8_t)c);
			usci0.xmit((uint8_t)']');
			switch(c)
			{
				case '+':
					P1OUT |= RED_LED;
					break;
				case '-':
					P1OUT &= ~RED_LED;
					break;
				case '2':
					MOTOR_LEFT=MOTOR_PERIOD;
					P1OUT |= PWM_PIN | LEFT_DIR_PIN | RIGHT_DIR_PIN;
					break;
				case '8':
					MOTOR_LEFT=MOTOR_PERIOD;
					P1OUT &= ~(LEFT_DIR_PIN | RIGHT_DIR_PIN);
					P1OUT |= PWM_PIN;
					break;
				case '6':
					MOTOR_LEFT=MOTOR_PERIOD;
					P1OUT &= ~(RIGHT_DIR_PIN);
					P1OUT |= PWM_PIN | LEFT_DIR_PIN;
					break;
				case '4':
					MOTOR_LEFT=MOTOR_PERIOD;
					P1OUT &= ~(LEFT_DIR_PIN);
					P1OUT |= PWM_PIN | RIGHT_DIR_PIN;
					break;
				case '0':
					P1OUT &= ~(PWM_PIN | LEFT_DIR_PIN | RIGHT_DIR_PIN);
					MOTOR_LEFT=0;
					break;
				default:
					;;
			}
		}
	}
}


