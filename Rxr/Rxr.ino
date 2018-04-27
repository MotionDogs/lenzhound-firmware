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

#define IS_STEPPER_MOTOR 0

#include <SPI.h>
#include <Mirf.h>
#include <MirfHardwareSpiDriver.h>
#include <MirfSpiDriver.h>
#include <nRF24L01.h>
#include <EEPROM.h>
#include "NewTimerOne.h"
#include "settings.h"
#include "console.h"
#include "util.h"
#include "constants.h"
#include "macros.h"
#include "motor.h"
#include "controller.h"
#include "serial_api.h"
#include "radio.h"


void timer_interrupt()
{
    controller_run();
}

void setup()
{
    Serial.begin(SERIAL_BAUD);

    // dcdrv
    SET_MODE(DCSPEED_PIN, OUT);
    SET_MODE(DCIN1_PIN, OUT);
    SET_MODE(DCIN2_PIN, OUT);

    // antenna
//    SET_MODE(ANT_CTRL1, OUT);
//    SET_MODE(ANT_CTRL2, OUT);

//    ANT_CTRL1(CLR);
//    ANT_CTRL2(SET);

    controller_init();
    radio_init();
    controller_set_accel(1);
    controller_set_speed(1);
    controller_set_current_level(0);

    Timer1.initialize();
    Timer1.attachInterrupt(timer_interrupt, ISR_PERIOD);
}

void loop()
{
    radio_run();
    console_run();
}
