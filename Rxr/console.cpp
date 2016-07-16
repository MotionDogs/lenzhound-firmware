#include "Arduino.h"
#include "console.h"
#include "serial_api.h"

void console_run(console_state_t *state)
{
    while (Serial.available() > 0) {
        serial_api_queue_byte(state->serial_state,
                              (char)Serial.read());
        state->failing_to_write = 0;
    }

    serial_api_response_t response = serial_api_read_response(
        state->serial_state);

    int index = 0;
    int remaining = response.length;

    if (remaining && !state->failing_to_write) {
        for (int i = 0; i < 3; ++i)
        {
            int written = Serial.write(response.buffer + index, remaining);
            index += written;
            remaining -= written;
            if (remaining <= 0) {
                break;
            }
        }
        if (!index) {
            state->failing_to_write = 1;
        }
    }
}
