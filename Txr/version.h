#ifndef VERSION_H
#define VERSION_H

enum {
    ROLE_PAW,
    ROLE_DOGBONE,
    ROLE_COUNT
};

const int VERSION_MAJOR = 0x02;
const int VERSION_MINOR = 0x02;
const int VERSION = (VERSION_MAJOR << 8) | VERSION_MINOR;
const int ROLE = ROLE_PAW;

inline int low_byte(int val)
{
    return val & 0x00ff;
}

inline int high_byte(int val)
{
    return (val & 0xff00) >> 8;
}

#endif
