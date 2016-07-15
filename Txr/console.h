#ifndef console_h
#define console_h

#include "serial_api.h"

struct console_state_t {
    serial_api_state_t* serial_state;
};

void console_run(console_state_t* state);

#endif
