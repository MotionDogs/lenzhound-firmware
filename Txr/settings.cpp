#include "Arduino.h"
#include "settings.h"

static settings_state_t settings_state;

int _settings_position(int preset, int offset, int size)
{
    // 2*0+0*9 2*0+1*9 2*9+0*11  2*9+1*11
    // |   a   |   a   |    b    |    b    |
    return PROFILE_SETTINGS_START + (MAX_PROFILES * offset) + (preset * size);
}

int _settings_position(int offset, int size)
{
    int preset = settings_get_preset_index();
    return _settings_position(preset, offset, size);
}


char settings_get_preset_index()
{
    char preset_index = eeprom_read_char(PRESET_INDEX_OFFSET);
    if (preset_index < 0) {
        return 0;
    } else if (preset_index > 3) {
        return 3;
    } else {
        return preset_index;
    }
}

void settings_set_preset_index(char index)
{
    settings_flush_debounced_values();
    eeprom_write_char(PRESET_INDEX_OFFSET, index);
    settings_state.debounced_max_speed =
        eeprom_read_uint16(
            _settings_position(index, MAX_SPEED_OFFSET, MAX_SPEED_SIZE));
    settings_state.debounced_max_accel =
        eeprom_read_int16(
            _settings_position(index, MAX_ACCEL_OFFSET, MAX_ACCEL_SIZE));
}

void settings_init()
{
    unsigned long sentinel = eeprom_read_uint32(SENTINEL_LOC);
    if (sentinel != SENTINEL_VALUE) {
        for (int i = 0; i < MAX_PROFILES; ++i) {
            eeprom_write_int16(
                _settings_position(i,CHANNEL_OFFSET, CHANNEL_SIZE),
                DEFAULT_CHANNEL);
            eeprom_write_int16(
                _settings_position(i,CAL_POS_1_OFFSET, CAL_POS_1_SIZE),
                DEFAULT_CAL_POS_1);
            eeprom_write_int16(
                _settings_position(i,CAL_POS_2_OFFSET, CAL_POS_2_SIZE),
                DEFAULT_CAL_POS_2);

            int saved_position_offset =
                _settings_position(i,SAVED_POSITION_OFFSET,SAVED_POSITION_SIZE);

            eeprom_write_int16(
                saved_position_offset + 0,
                DEFAULT_SAVED_POSITION);
            eeprom_write_int16(
                saved_position_offset + 2,
                DEFAULT_SAVED_POSITION);
            eeprom_write_int16(
                saved_position_offset + 4,
                DEFAULT_SAVED_POSITION);
            eeprom_write_int16(
                saved_position_offset + 6,
                DEFAULT_SAVED_POSITION);
            eeprom_write_int16(
                _settings_position(i,START_IN_CAL_OFFSET, START_IN_CAL_SIZE),
                DEFAULT_START_IN_CAL);
            eeprom_write_uint16(
                _settings_position(i,MAX_SPEED_OFFSET, MAX_SPEED_SIZE),
                DEFAULT_MAX_SPEED);
            eeprom_write_int16(
                _settings_position(i,MAX_ACCEL_OFFSET, MAX_ACCEL_SIZE),
                DEFAULT_MAX_ACCEL);
            eeprom_write_uint32(
                _settings_position(i,ID_OFFSET, ID_SIZE),
                DEFAULT_ID_SEED + (long)i);
            eeprom_write_string(
                _settings_position(i,NAME_OFFSET, NAME_SIZE),
                DEFAULT_NAME, NAME_MAX_LENGTH);
        }

        eeprom_write_char(PRESET_INDEX_OFFSET, 0);
        eeprom_write_uint32(SENTINEL_LOC, SENTINEL_VALUE);
    }

    char preset = settings_get_preset_index();
    settings_state.debounced_max_speed =
        eeprom_read_uint16(_settings_position(preset,
            MAX_SPEED_OFFSET, MAX_SPEED_SIZE));
    settings_state.debounced_max_accel =
        eeprom_read_int16(_settings_position(preset,
            MAX_ACCEL_OFFSET, MAX_ACCEL_SIZE));
}

void settings_flush_debounced_values()
{
    unsigned int max_speed = settings_state.debounced_max_speed;
    unsigned int max_accel = settings_state.debounced_max_accel;
    unsigned int existing_max_speed = 
        eeprom_read_uint16(_settings_position(MAX_SPEED_OFFSET, MAX_SPEED_SIZE));
    unsigned int existing_max_accel = 
        eeprom_read_int16(_settings_position(MAX_ACCEL_OFFSET, MAX_ACCEL_SIZE));

    if (existing_max_speed != max_speed) {
        eeprom_write_uint16(
            _settings_position(MAX_SPEED_OFFSET, MAX_SPEED_SIZE), max_speed);   
    }
    if (existing_max_accel != max_accel) {
        eeprom_write_int16(
            _settings_position(MAX_ACCEL_OFFSET, MAX_ACCEL_SIZE), max_accel);   
    }
}

unsigned long settings_get_id()
{
    return eeprom_read_uint32(_settings_position(ID_OFFSET, ID_SIZE));
}

void settings_get_name(char* buffer)
{
    eeprom_read_string(
        _settings_position(NAME_OFFSET, NAME_SIZE), buffer, NAME_MAX_LENGTH);
}

int settings_get_channel()
{
    return eeprom_read_int16(_settings_position(CHANNEL_OFFSET, CHANNEL_SIZE));
}

int settings_get_calibration_position_1()
{
    return eeprom_read_int16(CAL_POS_1_OFFSET);
}

int settings_get_calibration_position_2()
{
    return eeprom_read_int16(CAL_POS_2_OFFSET);
}

bool settings_get_start_in_calibration_mode()
{
    return (bool)eeprom_read_int16(START_IN_CAL_OFFSET);
}

int settings_get_saved_position(int index)
{
    return eeprom_read_int16(SAVED_POSITION_OFFSET + (index * sizeof(int)));
}

unsigned int settings_get_max_speed()
{
    return settings_state.debounced_max_speed;
}

int settings_get_max_accel()
{
    return settings_state.debounced_max_accel;
}


void settings_set_id(unsigned long val)
{
    eeprom_write_uint32(_settings_position(ID_OFFSET, ID_SIZE), val);
}

void settings_set_name(char* val)
{
    eeprom_write_string(_settings_position(NAME_OFFSET, NAME_SIZE),
        val, NAME_MAX_LENGTH);
}

void settings_set_channel(int val)
{
    eeprom_write_int16(_settings_position(CHANNEL_OFFSET, CHANNEL_SIZE), val);
}

void settings_set_calibration_position_1(int val)
{
    eeprom_write_int16(CAL_POS_1_OFFSET, val);
}

void settings_set_calibration_position_2(int val)
{
    eeprom_write_int16(CAL_POS_2_OFFSET, val);
}

void settings_set_start_in_calibration_mode(bool val)
{
    eeprom_write_int16(START_IN_CAL_OFFSET, val);
}

void settings_set_saved_position(int index, int val)
{
    eeprom_write_int16(SAVED_POSITION_OFFSET + (index * sizeof(int)), val);
}

void settings_set_max_speed(unsigned int val)
{
    settings_state.debounced_max_speed = val;
}

void settings_set_max_accel(int val)
{
    settings_state.debounced_max_accel = val;
}