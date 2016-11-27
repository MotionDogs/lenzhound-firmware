#ifndef LEDS_H__
#define LEDS_H__

#include "qp_port.h"
#include "bsp.h"
#include "serial_api.h"

enum {
    LED_OFF,
    LED_ON,
    LED_TOGGLE
};

// NOTE(doug): this exists to make it easier in the future to shoot out events
// for these LEDs to the API, to make the UI prettier if we want to.
inline void set_LED_status(int led, int status)
{
    log_value(SERIAL_LEDS, ((status & 0xff) << 8) | (led & 0xff));
    if (status == LED_ON) {
        switch (led) {
        case SPEED_LED1: {
            RED_LED_ON();
        } break;
        case SPEED_LED2: {
            AMBER_LED_ON();
        } break;
        case SPEED_LED4: {
            AMBER2_LED_ON();
        } break;
        case SPEED_LED5: {
            GREEN_LED_ON();
        } break;
        case GREEN_LED: {
            GREEN2_LED_ON();
        } break;
        case ENC_RED_LED: {
            ENC_RED_LED_ON();
        } break;
        case ENC_GREEN_LED: {
            ENC_GREEN_LED_ON();
        } break;
        }
    } else if (status == LED_OFF) {
        switch (led) {
        case SPEED_LED1: {
            RED_LED_OFF();
        } break;
        case SPEED_LED2: {
            AMBER_LED_OFF();
        } break;
        case SPEED_LED4: {
            AMBER2_LED_OFF();
        } break;
        case SPEED_LED5: {
            GREEN_LED_OFF();
        } break;
        case GREEN_LED: {
            GREEN2_LED_OFF();
        } break;
        case ENC_RED_LED: {
            ENC_RED_LED_OFF();
        } break;
        case ENC_GREEN_LED: {
            ENC_GREEN_LED_OFF();
        } break;
        }
    } else if (status == LED_TOGGLE) {
        switch (led) {
        case SPEED_LED1: {
            RED_LED_TOGGLE();
        } break;
        case SPEED_LED2: {
            AMBER_LED_TOGGLE();
        } break;
        case SPEED_LED4: {
            AMBER2_LED_TOGGLE();
        } break;
        case SPEED_LED5: {
            GREEN_LED_TOGGLE();
        } break;
        case GREEN_LED: {
            GREEN2_LED_TOGGLE();
        } break;
        case ENC_RED_LED: {
            ENC_RED_LED_TOGGLE();
        } break;
        case ENC_GREEN_LED: {
            ENC_GREEN_LED_TOGGLE();
        } break;
        }
    }
}

inline void set_speed_LED_status(int led, int status)
{
    int actual = 0;
    switch (led) {
        case 0: {
            actual = SPEED_LED1;
        } break;
        case 1: {
            actual = SPEED_LED2;
        } break;
        case 2: {
            actual = SPEED_LED4;
        } break;
        case 3: {
            actual = SPEED_LED5;
        } break;
        default: {
            return;
        }
    }
    set_LED_status(actual, status);
}

inline void set_speed_LEDs_off()
{
    analogWrite(SPEED_LED1, 0);
    analogWrite(SPEED_LED2, 0);
    analogWrite(SPEED_LED3_1, 0);
    analogWrite(SPEED_LED3_2, 0);
    analogWrite(SPEED_LED4, 0);
    analogWrite(SPEED_LED5, 0);

    set_speed_LED_status(0, LED_OFF);
    set_speed_LED_status(1, LED_OFF);
    set_speed_LED_status(2, LED_OFF);
    set_speed_LED_status(3, LED_OFF);
}

#endif /* end of include guard: LEDS_H__ */