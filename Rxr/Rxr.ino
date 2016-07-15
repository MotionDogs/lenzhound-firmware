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

#include <SPI.h>
#include <Mirf.h>
#include <MirfHardwareSpiDriver.h>
#include <MirfSpiDriver.h>
#include <nRF24L01.h>
#include <EEPROM.h>
#include <CmdMessenger.h>
#include "NewTimerOne.h"
#include "settings.h"
#include "console.h"
#include "util.h"
#include "constants.h"
#include "macros.h"
#include "motor.h"
#include "motorcontroller.h"
#include "events.h"
#include "serial_api.h"
#include "radio.h"

// Console console;
static serial_api_state_t serial_api_state = { 0 };
static console_state_t console_state = { 0 };
static radio_state_t radio_state = { 0 };
static lh::MotorController motor_controller = lh::MotorController();

void timer_interrupt()
{
    motor_controller.run();
}

void setup()
{
    Serial.begin(SERIAL_BAUD);

    SET_MODE(SLEEP_PIN, OUT);
    SET_MODE(ENABLE_PIN, OUT);
    SET_MODE(MS1_PIN, OUT);
    SET_MODE(MS2_PIN, OUT);
    SET_MODE(STEP_PIN, OUT);
    SET_MODE(DIR_PIN, OUT);
    SET_MODE(ANT_CTRL1, OUT);
    SET_MODE(ANT_CTRL2, OUT);

    ANT_CTRL1(CLR);
    ANT_CTRL2(SET);

    radio_state.serial_state = &serial_api_state;
    radio_state.motor_controller = &motor_controller;
    radio_init(&radio_state);
    console_state.serial_state = &serial_api_state;
    serial_api_state.motor_controller = &motor_controller;

    long accel = settings_get_accel();
    long max_velocity = settings_get_max_velocity();
    long z_max_velocity = settings_get_z_max_velocity();
    long z_accel = settings_get_z_accel();

    motor_controller.set_accel(accel);
    motor_controller.set_velocity(max_velocity);
    motor_controller.set_z_accel(z_accel);
    motor_controller.set_z_velocity(z_max_velocity);
    motor_controller.set_accel_percent(100);
    motor_controller.set_velocity_percent(100);

    Timer1.initialize();
    Timer1.attachInterrupt(timer_interrupt, ISR_PERIOD);
}

void loop()
{
    radio_run(&radio_state);
    console_run(&console_state);
}
