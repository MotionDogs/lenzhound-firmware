#include "Arduino.h"
#include "console.h"
#include "serial_api.h"
#include "bsp.h"

void console_run(console_state_t *state)
{
    while (BSP_serial_available() > 0) {
        serial_api_queue_byte(state->serial_state,
                              SERIAL_API_SRC_CONSOLE,
                              (char)BSP_serial_read());
    }

    serial_api_response_t response = serial_api_read_response(
        state->serial_state,
        SERIAL_API_SRC_CONSOLE);

    int write_index = 0;
    int remaining = response.length;

    for (int i = 0; i < 3; ++i)
    {
        int written = Serial.write(response.buffer + write_index, remaining);
        write_index += written;
        remaining -= written;
        if (remaining <= 0) {
            break;
        }
    }
}
