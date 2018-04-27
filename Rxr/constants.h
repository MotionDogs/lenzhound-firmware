#ifndef lenzhound_constants_h
#define lenzhound_constants_h

const int SERIAL_BAUD = 57600;

// ISR constants
const long SECONDS_TO_MICROSECONDS = 1000000L;
const long ISR_CALLS_PER_SECOND    = 60L;
const long ISR_PERIOD              = SECONDS_TO_MICROSECONDS/ISR_CALLS_PER_SECOND;

// Motor constants
const long MOTOR_SLEEP_THRESHOLD   = ISR_CALLS_PER_SECOND * 5; // five seconds

enum {
  FREE_MODE,
  PLAYBACK_MODE,
  Z_MODE
};

#endif // lenzhound_constants_h
