#ifndef rxr_macros_h
#define rxr_macros_h

#include "Arduino.h"
#include "constants.h"

// pin macros
#define CLR(x,y)            ( PORT ## x&=(~(1<<y)) )
#define SET(x,y)            ( PORT ## x|=(1<<y) )
#define IN(x,y)             ( DDR ## x&=(~(1<<y)) )
#define OUT(x,y)            ( DDR ## x|=(1<<y) )
#define SET_MODE(pin,mode)  ( pin(mode) )

// antena pins
#define ANT_CTRL1(verb)     ( verb(F,0) )
#define ANT_CTRL2(verb)     ( verb(F,1) )

// dcdrv pins
#define DCSPEED_PIN(verb)   ( verb(B,5) )
#define DCIN1_PIN(verb)     ( verb(D,6) )
#define DCIN2_PIN(verb)     ( verb(D,7) )
#define ENCA_PIN(verb)      ( verb(D,0) )
#define ENCB_PIN(verb)      ( verb(D,1) )

#endif // rxr_macros_h
