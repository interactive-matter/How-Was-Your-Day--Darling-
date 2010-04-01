//libraries
#include <avr/power.h>  
#include <avr/interrupt.h>  
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/wdt.h>

//local modules
#include "pins.h"
#include "memory.h"
#include "pwm.h"
#include "measurement.h"
#include "status.h"
#include "util.h"

void sleep(void) {

	if (isCalculating() | isPlayingBack() | isMeasurementNeeded()) {
		//let the main loop run
		return;
	} else {
		//ensure that the watchdog is running
		//go completely to sleep
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	}
	sleep_cpu();
}


int main (void) {
	
	wdt_disable();
	wdt_reset();
	
	status = 0;
	sei();
	
	power_all_disable();
	sleep_enable();
	
	//set Port Directions - this never changes
	DDRB = ( _BV(playbackPin) //playback is an output
			| _BV(enableMeasurePin) // enable measure is an output
			| _BV (measurePin)
			// temperature sensor is an input
			//lightsensorpin is an input
		);
	//disable the digital input of the adcs
	//to save some power
	DIDR0 = 3;

	//signalize that we are alive!
	pulseLed(measurePin,1);
	delay_ms(100);
	pulseLed(playbackPin,10);
	delay_ms(250);
	pulseLed(measurePin,1);
	delay_ms(100);
	pulseLed(measurePin,1);
	
	startMeasureLoop();

	//and now enable the interputs
	sei();

	
	while(1) {
		if(isCalculating()) {
			performCalculation();
		}
		if (isMeasurementNeeded()) {
			performMeasurement();
		}
		if (isPWMToBeStarted()) {
			startPwm();
			clearStartPWM();
		}
		sleep();	
	}
}
