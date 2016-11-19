
#ifndef settings_h
#define settings_h

#include "eeprom_helpers.h"

// EEPROM locations for parameters
#define MAX_VEL_LOC       0  // long
#define ACCEL_LOC         4  // long
#define CHANNEL_LOC       12 // int
#define ZMODE_MAX_VEL_LOC 18 // long
#define ZMODE_ACCEL_LOC   22 // long

inline void set_int_16(int loc, int val)
{
    eeprom_write_int16(loc, val);
}

inline int get_int_16(int loc) {
    return eeprom_read_int16(loc);
}

inline void set_int_32(int loc, long val)
{
    eeprom_write_int32(loc, val);
}

inline long get_int_32(int loc) {
    return eeprom_read_int32(loc);
}

inline int settings_get_channel()
{
    return get_int_16(CHANNEL_LOC);
}
inline long settings_get_max_velocity()
{
    return get_int_32(MAX_VEL_LOC);
}
inline long settings_get_accel()
{
    return get_int_32(ACCEL_LOC);
}
inline long settings_get_z_max_velocity()
{
    return get_int_32(ZMODE_MAX_VEL_LOC);
}
inline long settings_get_z_accel()
{
    return get_int_32(ZMODE_ACCEL_LOC);
}

inline void settings_set_channel(int val)
{
    return set_int_16(CHANNEL_LOC, val);
}
inline void settings_set_max_velocity(long val)
{
    return set_int_32(MAX_VEL_LOC, val);
}
inline void settings_set_accel(long val)
{
    return set_int_32(ACCEL_LOC, val);
}
inline void settings_set_z_max_velocity(long val)
{
    return set_int_32(ZMODE_MAX_VEL_LOC, val);
}
inline void settings_set_z_accel(long val)
{
    return set_int_32(ZMODE_ACCEL_LOC, val);
}
#endif
