#ifndef settings_h
#define settings_h

#include "eepromImpl.h"

// EEPROM locations for parameters
#define CHANNEL_LOC       12 // int
#define DATA_RATE_LOC     16 // int
#define CAL_POS_1_LOC     18 // int
#define CAL_POS_2_LOC     20 // int
#define SAVED_POS_ARR_LOC 22 // int[4]
#define START_IN_CAL_LOC  30 // boolean


inline void set_int_16(int loc, int val)
{
    eeprom::WriteInt16(loc, val);
}

inline int get_int_16(int loc) {
    int val;
    eeprom::ReadInt16(loc, &val);
    return val;
}

inline int settings_get_channel()
{
    return get_int_16(CHANNEL_LOC);
}
inline int settings_get_calibration_position_1()
{
    return get_int_16(CAL_POS_1_LOC);
}
inline int settings_get_calibration_position_2()
{
    return get_int_16(CAL_POS_2_LOC);
}
inline bool settings_get_start_in_calibration_mode()
{
    return (bool)get_int_16(START_IN_CAL_LOC);
}
inline int settings_get_saved_position(int index)
{
    return get_int_16(SAVED_POS_ARR_LOC + (index * sizeof(int)));
}

inline void settings_set_channel(int val)
{
    set_int_16(CHANNEL_LOC, val);
}
inline void settings_set_calibration_position_1(int val)
{
    set_int_16(CAL_POS_1_LOC, val);
}
inline void settings_set_calibration_position_2(int val)
{
    set_int_16(CAL_POS_2_LOC, val);
}
inline void settings_set_start_in_calibration_mode(bool val)
{
    set_int_16(START_IN_CAL_LOC, val);
}
inline void settings_set_saved_position(int index, int val)
{
    set_int_16(SAVED_POS_ARR_LOC + (index * 2), val);
}

#endif
