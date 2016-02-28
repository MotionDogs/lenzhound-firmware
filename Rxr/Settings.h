
#ifndef Settings_h
#define Settings_h

// EEPROM locations for parameters
#define MAX_VEL_LOC       0  // long
#define ACCEL_LOC         4  // long
#define MICROSTEPS_LOC    8  // int
#define ANTENNA_LOC       10 // int
#define CHANNEL_LOC       12 // int
#define PA_LEVEL_LOC      14 // int
#define ZMODE_MAX_VEL_LOC 18 // long
#define ZMODE_ACCEL_LOC   22 // long


class Settings
{
public:
  Settings();
  void SetMaxVelocity(long val);
  long GetMaxVelocity();
  void SetAcceleration(long val);
  long GetAcceleration();
  void SetMicrosteps(int val);
  int GetMicrosteps();
  void SetAntenna(int val);
  int GetAntenna();
  void SetChannel(int val);
  int GetChannel();
  void SetPALevel(int val);
  int GetPALevel();
  int GetDataRate();
  void SetZModeMaxVelocity(long val);
  long GetZModeMaxVelocity();
  void SetZModeAcceleration(long val);
  long GetZModeAcceleration();


private:
  // any reason to save the state of the settings?
  // or is it a one time read on boot up?

};

#endif


#ifndef settings_h
#define settings_h

#include "eepromImpl.h"

// EEPROM locations for parameters
#define MAX_VEL_LOC       0  // long
#define ACCEL_LOC         4  // long
#define CHANNEL_LOC       12 // int
#define ZMODE_MAX_VEL_LOC 18 // long
#define ZMODE_ACCEL_LOC   22 // long

inline void set_int_16(int loc, int val)
{
    eeprom::WriteInt16(loc, val);
}

inline int get_int_16(int loc) {
    int val;
    eeprom::ReadInt16(loc, &val);
    return val;
}

inline void set_int_32(int loc, long val)
{
    eeprom::WriteInt32(loc, val);
}

inline long get_int_32(int loc) {
    long val;
    eeprom::ReadInt32(loc, &val);
    return val;
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
