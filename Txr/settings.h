#ifndef settings_h
#define settings_h

#include "eeprom_helpers.h"

#define SENTINEL_LOC            00
#define SENTINEL_VALUE          0xfafbul

#define PROFILE_SETTINGS_START  128

#define OLD_SETTINGS_END		32

#define PRESET_INDEX_OFFSET     32

#define CAL_POS_1_OFFSET        34 // int16
#define CAL_POS_1_SIZE          2  // int16

#define CAL_POS_2_OFFSET        36 // int16
#define CAL_POS_2_SIZE          2  // int16

#define START_IN_CAL_OFFSET     38 // bool16
#define START_IN_CAL_SIZE       2  // bool16

#define SAVED_POSITION_OFFSET   40 // int16[4]
#define SAVED_POSITION_SIZE     8  // int16[4]

#define CHANNEL_OFFSET			48 // int16
#define CHANNEL_SIZE			2  // int16

#define MAX_PROFILES            6

// EEPROM locations for parameters
#define ID_OFFSET               0  // uint32
#define ID_SIZE                 4  // uint32

#define MAX_SPEED_OFFSET        20 // uint16
#define MAX_SPEED_SIZE          2  // uint16

#define MAX_ACCEL_OFFSET        22 // int16
#define MAX_ACCEL_SIZE          2  // int16

#define NAME_OFFSET             24 // char[42]
#define NAME_SIZE               42 // char[42]

#define CURRENT_LEVEL_OFFSET    66 // uint16
#define CURRENT_LEVEL_SIZE      2  // uint16

#define NAME_MAX_LENGTH         41 // 40 + NULL terminator

#define DEFAULT_CHANNEL         1
#define DEFAULT_DATA_RATE       0
#define DEFAULT_CAL_POS_1       0
#define DEFAULT_CAL_POS_2       800
#define DEFAULT_SAVED_POSITION  0
#define DEFAULT_START_IN_CAL    0
#define DEFAULT_MAX_SPEED       16384
#define DEFAULT_MAX_ACCEL       32
#define DEFAULT_CURRENT_LEVEL   0
#define DEFAULT_NAME            ""

#define DEFAULT_ID_SEED         0xfaceul

#define ENCODER_STEPS_PER_CLICK 4

struct settings_state_t {
    unsigned int debounced_max_speed;
    int debounced_max_accel;
};

void settings_init();
void settings_flush_debounced_values();
void settings_reset_to_defaults();

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
int settings_get_current_level();


void settings_set_id(unsigned long val);
void settings_set_name(char* val);
void settings_set_channel(int val);
void settings_set_calibration_position_1(int val);
void settings_set_calibration_position_2(int val);
void settings_set_start_in_calibration_mode(bool val);
void settings_set_saved_position(int index, int val);
void settings_set_max_speed(unsigned int val);
void settings_set_max_accel(int val);
void settings_set_current_level(unsigned int val);

int settings_process_accel_in(int val);
int settings_process_accel_out(int val);

#endif
