#include "version.h"
#include "serial_api.h"
#include "radio.h"
#include "settings.h"
#include "Arduino.h"

inline char *_serial_api_out(serial_api_state_t *state, int index)
{
    return &state->out_buffer[index];
}

inline char *_serial_api_in(serial_api_state_t *state, int index)
{
    return &state->in_buffer[index];
}

inline void _serial_api_print(serial_api_state_t *state, const char *in)
{
    char *out = _serial_api_out(state, state->out_index);

    while (*in) {
        if (state->out_index++ > SERIAL_API_OUT_BUFFER_SIZE) {
            state->out_index = 0;
            out = _serial_api_out(state, 0);
            in = (char *)MAX_RESPONSE_LENGTH_EXCEEDED;
        }
        *(out++) = *(in++);
    }
}

inline void _serial_api_print_len(serial_api_state_t *state,
                                  char *in,
                                  int len)
{
    if (len + state->out_index > SERIAL_API_OUT_BUFFER_SIZE) {
        _serial_api_print(state, MAX_RESPONSE_LENGTH_EXCEEDED);
    } else {
        char *out = _serial_api_out(state, state->out_index);

        for (int i = 0; i < len; i++) {
            *(out++) = *(in++);
        }

        state->out_index += len;
    }
}

inline void _serial_api_end(serial_api_state_t *state,
                            const char *in)
{
    _serial_api_print(state, in);
    _serial_api_print(state, "\n");
}

inline void _serial_api_end_len(serial_api_state_t *state,
                                char *in, int len)
{
    _serial_api_print_len(state, in, len);
    _serial_api_print(state, "\n");
}

inline void _serial_api_reset_in_buffer(serial_api_state_t *state)
{
    state->in_index = 0;
    state->escaped = 0;
}

void _queue_radio_command(serial_api_state_t *state, char type)
{
    radio_packet_t packet;
    packet.type = type;
    radio_queue_message(state->radio_state, packet);
}

void _send_uval(serial_api_state_t *state, unsigned int val, char type)
{
    radio_packet_t packet;
    packet.type = type;
    packet.typed_uval.val = val;
    radio_queue_message(state->radio_state, packet);
}

unsigned int _parse_uval(char* in) {
    unsigned int val;
    sscanf(in + 2, "%u", &val);
    return val;
}

void _parse_and_send_uval(serial_api_state_t *state, char* in, char type)
{
    unsigned int val = _parse_uval(in);
    _send_uval(state, val, type);
}

void _print_val(serial_api_state_t *state, char type, int val)
{
    char buffer[16];
    sprintf(buffer, "%c=%d", type, val);
    _serial_api_end(state, buffer);
}

void _serial_api_process_command(serial_api_state_t *state,
                                 int length)
{
    char *in = _serial_api_in(state, 0);
    char cmd = *in;
    switch (cmd) {
    case (SERIAL_API_CMD_ECHO): {
        if (length < 3) {
            _serial_api_end(state, MALFORMED_COMMAND);
        } else {
            _serial_api_end_len(state, in + 2, length - 2);
        }
    } break;
    case (SERIAL_API_CMD_VERSION): {
        char buffer[16];
        sprintf(buffer, "%c=%d.%d", cmd, VERSION_MAJOR, VERSION_MINOR);
        _serial_api_end(state, buffer);
    } break;
    case (SERIAL_API_CMD_ROLE): {
        _print_val(state, cmd, ROLE);
    } break;
    case (SERIAL_API_CMD_GET_CHANNEL): {
        _print_val(state, cmd, settings_get_channel());
    } break;
    case (SERIAL_API_CMD_GET_MAX_VELOCITY): {
        unsigned int max_vel = state->motor_controller->get_velocity();
        _print_val(state, cmd, max_vel);
    } break;
    case (SERIAL_API_CMD_GET_ACCEL): {
        unsigned int accel = state->motor_controller->get_accel();
        _print_val(state, cmd, accel);
    } break;
    case (SERIAL_API_CMD_REMOTE_VERSION): {
        _queue_radio_command(state, PACKET_GET_VERSION);
    } break;
    case (SERIAL_API_CMD_REMOTE_ROLE): {
        _queue_radio_command(state, PACKET_GET_ROLE);
    } break;
    case (SERIAL_API_CMD_SET_MAX_VELOCITY): {
        unsigned int max_vel = _parse_uval(in);
        state->motor_controller->set_velocity((long)max_vel);
    } break;
    case (SERIAL_API_CMD_SET_ACCEL): {
        unsigned int accel = _parse_uval(in);
        state->motor_controller->set_accel((long)accel);
    } break;
    case (SERIAL_API_CMD_SAVE_CONFIG): {
        unsigned int max_vel = state->motor_controller->get_velocity();
        unsigned int accel = state->motor_controller->get_accel();
        settings_set_max_velocity(max_vel);
        settings_set_accel(accel);
    } break;
    case (SERIAL_API_CMD_SET_CHANNEL): {
        unsigned int channel = _parse_uval(in);
        settings_set_channel(channel);
        radio_set_channel(channel);
    } break;
    case (SERIAL_API_CMD_SET_REMOTE_CHANNEL): {
        unsigned int channel = _parse_uval(in);
        _send_uval(state, channel, PACKET_SET_CHANNEL);
    } break;
    case (SERIAL_API_CMD_GET_REMOTE_CHANNEL): {
        _queue_radio_command(state, PACKET_GET_CHANNEL);
    } break;
    case (SERIAL_API_CMD_GET_START_STATE): {
    } break;
    case (SERIAL_API_CMD_GET_Z_MAX_VELOCITY): {
    } break;
    case (SERIAL_API_CMD_GET_Z_ACCEL): {
    } break;
    case (SERIAL_API_CMD_SET_Z_MAX_VELOCITY): {
    } break;
    case (SERIAL_API_CMD_SET_Z_ACCEL): {
    } break;
    case (SERIAL_API_CMD_GET_POT): {
    } break;
    case (SERIAL_API_CMD_GET_ENCODER): {
    } break;
    case (SERIAL_API_CMD_SET_START_STATE): {
    } break;
    default: {
        _serial_api_end(state, UNKNOWN_COMMAND);
    } break;
    }
}

void _serial_api_inner_queue_byte(serial_api_state_t *state,
                                  char byte,
                                  int index)
{
    char *next = _serial_api_in(state, index);

    if (byte == SERIAL_API_END_OF_COMMAND) {
        *next = 0;
        int len = index;
        _serial_api_process_command(state, len);
        _serial_api_reset_in_buffer(state);
    } else {
        *next = byte;
    }
}

serial_api_response_t serial_api_read_response(serial_api_state_t *state)
{
    serial_api_response_t result;

    result.buffer = _serial_api_out(state, 0);
    result.length = state->out_index;
    state->out_index = 0;
    return result;
}

void serial_api_queue_byte(serial_api_state_t *state, char byte)
{
    int i = state->in_index++;

    if (i < SERIAL_API_IN_BUFFER_SIZE) {
        _serial_api_inner_queue_byte(state, byte, i);
    } else {
        _serial_api_end(state, MAX_INPUT_LENGTH_EXCEEDED);
        _serial_api_reset_in_buffer(state);
    }
}

void serial_api_queue_output(serial_api_state_t *state,
                             const char *message)
{
    _serial_api_end(state, message);
}

void serial_api_queue_output_len(serial_api_state_t *state,
                             char *message,
                             int length)
{
    _serial_api_end_len(state, message, length);
}
