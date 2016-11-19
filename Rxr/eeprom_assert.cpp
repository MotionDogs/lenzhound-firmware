#include "Arduino.h"
#include "eeprom_helpers.h"

void eeprom_assert(bool condition, int code)
{
    if (!condition) {
        char buffer[100];

        int length = sprintf(buffer, "ERR: %d", code);

        eeprom_write_debug_string(buffer);

        asm volatile ("jmp 0x0000"); 
    }
}