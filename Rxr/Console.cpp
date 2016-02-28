#include "Arduino.h"
#include "console.h"
#include "serial_api.h"

void console_run(console_state_t *state)
{
    while (Serial.available() > 0) {
        serial_api_queue_byte(state->serial_state,
                              SERIAL_API_SRC_CONSOLE,
                              (char)Serial.read());
    }

    serial_api_response_t response = serial_api_read_response(
        state->serial_state,
        SERIAL_API_SRC_CONSOLE);

    if (response.length) {
        int written = Serial.write(response.buffer, response.length);

        // BSP_assert(written == response.length);
    }
}
