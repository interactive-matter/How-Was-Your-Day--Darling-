#ifndef STATUS_H_
#define STATUS_H_

//bitwise status information
//TODO: stored in the unused USI register
register uint8_t status asm("r7");

//our bits
#define PLAYBACKSTATUS 0
#define STARTPWM 1
#define CALCULATING 2
#define MEASURE 3

#define isPlayingBack() (status & _BV(PLAYBACKSTATUS))

#define setPlayingBack() status |= _BV(PLAYBACKSTATUS);

#define clearPlayingBack() status &= ~ _BV(PLAYBACKSTATUS);

#define isPWMToBeStarted() (status & _BV(STARTPWM))

#define setStartPWM() status |= _BV(STARTPWM);

#define clearStartPWM() status &= ~_BV(STARTPWM);

#define isCalculating() (status & _BV(CALCULATING))

#define setCalculating() status |= _BV(CALCULATING);

#define clearCalculating() status &= ~_BV(CALCULATING);

#define isMeasurementNeeded() (status & _BV(MEASURE))

#define setMeasurementNeeded() status |= _BV(MEASURE);

#define clearMeasurementNeeded() status &= ~ _BV(MEASURE);

#endif /*STATUS_H_*/
