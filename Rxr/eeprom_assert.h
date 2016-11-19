#ifndef eeprom_assert_h
#define eeprom_assert_h

enum {
    ERROR_READ_EXCEEDED_EEPROM_SIZE,
    ERROR_WRITE_EXCEEDED_EEPROM_SIZE,
    ERROR_UNKNOWN_OK_CODE,
};

void eeprom_assert(bool condition, int code);

#endif