
#ifndef eeprom_helpers_h
#define eeprom_helpers_h

#define EEPROM_MIN_ADDR         0
#define EEPROM_MAX_ADDR         899

#define DEBUG_STRING_LOC        900
#define DEBUG_STRING_MAX_LEN    100

//#define EEPROM_MIN_ADDR  0;
//#define EEPROM_MAX_ADDR  1023;

void eeprom_read_bytes(int start, unsigned char* buffer, int count);
void eeprom_read_string(int start, char* buffer, int max_count);
char eeprom_read_char(int addr);
int eeprom_read_int16(int addr);
unsigned int eeprom_read_uint16(int addr);
long eeprom_read_int32(int addr);
unsigned long eeprom_read_uint32(int addr);

void eeprom_write_bytes(int start, unsigned char* buffer, int count);
void eeprom_write_string(int start, char* buffer, int max_count);
void eeprom_write_char(int addr, char value);
void eeprom_write_char(int addr, char value);
void eeprom_write_int16(int addr, int value);
void eeprom_write_uint16(int addr, unsigned int value);
void eeprom_write_int32(int addr, long value);
void eeprom_write_uint32(int addr, unsigned long value);

void eeprom_write_debug_string(char* buffer);

#endif  // eeprom_helpers_h
