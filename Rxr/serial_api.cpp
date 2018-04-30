#include "config.h"
#include "serial_api.h"
#include "radio.h"
#include "settings.h"
#include "eeprom_helpers.h"
#include "controller.h"
#include "util.h"
#include "Arduino.h"

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
    case (SERIAL_SAVE_CONFIG): {
        PACKET_SEND_EMPTY(PACKET_SAVE_CONFIG);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_PRESET_INDEX_GET): {
        PACKET_SEND_EMPTY(PACKET_PRESET_INDEX_GET);
    } break;
    case (SERIAL_PRESET_INDEX_SET): {
        PACKET_SEND(PACKET_PRESET_INDEX_SET, preset_index_set, _parse_i16(in));
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_ID_GET): {
        PACKET_SEND_EMPTY(PACKET_PROFILE_ID_GET);
    } break;
    case (SERIAL_ID_SET): {
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_NAME_GET): {
        PACKET_SEND_EMPTY(PACKET_PROFILE_NAME_GET);
    } break;
    case (SERIAL_NAME_SET): {
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_CHANNEL_GET): {
        _print_i16(cmd, settings_get_channel());
    } break;
    case (SERIAL_CHANNEL_SET): {
        unsigned int channel = _parse_u16(in);
        settings_set_channel(channel);
        radio_set_channel(channel);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_REMOTE_CHANNEL_GET): {
        PACKET_SEND_EMPTY(PACKET_CHANNEL_GET);
    } break;
    case (SERIAL_REMOTE_CHANNEL_SET): {
        int channel = _parse_i16(in);
        PACKET_SEND(PACKET_CHANNEL_SET, channel_set, channel);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_START_STATE_GET): {
        PACKET_SEND_EMPTY(PACKET_START_STATE_GET);
    } break;
    case (SERIAL_START_STATE_SET): {
        int start_state = _parse_i16(in);
        PACKET_SEND(PACKET_START_STATE_SET, start_state_set, start_state);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_MAX_SPEED_GET): {
        unsigned int max_vel = controller_get_speed();
        _print_i16(cmd, max_vel);
    } break;
    case (SERIAL_MAX_SPEED_SET): {
        unsigned int max_vel = _parse_u16(in);
        controller_set_speed((long)max_vel);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_ACCEL_GET): {
        unsigned int accel = controller_get_accel();
        _print_i16(cmd, accel);
    } break;
    case (SERIAL_ACCEL_SET): {
        unsigned int accel = _parse_u16(in);
        controller_set_accel((long)accel);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_POT_GET): {
    } break;
    case (SERIAL_ENCODER_GET): {
    } break;
    case (SERIAL_LEDS): {
    } break;
    case (SERIAL_RELOAD_CONFIG): {
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_TARGET_POSITION_GET): {
        long position = controller_get_target_position();
        _print_i16(cmd, fixed_to_i16(position));
    } break;
    case (SERIAL_TARGET_POSITION_SET): {
        long position = i16_to_fixed(_parse_i16(in));
        if (controller_is_position_initialized()) {
            controller_move_to_position(position);
        } else {
            controller_initialize_position(position);
        }
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_CURRENT_LEVEL_GET): {
        unsigned int current_level = controller_get_current_level();
        _print_u16(cmd, current_level);
    } break;
    case (SERIAL_CURRENT_LEVEL_SET): {
        unsigned int current_level = _parse_u16(in);
        controller_set_current_level(current_level);
        _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_MOTOR_DRIVER_GET): {
      unsigned int motor_driver = controller_get_motor_driver();
      _print_u16(cmd, motor_driver);
    } break;
    case (SERIAL_MOTOR_DRIVER_SET): {
      unsigned int motor_driver = _parse_u16(in);
      controller_set_motor_driver(motor_driver);
      _serial_api_print_ok(cmd);
    } break;
    case (SERIAL_FACTORY_RESET): {
        settings_reset_to_defaults();
        _serial_api_print_ok(cmd);
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
