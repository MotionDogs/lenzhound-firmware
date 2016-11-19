
#include <EEPROM.h>
#include "eeprom_assert.h"
#include "eeprom_helpers.h"

void eeprom_read_bytes(int start, char* buffer, int count)
{
  eeprom_assert(
    start >= EEPROM_MIN_ADDR && (start + count) <= EEPROM_MAX_ADDR,
    ERROR_READ_EXCEEDED_EEPROM_SIZE);

  for (int i = 0; i < count; i++) {
    buffer[i] = EEPROM.read(start + i);
  }
}

void eeprom_read_string(int start, char* buffer, int max_count)
{
  eeprom_assert(
    start >= EEPROM_MIN_ADDR && (start + max_count) <= EEPROM_MAX_ADDR,
    ERROR_READ_EXCEEDED_EEPROM_SIZE);

  int i = 0;
  for (; i < max_count; i++) {
    buffer[i] = EEPROM.read(start + i);
    if (!buffer[i]) {
      break;
    }
  }
  buffer[i] = 0;
}

void eeprom_write_bytes(int start, char* buffer, int count)
{
  eeprom_assert(
    start >= EEPROM_MIN_ADDR && (start + count) <= EEPROM_MAX_ADDR,
    ERROR_WRITE_EXCEEDED_EEPROM_SIZE);

  for (int i = 0; i < count; i++) {
    EEPROM.write(start + i, buffer[i]);
  }
}

void eeprom_write_string(int start, char* buffer, int max_count)
{
  eeprom_assert(
    start >= EEPROM_MIN_ADDR && (start + max_count) <= EEPROM_MAX_ADDR,
    ERROR_WRITE_EXCEEDED_EEPROM_SIZE);

  int i = 0;
  for (; i < max_count; i++) {
    EEPROM.write(start + i, buffer[i]);
    if (!buffer[i]) {
      break;
    }
  }
  EEPROM.write(start + i, 0);
}

char eeprom_read_char(int addr)
{
  char result;
  eeprom_read_bytes(addr, (char*)&result, sizeof(char));
  return result;
}

int eeprom_read_int16(int addr)
{
  int result;
  eeprom_read_bytes(addr, (char*)&result, sizeof(int));
  return result;
}

unsigned int eeprom_read_uint16(int addr)
{
  unsigned int result;
  eeprom_read_bytes(addr, (char*)&result, sizeof(unsigned int));
  return result;
}

long eeprom_read_int32(int addr)
{
  long result;
  eeprom_read_bytes(addr, (char*)&result, sizeof(long));
  return result;
}

unsigned long eeprom_read_uint32(int addr)
{
  unsigned long result;
  eeprom_read_bytes(addr, (char*)&result, sizeof(unsigned long));
  return result;
}

void eeprom_write_char(int addr, char value)
{
  eeprom_write_bytes(addr, &value, sizeof(value));
}

void eeprom_write_int16(int addr, int value)
{
  eeprom_write_bytes(addr, (char*)&value, sizeof(value));
}

void eeprom_write_uint16(int addr, unsigned int value)
{
  eeprom_write_bytes(addr, (char*)&value, sizeof(value));
}

void eeprom_write_int32(int addr, long value)
{
  eeprom_write_bytes(addr, (char*)&value, sizeof(value));
}

void eeprom_write_uint32(int addr, unsigned long value)
{
  eeprom_write_bytes(addr, (char*)&value, sizeof(value));
}


void eeprom_write_debug_string(char* buffer)
{
  int i = DEBUG_STRING_LOC;
  int end_i = DEBUG_STRING_LOC + DEBUG_STRING_MAX_LEN;
  char* ptr = buffer;

  while (*ptr && i < end_i) {
    EEPROM.write(i, *ptr);
    ++ptr;
  }
}