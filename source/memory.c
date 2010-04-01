#include <inttypes.h>
#include <stdlib.h>
#include <avr/eeprom.h>

#include "memory.h"

uint8_t eepromMemory[] EEMEM = { 0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25,
		27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59,
		61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95,
		97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125,
		127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153,
		155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 179, 181, 183,
		185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211,
		213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233, 235, 27, 239,
		241, 243, 245, 247, 249, 251, 253, 255} ;

void writeReading(uint8_t position, uint8_t value) {
	eeprom_write_byte(eepromMemory+position, value);
}

//get a readout value
uint8_t getReadout(uint8_t position) {
	return eeprom_read_byte(eepromMemory+position);
}
