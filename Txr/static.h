#ifndef static_h
#define static_h

#include "console.h"
#include "radio.h"
#include "serial_api.h"

console_state_t* get_console_state();
radio_state_t* get_radio_state();
serial_api_state_t* get_serial_api_state();

#endif
