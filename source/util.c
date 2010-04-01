#include <inttypes.h>
#include <avr/io.h>      
#include <util/delay.h>


//pulse a led for a certain time
void pulseLed(uint8_t pin, uint8_t timeMs) {
	PORTB |= _BV(pin);
	uint8_t wait = timeMs;
	while (wait) {
		_delay_us(980);
		wait--;
	}
	PORTB &= ~ _BV(pin);
}

void delay_ms(uint8_t ms) {
	uint8_t wait = ms;
	while (wait) {
		_delay_us(980);
		wait--;
	}
}