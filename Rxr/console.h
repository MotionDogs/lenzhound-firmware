#ifndef console_h
#define console_h

#include "serial_api.h"

struct console_state_t {
    int failing_to_write;
};

void console_run();

#endif
