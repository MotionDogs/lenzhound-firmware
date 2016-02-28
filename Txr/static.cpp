#include "static.h"

static console_state_t console_state = {0};
static radio_state_t radio_state = {0};
static serial_api_state_t serial_api_state = {0};

console_state_t* get_console_state()
{
    return &console_state;
}

radio_state_t* get_radio_state()
{
    return &radio_state;
}

serial_api_state_t* get_serial_api_state()
{
    return &serial_api_state;
}
