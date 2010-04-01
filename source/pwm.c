#include <inttypes.h>
#include <avr/io.h>      
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>      
#include <util/delay.h>

#include "pins.h"
#include "memory.h"
#include "status.h"
#include "pwm.h"

//the calulated next playback level
volatile uint8_t nextLevel;
volatile uint8_t nextStepLength;
volatile int8_t nextDirection;
//the playback position
static uint8_t currentReplayPos;

//PWM calculation:
//we want 1 story per minute
//this means that the major steps must be performed at 60/128hz = 0.47hz
//this means that the minor steps must be perofmed at 0.47*256hv = 120hz
//this means that the pwm must tick at least tick at 120hz*256hz = 30720hz
//this means the minimume prescaling freq(@8Mhz) is clck/260
const uint8_t pwmPrescaler = (0<<CS02) | (1<<CS01) | (1<<CS00); //clk/256
volatile uint8_t lastPlaybackPos;

void startPwmLoop(uint8_t lastMeasurementPos) {
	currentReplayPos = lastMeasurementPos;	
	//we play back to one position before we begun
	lastPlaybackPos = lastMeasurementPos;
	if (lastPlaybackPos) {
		lastPlaybackPos--;
	} else {
		lastPlaybackPos=SAMPLES-1;
	}
	//we need the pwm timer to be started
	setStartPWM();
	//the first calculation is done inline to have the values
	performCalculation();
	setPlayingBack();
}

void startPwm() {

	power_timer0_enable();

	//
	// Timer 0 einstellen
	//  
	// Modus 14:
	//    FastPWM on compare mode
	//
	//     WGM02   WGM01    WGM00 (fast PWM)
	//     0       1        1
	//
	//
	//  Steuerung des Ausgangsport: Non inverted pwm
	//     COM0A1   COM0A0
	//       1        0

	TCCR0A = (1<<COM0B1) | (0 << COM0B0) | (1<<WGM01) | (1<<WGM00);
	TCCR0B = (0<<WGM02) | pwmPrescaler;

	//
	//  den Endwert (TOP) für den Zähler setzen
	//  der Zähler zählt bis zu diesem Wert
	//we allways count to 256

	//enable interrupt
	TIMSK |= _BV(TOIE0);

	OCR0A = 0;	
}

void stopPwm() {
	OCR0B = 0;
	//disable the counter clock
	//     CS02     CS01    CS00
	//       0        0       0
	//  Steuerung des Ausgangsport: No Output at all
	//     COM0A1   COM0A0
	//       0        1	(prev value)
	TCCR0A &= (uint8_t)~(_BV(COM0B1));
	TCCR0B &= (uint8_t)~(_BV(CS02) | _BV(CS01) | _BV(CS00));

	TIMSK &= ~ _BV(TOIE0);

	power_timer0_disable();

	PORTB = 0;

}

//the interupt variables are saved here
//globally to store them in registers
register uint8_t subStep asm ("r2");
register uint8_t localStepLength asm("r3");
register uint8_t localLevel asm("r4");
register uint8_t nextStep asm ("r5");
register int8_t direction asm ("r6");


ISR(TIMER0_OVF_vect)
{

	//halt the timer to reconfigure	
	GTCCR = _BV(TSM) | _BV(PSR0);
	
	if (currentReplayPos!=lastPlaybackPos) {

		if (subStep==0) {
			//go to the next position
			currentReplayPos++;
			if (currentReplayPos == SAMPLES) {
				currentReplayPos=0;
			}
			localStepLength= nextStepLength;
			nextStep = localStepLength;
			localLevel = nextLevel;
			direction = nextDirection;
			setCalculating();		
		} else {
			//do nextSubStep
			if (subStep==nextStep) {
				localLevel += direction;
				nextStep += localStepLength;
			}
		}
	} else {
		//we have reached a a full circle
		stopPwm();
		currentReplayPos = 0;
		//stop replaying
		clearPlayingBack();
	}
	subStep++;
	//set the value to the timer
	OCR0B=localLevel;
	//OCR0B=currentReplayPos;

	//and reset & run!	
	GTCCR = 0;
}

void performCalculation(void) {

	//ATTENTION: we perform the NEXT calculation
	static uint8_t previous;
	//is this the first calculation
	if (isPWMToBeStarted()) {
		//we want a soft start from dark
		previous = 0;
	}
	//we assume that we reached the final value
	//but get sure of that!
	nextLevel = previous;
	uint8_t next=getReadout(currentReplayPos);
	if (next>previous) {
		nextDirection=1;		
		nextStepLength = 255 / (next-previous);
	} else if (previous>next) {
		nextDirection=-1;		
		nextStepLength = 255 / (previous-next);
	} else {
		nextDirection=0;
		nextStepLength=0;
	}
	previous = next;
	clearCalculating();
}

