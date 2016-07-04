#include "version.h"
#include "serial_api.h"
#include "radio.h"
#include "util.h"
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

inline void _serial_api_end(serial_api_state_t *state, const char *in)
{
    _serial_api_print(state, in);
    _serial_api_print(state, "\n");
}

inline void _serial_api_end_len(serial_api_state_t *state, char *in, int len)
{
    _serial_api_print_len(state, in, len);
    _serial_api_print(state, "\n");
}

inline void _serial_api_reset_in_buffer(serial_api_state_t *state)
{
    state->in_index = 0;
    state->escaped = 0;
}


void _print_val(serial_api_state_t *state, char type, int val)
{
    char buffer[16];
    sprintf(buffer, "%c=%d", type, val);
    _serial_api_end(state, buffer);
}

void _serial_api_process_command(serial_api_state_t *state, int length)
{
    char *in = _serial_api_in(state, 0);
    char cmd = *in;

    switch (cmd) {
    case (SERIAL_API_CMD_ECHO): {
        if (length < 3) {
            _serial_api_print(state, "f ");
            _serial_api_end(state, MALFORMED_COMMAND);
        } else {
            _serial_api_print(state, "f ");
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
    case (SERIAL_API_CMD_SET_VALUE): {
        char type;
        long val;
        sscanf(in, "s %c=%ld", &type, &val);
        switch (type) {
            case ('o'): {
                long transformed = i32_to_fixed(val);
                if (state->motor_controller->is_position_initialized()) {
                    state->motor_controller->move_to_position(transformed);
                } else {
                    state->motor_controller->initialize_position(transformed);
                }
            } break;
            case ('v'): {
                state->motor_controller->set_velocity_percent((int)val);
            } break;
            case ('a'): {
                state->motor_controller->set_accel_percent((int)val);
            } break;
            case ('V'): {
                state->motor_controller->set_velocity(val);
            } break;
            case ('A'): {
                state->motor_controller->set_accel(val);
            } break;
        }
    } break;
    default: {
        _serial_api_print(state, "f ");
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
        _serial_api_print(state, "f ");
        _serial_api_end(state, MAX_INPUT_LENGTH_EXCEEDED);
        _serial_api_reset_in_buffer(state);
    }
}

void serial_api_queue_output(serial_api_state_t *state, const char *message)
{
    _serial_api_end(state, message);
}

void serial_api_queue_output_len(serial_api_state_t *state,
                             char *message,
                             int length)
{
    _serial_api_end_len(state, message, length);
}
