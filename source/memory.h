#ifndef MEMORY_H_
#define MEMORY_H_


//numer of samples
#define SAMPLES 128

//allocates the needed memory
void initMemory(void);

//write readout values
//the code assumes that the positions are accessed consecutively
void writeReading(uint8_t position, uint8_t value);

//get a readout value
uint8_t getReadout(uint8_t position);
//this must be called after each readout (at least before the next
void prepareNextReadPage(uint8_t position);

#endif /*MEMORY_H_*/
