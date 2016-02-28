#include "version.h"
#include "serial_api.h"
#include "radio.h"
#include "bsp.h"
#include "settings.h"
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

void _queue_get_radio_val(serial_api_state_t *state, char type)
{
    radio_packet_t packet;
    packet.type = type;
    radio_queue_message(state->radio_state, packet);
}

void _parse_and_send_uval(serial_api_state_t *state, char* in, char type)
{
    unsigned int val;
    sscanf(in + 2, "%u", &val);
    radio_packet_t packet;
    packet.type = type;
    packet.typed_uval.val = val;
    radio_queue_message(state->radio_state, packet);
}

void _print_val(serial_api_state_t *state, int source, char type, int val)
{
    char buffer[16];
    sprintf(buffer, "%c=%d", type, val);
    _serial_api_end(state, source, buffer);
}

void _serial_api_process_command(serial_api_state_t *state, int source,
                                 int length)
{
    char *in = _serial_api_in(state, source, 0);
    char cmd = *in;
    switch (cmd) {
    case (SERIAL_API_CMD_ECHO): {
        if (length < 3) {
            _serial_api_end(state, SERIAL_API_SRC_CONSOLE, MALFORMED_COMMAND);
        } else {
            _serial_api_end_len(state, source, in + 2, length - 2);
        }
    } break;
    case (SERIAL_API_CMD_VERSION): {
        char buffer[16];
        sprintf(buffer, "%c=%d.%d", cmd, VERSION_MAJOR, VERSION_MINOR);
        _serial_api_end(state, source, buffer);
    } break;
    case (SERIAL_API_CMD_SET_START_STATE): {
        int mode;
        sscanf(in + 2, "%d", &mode);
        settings_set_start_in_calibration_mode(mode);
    } break;
    case (SERIAL_API_CMD_ROLE): {
        _print_val(state, source, cmd, ROLE);
    } break;
    case (SERIAL_API_CMD_GET_CHANNEL): {
        _print_val(state, source, cmd, settings_get_channel());
    } break;
    case (SERIAL_API_CMD_GET_START_STATE): {
        _print_val(state, source, cmd, settings_get_start_in_calibration_mode());
    } break;
    case (SERIAL_API_CMD_REMOTE_VERSION): {
        _queue_get_radio_val(state, PACKET_GET_VERSION);
    } break;
    case (SERIAL_API_CMD_REMOTE_ROLE): {
        _queue_get_radio_val(state, PACKET_GET_ROLE);
    } break;
    case (SERIAL_API_CMD_GET_MAX_VELOCITY): {
        _queue_get_radio_val(state, PACKET_GET_MAX_VELOCITY);
    } break;
    case (SERIAL_API_CMD_GET_ACCEL): {
        _queue_get_radio_val(state, PACKET_GET_ACCEL);
    } break;
    case (SERIAL_API_CMD_GET_Z_MAX_VELOCITY): {
        _queue_get_radio_val(state, PACKET_GET_Z_MAX_VELOCITY);
    } break;
    case (SERIAL_API_CMD_GET_Z_ACCEL): {
        _queue_get_radio_val(state, PACKET_GET_Z_ACCEL);
    } break;
    case (SERIAL_API_CMD_SET_MAX_VELOCITY): {
        _parse_and_send_uval(state, in, PACKET_SET_MAX_VELOCITY);
    } break;
    case (SERIAL_API_CMD_SET_ACCEL): {
        _parse_and_send_uval(state, in, PACKET_SET_ACCEL);
    } break;
    case (SERIAL_API_CMD_SET_Z_MAX_VELOCITY): {
        _parse_and_send_uval(state, in, PACKET_SET_Z_ACCEL);
    } break;
    case (SERIAL_API_CMD_SET_Z_ACCEL): {
        _parse_and_send_uval(state, in, PACKET_SET_Z_ACCEL);
    } break;
    default: {
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
        _serial_api_end(state, SERIAL_API_SRC_CONSOLE, MAX_INPUT_LENGTH_EXCEEDED);
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
