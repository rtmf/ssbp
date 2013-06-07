/*
 * MSP430 demo code for audio serial / motor driver booster pack
 * Control format is as follows
 * 	8 - both motors forward
 *	4 - turn left
 *	6 - turn right
 *	2 - both backward
 * 
 */
#define MOTOR_PERIOD 10000
#define PWM_PERIOD 100
#define MOTOR_RATE PWM_PERIOD
//#define MOTOR_RATE (MOTOR_PERIOD/2)
#define RED_LED BIT0

//#define JPOP

#ifdef JPOP

//all pins are on PORT1
#define LEFT_PWM_PIN BIT7
#define RIGHT_PWM_PIN BIT6
#define LEFT_DIR_PIN BIT5
#define RIGHT_DIR_PIN BIT4
#define JPOUT P1OUT

#define FORWARD 1
#define REVERSE 0
int POWER=PWM_PERIOD;
#else

//RIGHT PWM pin is on PORT1
#define RIGHT_PWM_PIN BIT6
//DIR pins are on PORT2
#define LEFT_PWM_PIN BIT5
#define LEFT_DIR_PIN BIT4
#define RIGHT_DIR_PIN BIT3
#define JPOUT P2OUT

#define FORWARD 0
#define REVERSE 1
int POWER=PWM_PERIOD/2;
#endif

#define ON 1
#define OFF 0
//audio in is on hw serial pins, PORT1/PIN1&2
//PIN1 is RX
#define AUDIO_IN_PIN BIT1

#include <msp430.h>
#include <legacymsp430.h>
#include <stdint.h>
#include "msp430-ssbp.h"
#include <stdlib.h>

long int MOTOR_LEFT=0;
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

int lpq=0, lrate=PWM_PERIOD/2, rpq=0, rrate=PWM_PERIOD/2;

int len=OFF,ren=OFF,ldir=0,rdir=0;
void pulseDrive()
{
	if (MOTOR_LEFT>0)
		MOTOR_LEFT--;
	else
	{
		len=OFF;
		ren=OFF;
		lrate=0;
		rrate=0;
		ldir=FORWARD;
		rdir=FORWARD;
	}
	if (len==ON)
	{
		lpq+=lrate;
		if (lpq>=PWM_PERIOD)
		{
			lpq-=PWM_PERIOD;
			JPOUT |= LEFT_PWM_PIN;
		}
		else
			JPOUT &= ~LEFT_PWM_PIN;
	}
	else
		JPOUT &= ~LEFT_PWM_PIN;
	if (ren==ON)
	{
		rpq+=rrate;
		if (rpq>=PWM_PERIOD)
		{
			rpq-=PWM_PERIOD;
			P1OUT |= RIGHT_PWM_PIN;
		}
		else
			P1OUT &= ~RIGHT_PWM_PIN;
	}
	else
		P1OUT &= ~RIGHT_PWM_PIN;
	if (ldir)
		JPOUT |= LEFT_DIR_PIN;
	else
		JPOUT &= ~LEFT_DIR_PIN;
	if (rdir)
		JPOUT |= RIGHT_DIR_PIN;
	else
		JPOUT &= ~RIGHT_DIR_PIN;
}

void setDrive(int leftEnable,int leftAmt,int leftDir,int rightEnable,int rightAmt,int rightDir)
{
	//TODO don't ignore Amt
	if (leftEnable==OFF && rightEnable==OFF)
		MOTOR_LEFT=0;
	else
		MOTOR_LEFT=MOTOR_PERIOD;
	len=leftEnable;
	ren=rightEnable;
	ldir=leftDir;
	rdir=rightDir;
	lrate=leftAmt;
	rrate=rightAmt;
	pulseDrive();
}

int main(void) {
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT

	BCSCTL1 = CALBC1_16MHZ; // set DCO clock for MCLK and SMCLK
	DCOCTL = CALDCO_16MHZ;

	usci0.init();
	P1DIR &= ~BIT1;
	P1REN &= ~BIT1;
#ifdef JPOP
	P1DIR |= RED_LED | RIGHT_PWM_PIN | LEFT_DIR_PIN | RIGHT_DIR_PIN | LEFT_PWM_PIN;
#else
	P1DIR |= RED_LED | RIGHT_PWM_PIN;
	P2DIR |= LEFT_DIR_PIN | RIGHT_DIR_PIN | LEFT_PWM_PIN;
#endif
	P1OUT = 0;
	P2OUT = 0;
	__bis_SR_register(GIE); // interrupts enabled
	while(true) {
		uint8_t c;
		pulseDrive();
		while (!usci0.empty()) {
			c = (uint8_t) (usci0.recv());
			usci0.xmit((uint8_t)'[');
			usci0.xmit((uint8_t)c);
			usci0.xmit((uint8_t)']');
			switch(c)
			{
/*				case 'Q':
					P2OUT |= LEFT_DIR_PIN;
					break;
				case 'q':
					P2OUT &= ~LEFT_DIR_PIN;
					break;
				case 'W':
					P2OUT |= RIGHT_DIR_PIN;
					break;
				case 'w':
					P2OUT &= ~RIGHT_DIR_PIN;
					break;
				case 'R':
					P1OUT |= RIGHT_PWM_PIN;
					break;
				case 'r':
					P1OUT &= ~RIGHT_PWM_PIN;
					break;
				case 'L':
					P2OUT |= LEFT_PWM_PIN;
					break;
				case 'l':
					P2OUT &= ~LEFT_PWM_PIN;
					break;*/
				case 'P':
					POWER+=10;
					if (POWER>PWM_PERIOD) POWER=PWM_PERIOD;
					break;
				case 'p':
					if (POWER>=10)
						POWER-=10;
					else
						POWER=0;
					break;
/*				case 'I':
					lrate+=10;
					if (lrate>PWM_PERIOD) lrate=PWM_PERIOD;
					break;
				case 'i':
					if (lrate>=10)
						lrate-=10;
					else
						lrate=0;
					break;
				case 'O':
					rrate+=10;
					if (rrate>PWM_PERIOD) rrate=PWM_PERIOD;
					break;
				case 'o':
					if (rrate>=10)
						rrate-=10;
					else
						rrate=0;
					break;*/
				case '+':
					P1OUT |= RED_LED;
					break;
				case '-':
					P1OUT &= ~RED_LED;
					break;
				case '2':
					setDrive(ON,POWER,FORWARD,ON,POWER,FORWARD);
					break;
				case '8':
					setDrive(ON,POWER,REVERSE,ON,POWER,REVERSE);
					break;
				case '4':
					setDrive(ON,POWER,REVERSE,ON,POWER,FORWARD);
					break;
				case '1':
					setDrive(OFF,0,REVERSE,ON,POWER,FORWARD);
					break;
				case '6':
					setDrive(ON,POWER,FORWARD,ON,POWER,REVERSE);
					break;
				case '3':
					setDrive(ON,POWER,FORWARD,OFF,0,REVERSE);
					break;
				case '7':
					setDrive(OFF,0,FORWARD,ON,POWER,REVERSE);
					break;
				case  '9':
					setDrive(ON,POWER,REVERSE,OFF,0,FORWARD);
					break;
				case '5':
				case '\n':
				case '\r':
				case '0':
					setDrive(OFF,0,FORWARD,OFF,0,FORWARD);
					break;
				default:
					;;
			}
		}
	}
}


