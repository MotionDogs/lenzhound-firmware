#ifndef SERIAL_API_H
#define SERIAL_API_H

#include "radio.h"
#include "motorcontroller.h"

const int SERIAL_API_IN_BUFFER_SIZE = 128;
const int SERIAL_API_OUT_BUFFER_SIZE = 128;
const char SERIAL_API_END_OF_RESPONSE = '\n';
const char SERIAL_API_END_OF_COMMAND = '\n';
const char SERIAL_API_ESCAPE = '\\';

#define MAX_RESPONSE_LENGTH_EXCEEDED "ERR 01"
#define MAX_INPUT_LENGTH_EXCEEDED    "ERR 02"
#define UNKNOWN_COMMAND              "ERR 03"
#define MALFORMED_COMMAND            "ERR 04"

struct radio_state_t;

struct serial_api_state_t {
    lh::MotorController* motor_controller;
    radio_state_t* radio_state;
    char in_buffer[SERIAL_API_IN_BUFFER_SIZE];
    char out_buffer[SERIAL_API_OUT_BUFFER_SIZE];
    int in_index;
    int escaped;
    int out_index;
};

enum {
    SERIAL_API_CMD_ECHO = 'h',
    SERIAL_API_CMD_VERSION = 'v',
    SERIAL_API_CMD_ROLE = 'r',
    SERIAL_API_CMD_REMOTE_VERSION = 'w',
    SERIAL_API_CMD_REMOTE_ROLE = 's',
    SERIAL_API_CMD_SAVE_CONFIG = 'u',
    SERIAL_API_CMD_GET_CHANNEL = 'c',
    SERIAL_API_CMD_SET_CHANNEL = 'C',
    SERIAL_API_CMD_GET_REMOTE_CHANNEL = 'd',
    SERIAL_API_CMD_SET_REMOTE_CHANNEL = 'D',
    SERIAL_API_CMD_GET_START_STATE = 't',
    SERIAL_API_CMD_SET_START_STATE = 'T',
    SERIAL_API_CMD_GET_MAX_VELOCITY = 'm',
    SERIAL_API_CMD_SET_MAX_VELOCITY = 'M',
    SERIAL_API_CMD_GET_ACCEL = 'a',
    SERIAL_API_CMD_SET_ACCEL = 'A',
    SERIAL_API_CMD_GET_Z_MAX_VELOCITY = 'n',
    SERIAL_API_CMD_SET_Z_MAX_VELOCITY = 'N',
    SERIAL_API_CMD_GET_Z_ACCEL = 'b',
    SERIAL_API_CMD_SET_Z_ACCEL = 'B',
    SERIAL_API_CMD_GET_POT = 'p',
    SERIAL_API_CMD_GET_ENCODER = 'e',
    SERIAL_API_CMD_LEDS = 'l',
};

struct serial_api_echo_command_t {
    char type;
    char *input;
    int length;
};

struct serial_api_command_t {
    union {
        char type;
        serial_api_echo_command_t echo;
    };
};

struct serial_api_response_t {
    char *buffer;
    int length;
};

serial_api_response_t serial_api_read_response(serial_api_state_t *state);
void serial_api_queue_byte(serial_api_state_t *state, char byte);
void serial_api_queue_output(serial_api_state_t *state, const char *message);
void serial_api_queue_output_len(serial_api_state_t *state,
                                 char *message,
                                 int length);

#endif
