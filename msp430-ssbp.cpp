/*
 * command format:
 * <number>[{c|j|v|d|l|/}]{\n|\r}
 * coerce C to c, J to j, V to v, D to d, L to l
 * if command character is missing, treat as j command
 * command types:
 *  d: set speed, range: 0-127
 *  q: set speed for turning and reverse single tread turning
 *  /: ? ('timermax')
 *  c: servo1 control, range: 0-255
 *  v: servo2 control, range: 0-255
 *  j: motor control, phone keypad layout, 0 or 5 to stop
 *  l: red LED control, 0=off, other value=on
 *  z: left motor power for differential drive
 *  x: right motor power for differential drive
 *  s: differential drive joystick command
 */
#include <msp430.h>
#include <legacymsp430.h>
#include <stdint.h>
#include "msp430-ssbp.h"
#include <stdlib.h>


//tune SCALE numbers to match motors on board in question
//TODO implement this
#define LEFT_SCALE 1
#define RIGHT_SCALE 1
//tune MOTOR_TIME to be a reasonable distance for each 'j' command
//#define MOTOR_TIME 10000

#define PWM_PERIOD 256

#define RED_LED BIT0

//#define JPOP
//this is doable from the Makefile now

#define FLIP
//this flips the numeric keypad from phone layout to numpad layout


#define PWM_CLOCK 8192

int timer=15;
#define MOTOR_TIME (timer<<5)
int POWER=PWM_PERIOD-1;
int REVPOWER=POWER/2;
int LEFTPOWER=PWM_PERIOD-1;
int RIGHTPOWER=PWM_PERIOD-1;
#define PDM
#ifdef JPOP

//all pins are on PORT1
#define LEFT_PWM_PIN BIT7
#define RIGHT_PWM_PIN BIT6
#define LEFT_DIR_PIN BIT5
#define RIGHT_DIR_PIN BIT4
#define JPOUT P1OUT

#define FORWARD 1
#define REVERSE 0
#else

#define SERVO
//turns on servos
#define SERVO_A_PIN BIT2
#define SERVO_B_PIN BIT3
//RIGHT PWM pin is on PORT1
#define RIGHT_PWM_PIN BIT6
//DIR pins are on PORT2
#define LEFT_PWM_PIN BIT5
#define LEFT_DIR_PIN BIT4
#define RIGHT_DIR_PIN BIT3
#define JPOUT P2OUT

#define FORWARD 0
#define REVERSE 1
#endif

#ifdef SERVO
#define servoCount 2
int servoLoc[servoCount];
int servoPin[servoCount] = {SERVO_A_PIN,SERVO_B_PIN};
#endif

#define ON 1
#define OFF 0
//audio in is on hw serial pins, PORT1/PIN1&2
//PIN1 is RX
#define AUDIO_IN_PIN BIT1

long int MOTOR_TIMER=0;
ringbuffer_ui8_16 usci_buffer = { 0, 0, { 0 } };

Serial<ringbuffer_ui8_16> usci0 = { usci_buffer };

int strlen(char * s)
{
	int len;
	for (len=0;*s;s++) len++;
	return len;
}

int lrate=PWM_PERIOD/2, rrate=PWM_PERIOD/2;

int len=OFF,ren=OFF,ldir=0,rdir=0;
#ifdef PDM
int lpq=0, rpq=0;
#else
int pwm=0;
#endif
int commands_waiting=0;
interrupt(USCIAB0RX_VECTOR) USCI0RX_ISR(void) {
	char c;
	c = UCA0RXBUF;
	//coerce c if necessary
	switch(c)
	{
		case 'C':
			c='c';
			break;
		case 'J':
			c='j';
			break;
		case 'D':
			c='d';
			break;
		case 'Q':
			c='q';
			break;
		case 'V':
			c='v';
			break;
		case 'L':
			c='l';
			break;
		case 'Z':
			c='z';
			break;
		case 'X':
			c='x';
			break;
		case 'S':
			c='s';
			break;
		case '\r':
		case '\n':
			c='\n';
			commands_waiting++;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'c':
		case 'd':
		case 'v':
		case 'j':
		case 'l':
		case 'q':
		case 'z':
		case 'x':
		case 's':
		case '/':
			break;
		default:
			c='\0';
			break;
	}
	if (c!='\0') usci_buffer.push_back(c);
}

int doDrive=0;
void pulseDrive()
{
	if (doDrive==0) return;
	doDrive=0;
	if (MOTOR_TIMER>0)
		MOTOR_TIMER--;
	else
	{
		len=OFF;
		ren=OFF;
		lrate=0;
		rrate=0;
		ldir=FORWARD;
		rdir=FORWARD;
	}
#ifndef PDM
	pwm++;
	if (pwm>=PWM_PERIOD) pwm=0;
#endif
	if (len==ON)
	{
#ifdef PDM
		lpq+=lrate;
		if (lpq>=PWM_PERIOD)
#else
		if (pwm<=lrate)
#endif
		{
#ifdef PDM
			lpq-=PWM_PERIOD;
#endif
			JPOUT |= LEFT_PWM_PIN;
		}
		else
			JPOUT &= ~LEFT_PWM_PIN;
	}
	else
		JPOUT &= ~LEFT_PWM_PIN;
	if (ren==ON)
	{
#ifdef PDM
		rpq+=rrate;
		if (rpq>=PWM_PERIOD)
#else
		if (pwm<=rrate)
#endif
		{
#ifdef PDM
			rpq-=PWM_PERIOD;
#endif
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
	if (leftEnable==OFF && rightEnable==OFF)
		MOTOR_TIMER=0;
	else
		MOTOR_TIMER=MOTOR_TIME;
	len=leftEnable;
	ren=rightEnable;
	ldir=leftDir;
	rdir=rightDir;
	lrate=leftAmt;
	rrate=rightAmt;
#ifndef PDM
	pwm=0;
#endif
	//pulseDrive();
}
void differentialDriveCommand(int value)
{
	switch(value)
	{
#ifdef FLIP
		case 7:
			setDrive(OFF,0,REVERSE,ON,RIGHTPOWER,FORWARD);
			break;
		case 8:
			setDrive(ON,LEFTPOWER,FORWARD,ON,RIGHTPOWER,FORWARD);
			break;
		case 9:
			setDrive(ON,LEFTPOWER,FORWARD,OFF,0,REVERSE);
			break;
		case 1:
			setDrive(OFF,0,FORWARD,ON,RIGHTPOWER,REVERSE);
			break;
		case 2:
			setDrive(ON,LEFTPOWER,REVERSE,ON,RIGHTPOWER,REVERSE);
			break;
		case 3:
			setDrive(ON,LEFTPOWER,REVERSE,OFF,0,FORWARD);
			break;
#else
		case 1:
			setDrive(OFF,0,REVERSE,ON,RIGHTPOWER,FORWARD);
			break;
		case 2:
			setDrive(ON,LEFTPOWER,FORWARD,ON,RIGHTPOWER,FORWARD);
			break;
		case 3:
			setDrive(ON,LEFTPOWER,FORWARD,OFF,0,REVERSE);
			break;
		case 7:
			setDrive(OFF,0,FORWARD,ON,RIGHTPOWER,REVERSE);
			break;
		case 8:
			setDrive(ON,LEFTPOWER,REVERSE,ON,RIGHTPOWER,REVERSE);
			break;
		case 9:
			setDrive(ON,LEFTPOWER,REVERSE,OFF,0,FORWARD);
			break;
#endif
		case 6:
			setDrive(ON,LEFTPOWER,FORWARD,ON,RIGHTPOWER,REVERSE);
			break;
		case 4:
			setDrive(ON,LEFTPOWER,REVERSE,ON,RIGHTPOWER,FORWARD);
			break;
		case 5:
		case 0:
		default:
			setDrive(OFF,0,FORWARD,OFF,0,FORWARD);
			break;
	}

}
void driveCommand(int value)
{
	switch(value)
	{
#ifdef FLIP
		case 7:
			setDrive(OFF,0,REVERSE,ON,POWER,FORWARD);
			break;
		case 8:
			setDrive(ON,POWER,FORWARD,ON,POWER,FORWARD);
			break;
		case 9:
			setDrive(ON,POWER,FORWARD,OFF,0,REVERSE);
			break;
		case 1:
			setDrive(OFF,0,FORWARD,ON,REVPOWER,REVERSE);
			break;
		case 2:
			setDrive(ON,POWER,REVERSE,ON,POWER,REVERSE);
			break;
		case 3:
			setDrive(ON,REVPOWER,REVERSE,OFF,0,FORWARD);
			break;
#else
		case 1:
			setDrive(OFF,0,REVERSE,ON,POWER,FORWARD);
			break;
		case 2:
			setDrive(ON,POWER,FORWARD,ON,POWER,FORWARD);
			break;
		case 3:
			setDrive(ON,POWER,FORWARD,OFF,0,REVERSE);
			break;
		case 7:
			setDrive(OFF,0,FORWARD,ON,REVPOWER,REVERSE);
			break;
		case 8:
			setDrive(ON,POWER,REVERSE,ON,POWER,REVERSE);
			break;
		case 9:
			setDrive(ON,REVPOWER,REVERSE,OFF,0,FORWARD);
			break;
#endif
		case 6:
			setDrive(ON,REVPOWER,FORWARD,ON,REVPOWER,REVERSE);
			break;
		case 4:
			setDrive(ON,REVPOWER,REVERSE,ON,REVPOWER,FORWARD);
			break;
		case 5:
		case 0:
		default:
			setDrive(OFF,0,FORWARD,OFF,0,FORWARD);
			break;
	}
}
void handleCommand(int value, char command_type)
{
	P1OUT ^= RED_LED;
	switch(command_type)
	{
		case 'j':
			driveCommand(value);
			break;
		case 'q':
			if (value>127) value=127;
			REVPOWER=value*2;
			break;
		case 'd':
			if (value>127) value=127;
			POWER=value*2;
			break;
		case 'z':
			if (value>127) value=127;
			LEFTPOWER=value*2;
			break;
		case 'x':
			if (value>127) value=127;
			RIGHTPOWER=value*2;
			break;
		case 's':
			differentialDriveCommand(value);
			break;
		case 'l':
			if (value)
				P1OUT |= RED_LED;
			else
				P1OUT &= ~RED_LED;
			break;
		case '/':
			timer=value;
			break;
#ifdef SERVO
		case 'c':
			servoLoc[0]=value;
			break;			
		case 'v':
			servoLoc[1]=value;
			break;
#endif
		default:
			break;
	}
}
void readCommand()
{
	int value=0;
	char command_type='\0';
	char c;
	int len=0;
	if (commands_waiting)
	{
		while (!usci0.empty()) {
			c=usci0.recv();
			if (c>='0' && c<='9')
			{
				value*=10;
				value+=(c-'0');
				len++;
			}
			else if (c=='\n')
			{
				commands_waiting--;
				if (len>0 && command_type != '\0') handleCommand(value,command_type);
				break;
			}
			else
				command_type=c;
		}
	}
}
//int pulseTicker=0;
interrupt(TIMER0_A0_VECTOR) Timer0_A0 (void) // pwm timer
{
	doDrive=1;
        CCTL0 &= ~CCIFG;
}
#ifdef SERVO
int doServos = 0;
interrupt(WDT_VECTOR) WDT_ISR(void)
{
//	__bic_SR_register_on_exit(LPM0_bits);     // Exit LPM0
	CCTL0 &= ~CCIFG;
	doServos=1;
}
int state = 0;
int curServo = 0;
void handleServos(void)
{
	if (doServos==0) return;
	doServos=0;
	if((state % 64 == 0))
	{
		curServo = state >> 6;
		if (curServo<servoCount)
		{
			TA1CCR0 = (servoLoc[curServo]*(2000/256)) + 2000;
			TA1CTL = TASSEL_2 + ID_3 + MC_1 + TAIE + TACLR + OUT;
			TA1CCTL0 |= CCIE; 
			P1OUT |= servoPin[curServo];
		}
	}
	state++;
	if (state>=(40<<4)) state=0;
}
interrupt(TIMER1_A0_VECTOR) CCR0_ISR(void) 
{
	TA1CTL = MC_0;                             // timer A off, interrupt disabled 
	if (curServo<servoCount) P1OUT &= ~(servoPin[curServo]);
	CCTL0 &= ~CCIFG;
}
 
#endif
int main(void) {
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT

#ifdef SERVO
	int i;
	WDTCTL = WDT_MDLY_0_5;                     // WDT ~0.5ms interval timer
	IE1 |= WDTIE;                             // Enable WDT interrupt
	for (i=0;i<servoCount;i++) {
		P1DIR |= servoPin[i];
		servoLoc[i]=127;
	}
#endif

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

	TA0CCR0 = PWM_CLOCK; // PWM clock - fast as possible
	TA0CCTL0 = 0x10; // interrupt enable
	TA0CTL = TASSEL_2 + MC_1; // system clock (16MHz), count UP
	__bis_SR_register(GIE);
	while(true)
	{
//		__bis_SR_register(GIE+LPM0_bits); // interrupts enabled
#ifdef SERVO
		handleServos();
#endif
		readCommand();
		pulseDrive();
	}
//	__bis_SR_register(GIE+LPM0_bits); // interrupts enabled
	return 0;
}


