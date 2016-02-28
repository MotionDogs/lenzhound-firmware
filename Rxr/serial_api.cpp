#include "version.h"
#include "serial_api.h"
#include "radio.h"
#include "util.h"
#include "Arduino.h"

inline char *_serial_api_out(serial_api_state_t *state, int source, int index)
{
    return &state->out_buffer[source * SERIAL_API_OUT_BUFFER_SIZE + index];
}

inline char *_serial_api_in(serial_api_state_t *state, int source, int index)
{
    return &state->in_buffer[source * SERIAL_API_IN_BUFFER_SIZE + index];
}

inline void _serial_api_print(serial_api_state_t *state, int source,
                              const char *in)
{
    char *out = _serial_api_out(state, source, state->out_indices[source]);

    while (*in) {
        if (state->out_indices[source]++ > SERIAL_API_OUT_BUFFER_SIZE) {
            state->out_indices[source] = 0;
            out = _serial_api_out(state, source, 0);
            in = (char *)MAX_RESPONSE_LENGTH_EXCEEDED;
        }
        *(out++) = *(in++);
    }
}

inline void _serial_api_print_len(serial_api_state_t *state,
                                  int source,
                                  char *in,
                                  int len)
{
    if (len + state->out_indices[source] > SERIAL_API_OUT_BUFFER_SIZE) {
        _serial_api_print(state, source, MAX_RESPONSE_LENGTH_EXCEEDED);
    } else {
        char *out = _serial_api_out(state, source, state->out_indices[source]);

        for (int i = 0; i < len; i++) {
            *(out++) = *(in++);
        }

        state->out_indices[source] += len;
    }
}

inline void _serial_api_end(serial_api_state_t *state, int source,
                            const char *in)
{
    _serial_api_print(state, source, in);
    _serial_api_print(state, source, "\n");
}

inline void _serial_api_end_len(serial_api_state_t *state, int source,
                                char *in, int len)
{
    _serial_api_print_len(state, source, in, len);
    _serial_api_print(state, source, "\n");
}

inline void _serial_api_reset_in_buffer(serial_api_state_t *state, int source)
{
    state->indices[source] = 0;
    state->escaped[source] = 0;
}

void _serial_api_process_command(serial_api_state_t *state, int source,
                                 int length)
{
    char *in = _serial_api_in(state, source, 0);

    switch (*in) {
    case (SERIAL_API_CMD_ECHO): {
        if (length < 3) {
            _serial_api_print(state, source, "f ");
            _serial_api_end(state, source, MALFORMED_COMMAND);
        } else {
            _serial_api_print(state, source, "f ");
            _serial_api_end_len(state, source, in + 2, length - 2);
        }
    } break;
    // case (SERIAL_API_CMD_VERSION): {
    //     _serial_api_print(state, source, "f ");
    //     _serial_api_end(state, source, VERSION);
    // } break;
    // case (SERIAL_API_CMD_ROLE): {
    //     _serial_api_print(state, source, "f ");
    //     _serial_api_end(state, source, ROLE);
    // } break;
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
        _serial_api_print(state, source, "f ");
        _serial_api_end(state, source, UNKNOWN_COMMAND);
    } break;
    }
}

void _serial_api_inner_queue_byte(serial_api_state_t *state,
                                  int source,
                                  char byte,
                                  int index)
{
    char *next = _serial_api_in(state, source, index);

    if (byte == SERIAL_API_END_OF_COMMAND) {
        *next = 0;
        int len = index;
        _serial_api_process_command(state, source, len);
        _serial_api_reset_in_buffer(state, source);
    } else {
        *next = byte;
    }
}

serial_api_response_t serial_api_read_response(serial_api_state_t *state,
                                               int source)
{
    serial_api_response_t result;

    result.buffer = _serial_api_out(state, source, 0);
    result.length = state->out_indices[source];
    state->out_indices[source] = 0;
    return result;
}

void serial_api_queue_byte(serial_api_state_t *state, int source, char byte)
{
    int i = state->indices[source]++;

    if (i < SERIAL_API_IN_BUFFER_SIZE) {
        _serial_api_inner_queue_byte(state, source, byte, i);
    } else {
        _serial_api_print(state, source, "f ");
        _serial_api_end(state, source, MAX_INPUT_LENGTH_EXCEEDED);
        _serial_api_reset_in_buffer(state, source);
    }
}

void serial_api_queue_output(serial_api_state_t *state,
                             int source,
                             const char *message)
{
    _serial_api_end(state, source, message);
}

void serial_api_queue_output_len(serial_api_state_t *state,
                             int source,
                             char *message,
                             int length)
{
    _serial_api_end_len(state, source, message, length);
}
