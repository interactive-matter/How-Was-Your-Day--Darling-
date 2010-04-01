#include <inttypes.h>
#include <avr/io.h>      
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>

#include "pins.h"
#include "status.h"
#include "pwm.h"
#include "memory.h"
#include "util.h"

const uint8_t lightVoltageReference = (0<<REFS1) | (0<<REFS0); // use VCC as reference voltag
//this shuold be changed to 1v (independent of batt) 
const uint8_t temperatureVoltageReference = (0<<REFS1) | (0<<REFS0); // use VCC as reference voltage
//how much warmer must it be to trigger
const uint8_t warmTrigger=3;
//how many cycles must the temperature raise?
const uint8_t warmCycles = 2;

//defaults for the measure loop
//we have 128 Samples per 24 Hours
//leading to 5,3333 samples per hour
//leading to 3600s/5,333 = 675 seconds between each sample
//ho many samples do we accumuluate as median (minus one - implementationwise)
uint8_t accumulatedMeasureSamples = 255;
//so 675/64  is ca 11
//this leads to a measure span of 25 hours
//how may seconds do we wait before we grab a sample (minus one - implementationwise)?
//manually calibrated
uint8_t measureWaitSeconds = 8;
//this is needed to get an true 16bit value from the 10bit readout
//while the last two bits are untrustable
//const uint8_t watchdogTiming= (1 << WDP3) | (1<< WDP0); //8s - it is defferent on the 25
//TODO: REcalculate to go back to 1s
const uint8_t watchdogTiming= WDTO_1S;

void startMeasurement(uint8_t channel, uint8_t voltageReference) {
	//switch on adc
	power_adc_enable();

	//enable ADC
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // Frequenzvorteiler 
	// setzen auf 8 (1) und ADC aktivieren (1)

	ADMUX = channel | voltageReference;

	ADCSRA|= _BV(ADEN);//enable ADC with dummy conversion

	//wait till the voltag reference is up
	_delay_us(70);

	//do one dummy measurement
	ADCSRA |= _BV(ADSC); // eine ADC-Wandlung 
	while (ADCSRA & _BV(ADSC)) {
		; // auf Abschluss der Konvertierung warten 
	}
	uint16_t result = ADCW;
	result=0;
}

uint16_t readChannel() {

	uint16_t result=0;

	int loop = 4;
	do {
		ADCSRA |= _BV(ADSC); // eine ADC-Wandlung 
		while (ADCSRA & _BV(ADSC)) {
			; // auf Abschluss der Konvertierung warten 
		}
		result += ADCW;
		loop--;
	} while (loop);

	result = (result >> 2); //dive result by 4

	return result;
}

void finishMeasurement(void) {

	ADCSRA &= (uint8_t)~(_BV(ADEN)); // ADC deaktivieren (2)
	//switch off adc
	power_adc_disable();
}

void startMeasureLoop(void) {

	//prepare the watchog
	WDTCR = _BV(WDCE) | _BV(WDE);

	//
	// Watchdog einstellen
	//  
	// Modus:
	//    Just enable Watchdog interrupt
	//
	//     WDTON (fuse)   WDE    WDIE
	//     0               1       1 
	//

	WDTCR = _BV(WDE) | _BV(WDIE) | watchdogTiming;

}

ISR(WDT_vect)
{
	//reset the watchdog to avoid resets!
	WDTCR |= _BV(WDIE);
	//supress run while playing back
	//would look ugly
	if (!isPlayingBack()) {
		setMeasurementNeeded();
	}
}

void performMeasurement(void) {
	//the wait counter
	static uint8_t measureCounter;
	static uint8_t measureWaitCounter;
	//where to write the value
	static uint8_t measurePos;
	static uint16_t lightReadout;

	//we do not want to get disturbed by interrupts
cli	();

	//provide power to the sensors
	//and switch input pins to highZ

	if (measureWaitCounter==0) {
		pulseLed(measurePin,1);
		//save previous value
		uint16_t previousValue=lightReadout;

		//power up the sensors
		PORTB= _BV(enableMeasurePin);
		//set to read the light according to the lightVoltageReference
		startMeasurement(lightSensorChannel, lightVoltageReference);

		//do an accumulated light readout
		lightReadout += readChannel();
		//switch power off
		PORTB = 0;
		//if we have an overflow
		if (lightReadout<previousValue) {
			//it seems to be sufficient to wait until now
			accumulatedMeasureSamples=accumulatedMeasureSamples-measureCounter-1;
			//we have reached the max - we can write the value
			measureCounter=0;
			//restore the pre overflow value
			lightReadout=previousValue;
			//recalculate the measure wait seconds
			measureWaitSeconds = (uint8_t)((57600/(uint16_t)128)/(uint16_t)accumulatedMeasureSamples);
			//blink an adjustment
			pulseLed(measurePin,1);
		}

		if (measureCounter==0) {
			//signalize we write a value
			PORTB = _BV(measurePin);

			measureCounter=accumulatedMeasureSamples;

			//build average for the last period
			//and write the value
			writeReading(measurePos,(lightReadout >> 8));

			//reset the averagator
			lightReadout = 0;
			//increase the measurement position
			measurePos++;
			//and reset if needed
			if (measurePos==SAMPLES) {
				measurePos=0;
			}
			PORTB = 0;
		} else {
			measureCounter--;
		}
		measureWaitCounter=measureWaitSeconds;
	} else {
		measureWaitCounter--;
	}

	//check temperature 
	static uint16_t previousTemperature;
	static uint8_t raiseCount;

	//switch Sensors on
	PORTB = _BV(enableMeasurePin);
	startMeasurement(temperatureSensorChannel, temperatureVoltageReference);
	uint16_t currentTemperature=readChannel();
	//switch off sensor
	PORTB = 0;
	if (previousTemperature==0) {
		//this is the first run - unelegant!
		previousTemperature=currentTemperature;
	}

	if (previousTemperature-currentTemperature>warmTrigger) {
		raiseCount++;
	} else {
		raiseCount=0;
	}
	if (raiseCount>warmCycles) {
		pulseLed(playbackPin,5);
		startPwmLoop(measurePos);
		raiseCount=0;
	}

	previousTemperature = currentTemperature;

	//switch off sensor
	//we switch anything of sonce there is nothing else going on
	finishMeasurement();

	clearMeasurementNeeded();

	sei();
}


/*
 public void performSanityCheck() {
 //verifies that alll samples are more or less ok
 

 }*/
