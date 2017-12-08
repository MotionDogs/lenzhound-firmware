
#ifndef settings_h
#define settings_h

#include "eeprom_helpers.h"

// EEPROM locations for parameters
#define DEFAULT_CHANNEL   1
#define DEFAULT_CURRENT_LEVEL   0   // int
#define CHANNEL_LOC       32  // int
#define CURRENT_LEVEL_LOC 34  // int
#define SENTINEL_LOC      128 // int
#define SENTINEL_VALUE    0xfafbul

inline int settings_reset_to_defaults()
{
    eeprom_write_uint16(CURRENT_LEVEL_LOC, DEFAULT_CURRENT_LEVEL);
}

inline int settings_get_channel()
{
    if (settings_reset_to_defaults()) {
        eeprom_write_uint16(SENTINEL_LOC, SENTINEL_VALUE);
        eeprom_write_int16(CHANNEL_LOC, DEFAULT_CHANNEL);
        return DEFAULT_CHANNEL;
    }
    return eeprom_read_int16(CHANNEL_LOC);
}

inline void settings_set_channel(int val)
{
    return eeprom_write_int16(CHANNEL_LOC, val);
}

inline int settings_get_current_level()
{
    if (settings_reset_to_defaults()) {
        eeprom_write_uint16(SENTINEL_LOC, SENTINEL_VALUE);
        eeprom_write_int16(CURRENT_LEVEL_LOC, DEFAULT_CURRENT_LEVEL);
        return DEFAULT_CURRENT_LEVEL;
    }
    return eeprom_read_int16(CURRENT_LEVEL_LOC);
}

#endif
