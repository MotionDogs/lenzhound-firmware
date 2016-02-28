//****************************************************************************
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//****************************************************************************

#include <qp_port.h>//#include "qp_port.h"
#include "Txr.h"
#include "bsp.h"
#include "Arduino.h"                                   // Arduino include file
#include <Encoder.h>
#include "radio.h"
#include "console.h"
#include "serial_api.h"
#include "settings.h"
#include "static.h"


Q_DEFINE_THIS_FILE

//#define SAVE_POWER

#define TICK_DIVIDER       ((F_CPU / BSP_TICKS_PER_SEC / 1024) - 1)

#if TICK_DIVIDER > 255
#   error BSP_TICKS_PER_SEC too small
#elif TICK_DIVIDER < 2
#   error BSP_TICKS_PER_SEC too large
#endif

static Encoder encoder(2, 1);
static int PrevButtonState = 0;
static int PrevModeState = -1;    // force signal on startup with -1
static int PrevPositionState = 0; // position buttons

#ifdef Q_SPY
uint8_t l_TIMER2_COMPA;
#endif

#define RATE_MASK     0b00101000
#define RATE_250KB    0b00100000
#define RATE_1MB      0b00000000
#define RATE_2MB      0b00001000
#define PALEVEL_MASK  0b00000111
#define PALEVEL_18    0b00000000
#define PALEVEL_12    0b00000010
#define PALEVEL_6     0b00000100
#define PALEVEL_0     0b00000011

// ISRs ----------------------------------------------------------------------
ISR(TIMER4_COMPA_vect) {
    // No need to clear the interrupt source since the Timer4 compare
    // interrupt is automatically cleard in hardware when the ISR runs.

    QF::TICK(&l_TIMER2_COMPA);                // process all armed time events

    // Check state of buttons
    int curButtonState = CALBUTTON_ON();

    if (curButtonState != PrevButtonState) {
        if (curButtonState != 0) {
            QF::PUBLISH(Q_NEW(QEvt, ENC_DOWN_SIG), &l_TIMER2_COMPA);
        } else {
            QF::PUBLISH(Q_NEW(QEvt, ENC_UP_SIG), &l_TIMER2_COMPA);
        }
        PrevButtonState = curButtonState;
    }

    // Check mode switches
    curButtonState = MODE_SWITCHES();
    if (curButtonState != PrevModeState) {
        if (curButtonState & 0x10) {
            QF::PUBLISH(Q_NEW(QEvt, Z_MODE_SIG), &l_TIMER2_COMPA);
        } else if (curButtonState & 0x40) {
            QF::PUBLISH(Q_NEW(QEvt, FREE_MODE_SIG), &l_TIMER2_COMPA);
        } else {
            QF::PUBLISH(Q_NEW(QEvt, PLAY_MODE_SIG), &l_TIMER2_COMPA);
        }
        PrevModeState = curButtonState;
    }

    // Check position buttons
    curButtonState = PBUTTONS();
    if (curButtonState != PrevPositionState) {
        // only look at buttons that have changed to the on state (might have pressed two at once)
        int tempState = curButtonState ^ PrevPositionState;
        tempState &= curButtonState;
        PrevPositionState = curButtonState;
        if (tempState != 0) {
            PositionButtonEvt *evt = Q_NEW(PositionButtonEvt,
                                           POSITION_BUTTON_SIG);
            if (tempState & 0x40) {
                evt->ButtonNum = 0;
            } else if (tempState & 0x20) {
                evt->ButtonNum = 1;
            } else if (tempState & 0x10) {
                evt->ButtonNum = 2;
            } else if (tempState & 0x02) {
                evt->ButtonNum = 3;
            }
            QF::PUBLISH(evt, &l_TIMER2_COMPA);
        }
    }

    console_run(get_console_state());
    radio_run(get_radio_state());
}

//............................................................................
void BSP_init(void)
{
    pinMode(ZMODE_SW, INPUT);
    pinMode(FREE_SW, INPUT);
    pinMode(cBUTTON, INPUT);
    pinMode(p1BUTTON, INPUT);
    pinMode(p2BUTTON, INPUT);
    pinMode(p3BUTTON, INPUT);
    pinMode(p4BUTTON, INPUT);
    pinMode(SPEED_LED1, OUTPUT);
    pinMode(SPEED_LED2, OUTPUT);
    pinMode(SPEED_LED3, OUTPUT);
    pinMode(SPEED_LED4, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(ENC_RED_LED, OUTPUT);
    pinMode(ENC_GREEN_LED, OUTPUT);

    // init Console
    Serial.begin(57600);
    get_console_state()->serial_state = get_serial_api_state();
    get_radio_state()->serial_state = get_serial_api_state();
    get_serial_api_state()->radio_state = get_radio_state();

    radio_init(get_radio_state());

    // todo: not sure why this is here, but I tried taking it out once
    // and the program crashed
    digitalWrite(A5, HIGH);

    // White LED stays on always
    WHITE_LED_ON();

    //if (QS_INIT((void *)0) == 0) {       // initialize the QS software tracing
    //    Q_ERROR();
    //}

    //QS_OBJ_DICTIONARY(&l_SysTick_Handler);
}

//............................................................................
void QF::onStartup(void)
{
    // set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking
    TCCR4A = (1 << WGM41) | (0 << WGM40);
    TCCR4B = (1 << CS42) | (1 << CS41) | (1 << CS40); // 1/2^10
    //ASSR &= ~(1 << AS2);
    TIMSK4 = (1 << OCIE4A);                           // Enable TIMER2 compare Interrupt
    TCNT4 = 0;
    OCR4A = TICK_DIVIDER;                             // must be loaded last for Atmega168 and friends
}

//............................................................................
void QF::onCleanup(void)
{
}

//............................................................................
void QF::onIdle()
{
    //GREEN_LED_ON();     // toggle the GREEN LED on Arduino on and off, see NOTE1
    //GREEN_LED_OFF();

#ifdef SAVE_POWER

    SMCR = (0 << SM0) | (1 << SE); // idle sleep mode, adjust to your project

    // never separate the following two assembly instructions, see NOTE2
    __asm__ __volatile__ ("sei" "\n\t" :: );
    __asm__ __volatile__ ("sleep" "\n\t" :: );

    SMCR = 0;                                            // clear the SE bit

#else
    QF_INT_ENABLE();
#endif
}

long BSP_get_encoder()
{
    return encoder.read();
}

int BSP_get_pot()
{
    return analogRead(A0);
}

int BSP_get_mode()
{
    // basically duplicates what's in the ISR above,
    // but sharing the code wouldn't be so efficient in this case
    int buttonState = MODE_SWITCHES();

    if (buttonState & 0x10) {
        return Z_MODE;
    } else if (buttonState & 0x40) {
        return FREE_MODE;
    } else {
        return PLAYBACK_MODE;
    }
}

void BSP_turn_on_speed_LED(char num)
{
    switch (num) {
    case 0:
        RED_LED_ON();
        break;
    case 1:
        analogWrite(SPEED_LED2, 255);
        break;
    case 2:
        analogWrite(SPEED_LED3, 255);
        break;
    case 3:
        GREEN_LED_ON();
        break;
    }
}

void BSP_turn_off_speed_LED(char num)
{
    switch (num) {
    case 0:
        RED_LED_OFF();
        break;
    case 1:
        analogWrite(SPEED_LED2, 0);
        break;
    case 2:
        analogWrite(SPEED_LED3, 0);
        break;
    case 3:
        GREEN_LED_OFF();
        break;
    }
}

bool BSP_serial_available()
{
    return Serial.available() > 0;
}

char BSP_serial_read()
{
    return Serial.read();
}

int BSP_write_serial(char* buffer, int length)
{
    return Serial.write(buffer, length);
}

void BSP_assert(bool condition)
{
    Q_ASSERT(condition);
}

//............................................................................
void Q_onAssert(char const Q_ROM *const Q_ROM_VAR file, int line)
{
    QF_INT_DISABLE();                                // disable all interrupts
    GREEN_LED_ON();                                  // GREEN LED permanently ON
    RED_LED_ON();
    // AMBER_LED_ON();
    // AMBER2_LED_ON();
    asm volatile ("jmp 0x0000");    // perform a software reset of the Arduino
}

//////////////////////////////////////////////////////////////////////////////
// NOTE1:
// The Arduino's User LED is used to visualize the idle loop activity.
// The brightness of the LED is proportional to the frequency of invcations
// of the idle loop. Please note that the LED is toggled with interrupts
// locked, so no interrupt execution time contributes to the brightness of
// the User LED.
//
// NOTE2:
// The QF_onIdle() callback is called with interrupts *locked* to prevent
// a race condtion of posting a new event from an interrupt while the
// system is already committed to go to sleep. The only *safe* way of
// going to sleep mode is to do it ATOMICALLY with enabling interrupts.
// As described in the "AVR Datasheet" in Section "Reset and Interrupt
// Handling", when using the SEI instruction to enable interrupts, the
// instruction following SEI will be executed before any pending interrupts.
// As the Datasheet shows in the assembly example, the pair of instructions
//     SEI       ; enable interrupts
//     SLEEP     ; go to the sleep mode
// executes ATOMICALLY, and so *no* interrupt can be serviced between these
// instructins. You should NEVER separate these two lines.
//
