#include "config.h"
#include "serial_api.h"
#include "radio.h"
#include "bsp.h"
#include "settings.h"
#include "eeprom_helpers.h"
#include "leds.h"
#include "Arduino.h"

const char SERIAL_API_END_OF_RESPONSE       = '\n';
const char SERIAL_API_END_OF_COMMAND        = '\n';
const char SERIAL_API_ESCAPE                = '\\';
const int SERIAL_API_EEPROM_SCAN_LENGTH     = 16;

const char* MAX_RESPONSE_LENGTH_EXCEEDED    = "ERR 01";
const char* MAX_INPUT_LENGTH_EXCEEDED       = "ERR 02";
const char* UNKNOWN_COMMAND                 = "ERR 03";
const char* MALFORMED_COMMAND               = "ERR 04";

serial_api_state_t serial_api_state = {0};

inline char *_serial_api_out(int index)
{
    return &serial_api_state.out_buffer[index];
}

inline char *_serial_api_in(int index)
{
    return &serial_api_state.in_buffer[index];
}

inline void _serial_api_print(const char *in)
{
    char *out = _serial_api_out(serial_api_state.out_index);

    while (*in) {
        if (serial_api_state.out_index++ > SERIAL_API_OUT_BUFFER_SIZE) {
            serial_api_state.out_index = 0;
            out = _serial_api_out(0);
            in = (char *)MAX_RESPONSE_LENGTH_EXCEEDED;
        }
        *(out++) = *(in++);
    }

}

inline void _serial_api_print_len(char *in, int len)
{
    if (len + serial_api_state.out_index > SERIAL_API_OUT_BUFFER_SIZE) {
        _serial_api_print(MAX_RESPONSE_LENGTH_EXCEEDED);
    } else {
        char *out = _serial_api_out(serial_api_state.out_index);

        for (int i = 0; i < len; i++) {
            *(out++) = *(in++);
        }

        serial_api_state.out_index += len;
    }
}

inline void _serial_api_end(const char *in)
{
    _serial_api_print(in);
    _serial_api_print("\n");
}

inline void _serial_api_print_ok(char type)
{
    char buffer[16];
    sprintf(buffer, "%c OK", type);
    _serial_api_end(buffer);
}

inline void _serial_api_end_len(char *in, int len)
{
    _serial_api_print_len(in, len);
    _serial_api_print("\n");
}

inline void _serial_api_reset_in_buffer()
{
    serial_api_state.in_index = 0;
}

int _parse_i16(char* in) {
    int val;
    sscanf(in + 2, "%d", &val);
    return val;
}

long _parse_i32(char* in) {
    long val;
    sscanf(in + 2, "%ld", &val);
    return val;
}

unsigned int _parse_u16(char* in) {
    unsigned int val;
    sscanf(in + 2, "%u", &val);
    return val;
}

unsigned long _parse_u32(char* in) {
    unsigned long val;
    sscanf(in + 2, "%lu", &val);
    return val;
}

void _print_string(char type, char* str)
{
    char buffer[60];
    sprintf(buffer, "%c=%s", type, str);
    _serial_api_end(buffer);
}

void _print_i16(char type, int val)
{
    char buffer[16];
    sprintf(buffer, "%c=%d", type, val);
    _serial_api_end(buffer);
}

void _print_i32(char type, long val)
{
    char buffer[16];
    sprintf(buffer, "%c=%ld", type, val);
    _serial_api_end(buffer);
}

void _print_u16(char type, unsigned int val)
{
    char buffer[16];
    sprintf(buffer, "%c=%u", type, val);
    _serial_api_end(buffer);
}

void _print_u32(char type, unsigned long val)
{
    char buffer[16];
    sprintf(buffer, "%c=%lu", type, val);
    _serial_api_end(buffer);
}

void _serial_api_process_command(int length)
{
    char *in = _serial_api_in(0);
    char cmd = *in;

    switch (cmd) {
    case (SERIAL_ECHO): {
        if (length < 3) {
            _serial_api_end(MALFORMED_COMMAND);
        } else {
            _serial_api_end_len(in + 2, length - 2);
        }
    } break;
    case (SERIAL_VERSION): {
        char buffer[16];
        sprintf(buffer, "%c=%s", cmd, VERSION);
        _serial_api_end(buffer);
    } break;
    case (SERIAL_ROLE): {
        _print_i16(cmd, ROLE);
    } break;
    case (SERIAL_REMOTE_VERSION): {
        PACKET_SEND_EMPTY(PACKET_VERSION_GET);
    } break;
    case (SERIAL_REMOTE_ROLE): {
        PACKET_SEND_EMPTY(PACKET_ROLE_GET);
    } break;
    case (SERIAL_PRESET_INDEX_GET): {
        _print_i16(cmd, settings_get_preset_index());
    } break;
    case (SERIAL_PRESET_INDEX_SET): {
        int index = _parse_i16(in);
        int old_index = settings_get_preset_index();
        if (index != old_index) {
            settings_set_preset_index(index);
            _serial_api_print_ok(cmd);   
        }
    } break;
    case (SERIAL_ID_GET): {
        _print_u32(cmd, settings_get_id());
    } break;
    case (SERIAL_ID_SET): {
        settings_set_id(_parse_u32(in));
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_NAME_GET): {
        char buffer[NAME_MAX_LENGTH];
        settings_get_name(buffer);
        _print_string(cmd, buffer);
    } break;
    case (SERIAL_NAME_SET): {
        settings_set_name(in + 2);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_CHANNEL_GET): {
        _print_i16(cmd, settings_get_channel());
    } break;
    case (SERIAL_CHANNEL_SET): {
        unsigned int channel = _parse_u16(in);
        settings_set_channel(channel);
        radio_set_channel(channel, false);
    } break;
    case (SERIAL_REMOTE_CHANNEL_GET): {
        PACKET_SEND_EMPTY(PACKET_CHANNEL_GET);
    } break;
    case (SERIAL_START_STATE_GET): {
        _print_i16(cmd, settings_get_start_in_calibration_mode());
    } break;
    case (SERIAL_START_STATE_SET): {
        int mode;
        sscanf(in + 2, "%d", &mode);
        settings_set_start_in_calibration_mode(mode);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_MAX_SPEED_GET): {
        _print_u16(cmd, settings_get_max_speed());
    } break;
    case (SERIAL_MAX_SPEED_SET): {
        settings_set_max_speed(_parse_u16(in));
        PACKET_SEND(PACKET_MAX_SPEED_SET, max_speed_set, _parse_u16(in));
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_ACCEL_GET): {
        _print_i16(cmd, settings_get_max_accel());
    } break;
    case (SERIAL_ACCEL_SET): {
        settings_set_max_accel(_parse_i16(in));
        PACKET_SEND(PACKET_ACCEL_SET, accel_set, _parse_i16(in));
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_SAVE_CONFIG): {
        // NOTE(doug): this is deprecated, but we're leaving an OK for now
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_RELOAD_CONFIG): {
        PACKET_SEND(PACKET_ACCEL_SET, accel_set, settings_get_max_accel());
        PACKET_SEND(PACKET_MAX_SPEED_SET, max_speed_set,
            settings_get_max_speed());

        radio_set_channel(settings_get_channel(), false);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_TARGET_POSITION_GET): {
        PACKET_SEND_EMPTY(PACKET_TARGET_POSITION_GET);
    } break;
    case (SERIAL_TARGET_POSITION_SET): {
        PACKET_SEND(PACKET_TARGET_POSITION_SET,
            target_position_set, _parse_i32(in));
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_EEPROM_EXPORT): {
        int start = _parse_i16(in);
        int length = min(EEPROM_MAX_ADDR - start, SERIAL_API_EEPROM_SCAN_LENGTH);
        length = max(0, length);
        unsigned char byte_buffer[SERIAL_API_EEPROM_SCAN_LENGTH];
        eeprom_read_bytes(start, byte_buffer, length);

        char buffer[SERIAL_API_EEPROM_SCAN_LENGTH * 2 + 1];
        buffer[0] = 0;
        for (int i = 0; i < length; ++i) {
            sprintf(buffer + (i * 2), "%02x", byte_buffer[i]);
        }

        _print_string(cmd, buffer);
    } break;
    case (SERIAL_EEPROM_IMPORT): {
        int start = _parse_i16(in);
        int length;
        char buffer[33];
        sscanf(in, "%*c %*d %d %32s", &length, buffer);
        if (length > 16) {
            _serial_api_end(MALFORMED_COMMAND);
        } else if (length > 0) {
            unsigned char byte_buffer[17];
            for (int i = 0; i < length; ++i) {
                unsigned char byte;
                sscanf(buffer + i * 2, "%02hhx", &byte);
                byte_buffer[i] = byte;
            }
            eeprom_write_bytes(start, byte_buffer, length);

            _serial_api_print_ok(cmd);
        } else {
            _serial_api_print_ok(cmd);
        }
    } break;
    default: {
        _serial_api_end(UNKNOWN_COMMAND);
    } break;
    }
}

void _serial_api_inner_queue_byte(char byte,
                                  int index)
{
    char *next = _serial_api_in(index);

    if (byte == SERIAL_API_END_OF_COMMAND) {
        *next = 0;
        int len = index;
        _serial_api_process_command(len);
        _serial_api_reset_in_buffer();
    } else {
        *next = byte;
    }
}

serial_api_response_t serial_api_read_response()
{
    serial_api_response_t result;

    result.buffer = _serial_api_out(0);
    result.length = serial_api_state.out_index;
    serial_api_state.out_index = 0;

    return result;
}

void serial_api_queue_byte(char byte)
{
    int i = serial_api_state.in_index++;

    if (i < SERIAL_API_IN_BUFFER_SIZE) {
        _serial_api_inner_queue_byte(byte, i);
    } else {
        _serial_api_end(MAX_INPUT_LENGTH_EXCEEDED);
        _serial_api_reset_in_buffer();
    }
}

void serial_api_queue_output(const char *message)
{
    _serial_api_end(message);
}

void serial_api_queue_output_len(char *message,
                                 int length)
{
    _serial_api_end_len(message, length);
}
