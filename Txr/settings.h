#ifndef settings_h
#define settings_h

#include "eeprom_helpers.h"

#define SENTINEL_LOC            00
#define SENTINEL_VALUE          0xfafaul

#define SETTINGS_START          32

#define PRESET_INDEX_OFFSET     8

#define MAX_PROFILES            6

// EEPROM locations for parameters
#define ID_OFFSET               0  // uint32
#define ID_SIZE                 4  // uint32

#define CHANNEL_OFFSET          4  // int16
#define CHANNEL_SIZE            2  // int16

#define CAL_POS_1_OFFSET        6  // int16
#define CAL_POS_1_SIZE          2  // int16

#define CAL_POS_2_OFFSET        8  // int16
#define CAL_POS_2_SIZE          2  // int16

#define SAVED_POSITION_OFFSET   10 // int16[4]
#define SAVED_POSITION_SIZE     8 // int16[4]

#define START_IN_CAL_OFFSET     18 // bool16
#define START_IN_CAL_SIZE       2  // bool16

#define MAX_SPEED_OFFSET        20 // uint16
#define MAX_SPEED_SIZE          2  // uint16

#define MAX_ACCEL_OFFSET        22 // int16
#define MAX_ACCEL_SIZE          2  // int16

#define NAME_OFFSET             24 // char[42]
#define NAME_SIZE               42 // char[42]

#define NAME_MAX_LENGTH         41 // 40 + NULL terminator

#define DEFAULT_CHANNEL         1
#define DEFAULT_DATA_RATE       0
#define DEFAULT_CAL_POS_1       0
#define DEFAULT_CAL_POS_2       400
#define DEFAULT_SAVED_POSITION  0
#define DEFAULT_START_IN_CAL    1
#define DEFAULT_MAX_SPEED       32768
#define DEFAULT_MAX_ACCEL       32
#define DEFAULT_NAME            ""

#define DEFAULT_ID_SEED         0xfaceul

struct settings_state_t {
    unsigned int debounced_max_speed;
    int debounced_max_accel;
};

void settings_init();
void settings_flush_debounced_values();

char settings_get_preset_index();
void settings_set_preset_index(char index);

unsigned long settings_get_id();
void settings_get_name(char* buffer);
int settings_get_channel();
int settings_get_calibration_position_1();
int settings_get_calibration_position_2();
bool settings_get_start_in_calibration_mode();
int settings_get_saved_position(int index);
unsigned int settings_get_max_speed();
int settings_get_max_accel();

void settings_set_id(unsigned long val);
void settings_set_name(char* val);
void settings_set_channel(int val);
void settings_set_calibration_position_1(int val);
void settings_set_calibration_position_2(int val);
void settings_set_start_in_calibration_mode(bool val);
void settings_set_saved_position(int index, int val);
void settings_set_max_speed(unsigned int val);
void settings_set_max_accel(int val);

#endif
