
#ifndef settings_h
#define settings_h

#include "eeprom_helpers.h"

// EEPROM locations for parameters
#define DEFAULT_CHANNEL   1
#define DEFAULT_CURRENT_LEVEL   0   // int
#define DEFAULT_MOTOR_DRIVER   1  // int
#define CHANNEL_LOC       32  // int
#define CURRENT_LEVEL_LOC 34  // int
#define MOTOR_DRIVER_LOC  36  // int
#define SENTINEL_LOC      128 // int
#define SENTINEL_VALUE    0xfafbul

inline int settings_reset_to_defaults()
{
    eeprom_write_uint16(CURRENT_LEVEL_LOC, DEFAULT_CURRENT_LEVEL);
    eeprom_write_uint16(MOTOR_DRIVER_LOC, DEFAULT_MOTOR_DRIVER);
    eeprom_write_uint16(SENTINEL_LOC, SENTINEL_VALUE);
    eeprom_write_int16(CHANNEL_LOC, DEFAULT_CHANNEL);
}

inline int settings_get_channel()
{
    if (eeprom_read_uint16(SENTINEL_LOC) != SENTINEL_VALUE) {
        settings_reset_to_defaults();
        return DEFAULT_CHANNEL;
    }
    return eeprom_read_int16(CHANNEL_LOC);
}

inline void settings_set_channel(int val)
{
    return eeprom_write_int16(CHANNEL_LOC, val);
}

#endif
