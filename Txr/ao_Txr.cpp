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

#include "qp_port.h"
#include "bsp.h"
#include "Txr.h"
#include "Arduino.h" // if I start having problems with RF24, consider removing this
#include "radio.h"
#include "serial_api.h"
#include "console.h"
#include "settings.h"
#include "leds.h"

Q_DEFINE_THIS_FILE

#define ENCODER_CLAMP_SLOP 2

#define DIMLY_LIT_ENCODER_VAL 13
#define ENCODER_STEPS_PER_CLICK 4

#define BUTTON_LED_OFFISH_THRESHOLD 24

// various timeouts in ticks
enum TxrTimeouts {
    // how often to send encoder position
    SEND_ENCODER_TOUT  = BSP_TICKS_PER_SEC / 100,
    // how quick to flash LED
    FLASH_RATE_TOUT = BSP_TICKS_PER_SEC / 2,
    // how long to be flashing LED for
    FLASH_DURATION_TOUT = BSP_TICKS_PER_SEC / 4,
    // how long to hold calibration button before reentering calibration
    ENTER_CALIBRATION_TOUT = BSP_TICKS_PER_SEC * 1,
    // how often to check that transmitter is still powered (in case of low battery)
    ALIVE_DURATION_TOUT = BSP_TICKS_PER_SEC * 5,

    SEND_SPEED_AND_ACCEL_TOUT = BSP_TICKS_PER_SEC / 4,

    FLUSH_SETTINGS_TOUT = BSP_TICKS_PER_SEC * 4,

    DOUBLE_TAP_TOUT = BSP_TICKS_PER_SEC * 1
};

enum EncoderModes {
    ENCODER_MODE_ACCEL,
    ENCODER_MODE_SPEED
};

// todo: move this somewhere else or do differently
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long clamp(long x, long min, long max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}

long distance(long a, long b)
{
    return abs(a - b);
}

class Txr : public QP::QActive {
private:
    QTimeEvt alive_timeout_;
    QTimeEvt flash_timeout_;
    QTimeEvt send_timeout_;
    QTimeEvt speed_and_accel_timeout_;
    QTimeEvt flush_settings_timeout_;
    QTimeEvt calibration_timeout_;
    QTimeEvt double_tap_timeout_;
    long play_back_target_pos_;
    long cur_pos_;    // float to save partial moves needed by encoder resolution division
    long prev_pos_1_;   // used to prevent jitter
    long prev_pos_2_;   // used to prevent jitter
    long initial_encoder_count_;
    long initial_position_;
    long previous_encoder_count_;
    long calibration_pos_1_;
    long calibration_pos_2_;
    char enc_pushes_;
    char calibration_multiplier_;
    long saved_positions_[NUM_POSITION_BUTTONS];
    unsigned char prev_position_button_pressed_;
    long encoder_base_;
    int previous_preset_index_;
    int previous_channel_;
    bool double_tapping_;
    int encoder_mode_;
    bool flashing_position_button_;

public:
    Txr() :
        QActive((QStateHandler) & Txr::initial),
        flash_timeout_(FLASH_RATE_SIG),
        send_timeout_(SEND_TIMEOUT_SIG),
        flush_settings_timeout_(FLUSH_SETTINGS_TIMEOUT_SIG),
        speed_and_accel_timeout_(SPEED_AND_ACCEL_TIMEOUT_SIG),
        calibration_timeout_(CALIBRATION_SIG),
        alive_timeout_(ALIVE_SIG),
        double_tap_timeout_(DOUBLE_TAP_SIG),
        play_back_target_pos_(0),
        previous_preset_index_(0),
        previous_channel_(0),
        double_tapping_(false),
        encoder_mode_(ENCODER_MODE_SPEED)
    {
    }

protected:
    static QP::QState initial(Txr *const me, QP::QEvt const *const e);
    static QP::QState on(Txr *const me, QP::QEvt const *const e);
    static QP::QState uncalibrated(Txr *const me, QP::QEvt const *const e);
    static QP::QState calibrated(Txr *const me, QP::QEvt const *const e);
    static QP::QState flashing(Txr *const me, QP::QEvt const *const e);
    static QP::QState free_run_mode(Txr *const me, QP::QEvt const *const e);
    static QP::QState play_back_mode(Txr *const me, QP::QEvt const *const e);
    static QP::QState z_mode(Txr *const me, QP::QEvt const *const e);

    void update_position();
    void update_position_calibration();
    void update_position_play_back();
    void update_max_speed_using_encoder();
    void update_max_accel_using_encoder();
    void update_calibration_multiplier(int setting);
    void update_button_LEDs();
    void reset_calibration();
};

static Txr l_Txr;                   // the single instance of Txr active object (local)
QActive *const AO_Txr = &l_Txr;     // the global opaque pointer

int map_accel_LED(int x, int n)
{
    return x == n ? 128 : map(clamp(distance(x, n), 0, 7), 0, 7, 50, 0);
}

int map_speed_LED(int x, int n)
{
    return x == n ? 128 : map(clamp(distance(x, n), 0, 24), 0, 24, 50, 0);
}

void Txr::update_button_LEDs()
{
    int encoder_led;
    int button_leds[4];

    if (encoder_mode_ == ENCODER_MODE_ACCEL) {

        int factor = settings_get_max_accel() / ENCODER_STEPS_PER_CLICK;
        factor = clamp(factor, 1, 32);

        button_leds[0] = map_accel_LED(factor, 1);
        button_leds[1] = map_accel_LED(factor, 8);
        encoder_led = map_accel_LED(factor, 16);
        button_leds[2] = map_accel_LED(factor, 24);
        button_leds[3] = map_accel_LED(factor, 32);

        analogWrite(SPEED_LED3_1, encoder_led / 4);
        analogWrite(SPEED_LED3_2, min(DIMLY_LIT_ENCODER_VAL + encoder_led, 64) / 4);
    } else {

        int factor = (int)((long)settings_get_max_speed() * 100L / 32768L);
        factor = clamp(factor, 1, 100);

        button_leds[0] = map_speed_LED(factor, 1);
        button_leds[1] = map_speed_LED(factor, 25);
        encoder_led = map_speed_LED(factor, 50);
        button_leds[2] = map_speed_LED(factor, 75);
        button_leds[3] = map_speed_LED(factor, 100);

        analogWrite(SPEED_LED3_1, min(DIMLY_LIT_ENCODER_VAL + encoder_led, 64) / 4);
        analogWrite(SPEED_LED3_2, encoder_led / 4);
    }

    if (BSP_get_mode() == Z_MODE) {
        int preset = settings_get_preset_index();
        int* led = &button_leds[preset];
        float ms = (float)millis();

        // arbitrary constants here to get the desired period.
        int factor = (int)((sin(ms / 500.0f) + 1.0f) * 128.0f);
        *led = map(factor, 0, 256, max(1, *led), 16);
    }

    if (flashing_position_button_) {
        int* led = &button_leds[prev_position_button_pressed_];
        if (*led > BUTTON_LED_OFFISH_THRESHOLD) {
            *led = 0;
        } else {
            *led = 255;
        }
    }

    analogWrite(SPEED_LED1, button_leds[0]);
    analogWrite(SPEED_LED2, button_leds[1]);
    analogWrite(SPEED_LED4, button_leds[2]);
    analogWrite(SPEED_LED5, button_leds[3]);
}

void Txr::reset_calibration()
{
    initial_encoder_count_ = previous_encoder_count_ = BSP_get_encoder();
    initial_position_ = cur_pos_;
}

void Txr::update_position_calibration()
{
    long cur_encoder_count = BSP_get_encoder();

    if (cur_encoder_count != previous_encoder_count_) {
        log_value(SERIAL_ENCODER_GET, cur_encoder_count);
    }

    long encoder_delta = cur_encoder_count - initial_encoder_count_;
    long position_delta = (encoder_delta / ENCODER_STEPS_PER_CLICK) *
        calibration_multiplier_;

    long new_pos = initial_position_ + position_delta;

    previous_encoder_count_ = cur_encoder_count;

    if (new_pos != cur_pos_) {
        cur_pos_ = new_pos;
        PACKET_SEND(PACKET_TARGET_POSITION_SET, target_position_set, cur_pos_);
    }
}

void Txr::update_max_speed_using_encoder()
{
    long cur_encoder_count = BSP_get_encoder();

    if (cur_encoder_count != previous_encoder_count_) {
        log_value(SERIAL_ENCODER_GET, cur_encoder_count);
    }

    long encoder_delta = cur_encoder_count - previous_encoder_count_;
    long cur_max_speed = settings_get_max_speed();

    long max_speed_encoder_factor = clamp(cur_max_speed >> 6, 1, 128);

    long new_max_speed = cur_max_speed + (encoder_delta * max_speed_encoder_factor);
    new_max_speed = clamp(new_max_speed, 1, 32768);

    previous_encoder_count_ = cur_encoder_count;

    if (new_max_speed != cur_max_speed) {
        settings_set_max_speed((unsigned int)new_max_speed);
        log_value(SERIAL_MAX_SPEED_GET, new_max_speed);
    }
}

void Txr::update_max_accel_using_encoder()
{
    long cur_encoder_count = BSP_get_encoder();

    if (cur_encoder_count != previous_encoder_count_) {
        log_value(SERIAL_ENCODER_GET, cur_encoder_count);
    }

    long encoder_delta = cur_encoder_count - previous_encoder_count_;
    long cur_max_accel = settings_get_max_accel();

    long new_max_accel = cur_max_accel + encoder_delta;
    new_max_accel = clamp(new_max_accel,
        ENCODER_STEPS_PER_CLICK * 1,
        ENCODER_STEPS_PER_CLICK * 32);

    previous_encoder_count_ = cur_encoder_count;

    if (new_max_accel != cur_max_accel) {
        settings_set_max_accel(new_max_accel);
        log_value(SERIAL_ACCEL_GET, new_max_accel / ENCODER_STEPS_PER_CLICK);
    }
}

void Txr::update_position()
{
    long new_pos = BSP_get_pot();

    // only update the current position if it's not jittering between two values
    if (new_pos != prev_pos_1_ && new_pos != prev_pos_2_) {
        log_value(SERIAL_POT_GET, new_pos);

        prev_pos_1_ = prev_pos_2_;
        prev_pos_2_ = new_pos;
        cur_pos_ = map(new_pos, MIN_POT_VAL, MAX_POT_VAL,
                          calibration_pos_1_, calibration_pos_2_);

        PACKET_SEND(PACKET_TARGET_POSITION_SET, target_position_set, cur_pos_);
    }
}

void Txr::update_position_play_back()
{
    if (play_back_target_pos_ != cur_pos_) {
        cur_pos_ = play_back_target_pos_;
        PACKET_SEND(PACKET_TARGET_POSITION_SET, target_position_set, cur_pos_);
    }
}

void Txr::update_calibration_multiplier(int setting)
{
    switch (setting) {
        case PLAY_BACK_MODE: {
            calibration_multiplier_ = 40;
        } break;
        case Z_MODE: {
            calibration_multiplier_ = 80;
        } break;
        case FREE_MODE:
        default: {
            calibration_multiplier_ = 8;
        } break;
    }
}

QP::QState Txr::initial(Txr *const me, QP::QEvt const *const e)
{
    me->calibration_multiplier_ = 1;
    me->reset_calibration();
    me->prev_pos_1_ = -1;
    me->prev_pos_2_ = -1;
    me->subscribe(ENC_DOWN_SIG);
    me->subscribe(ENC_UP_SIG);
    me->subscribe(PLAY_BACK_MODE_SIG);
    me->subscribe(FREE_RUN_MODE_SIG);
    me->subscribe(Z_MODE_SIG);
    me->subscribe(POSITION_BUTTON_SIG);
    me->send_timeout_.postEvery(me, SEND_ENCODER_TOUT);
    me->flush_settings_timeout_.postEvery(me, FLUSH_SETTINGS_TOUT);
    me->alive_timeout_.postEvery(me, ALIVE_DURATION_TOUT);
    me->speed_and_accel_timeout_.postEvery(me, SEND_SPEED_AND_ACCEL_TOUT);
    me->calibration_pos_1_ = settings_get_calibration_position_1();
    me->calibration_pos_2_ = settings_get_calibration_position_2();
    me->previous_channel_ = settings_get_channel();

    long pos = map(BSP_get_pot(), MIN_POT_VAL, MAX_POT_VAL,
        me->calibration_pos_1_, me->calibration_pos_2_);
    me->cur_pos_ = pos;
    me->initial_position_ = pos;

    if (settings_get_start_in_calibration_mode()) {
        return Q_TRAN(&uncalibrated);
    } else {
        for (int i = 0; i < NUM_POSITION_BUTTONS; i++) {
            me->saved_positions_[i] = settings_get_saved_position(i);
        }

        if (FREESWITCH_ON()) {
            return Q_TRAN(&free_run_mode);
        } else if (ZSWITCH_ON()) {
            return Q_TRAN(&z_mode);
        } else {
            return Q_TRAN(&play_back_mode);
        }
    }
}

QP::QState Txr::on(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            status = Q_HANDLED();
        } break;
        case ALIVE_SIG: {
            if (radio_is_alive()) {
                set_LED_status(GREEN_LED, LED_OFF);
            } else {
                set_LED_status(GREEN_LED, LED_ON);
            }
            status = Q_HANDLED();
        } break;
        case SPEED_AND_ACCEL_TIMEOUT_SIG:{
            PACKET_SEND(PACKET_MAX_SPEED_SET, max_speed_set,
                settings_get_max_speed());
            PACKET_SEND(PACKET_ACCEL_SET, accel_set,
                settings_get_max_accel() / ENCODER_STEPS_PER_CLICK);

            int channel = settings_get_channel();
            if (channel != me->previous_channel_) {
                radio_set_channel(channel, false);
                me->previous_channel_ = channel;
            }

            status = Q_HANDLED();
        } break;
        case FLUSH_SETTINGS_TIMEOUT_SIG: {
            settings_flush_debounced_values();
            status = Q_HANDLED();
        } break;
        default: {
            status = Q_SUPER(&QP::QHsm::top);
        } break;
    }
    return status;
}

QP::QState Txr::uncalibrated(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            set_LED_status(ENC_RED_LED, LED_ON);
            set_LED_status(ENC_GREEN_LED, LED_OFF);
            me->update_calibration_multiplier(BSP_get_mode());
            me->cur_pos_ = 0;
            me->reset_calibration();
            PACKET_SEND_EMPTY(PACKET_RE_INIT_POSITION);

            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            status = Q_HANDLED();
        } break;
        case SEND_TIMEOUT_SIG: {
            me->update_position_calibration();
            status = Q_HANDLED();
        } break;
        case ENC_DOWN_SIG: {
            // if this is first time button press, just save the position
            if ((me->enc_pushes_)++ == 0) {
                me->calibration_pos_1_ = me->cur_pos_;
                settings_set_calibration_position_1(me->calibration_pos_1_);
            }
            // if this is second time, determine whether swapping is necessary
            // to map higher calibrated position with higher motor position
            else {
                me->calibration_pos_2_ = me->cur_pos_;
                settings_set_calibration_position_2(me->calibration_pos_2_);
            }
            status = Q_TRAN(&flashing);
        } break;
        case PLAY_BACK_MODE_SIG: {
            me->update_calibration_multiplier(PLAY_BACK_MODE);
            me->reset_calibration();
            status = Q_HANDLED();
        } break;
        case Z_MODE_SIG: {
            me->update_calibration_multiplier(Z_MODE);
            me->reset_calibration();
            status = Q_HANDLED();
        } break;
        case FREE_RUN_MODE_SIG: {
            me->update_calibration_multiplier(FREE_MODE);
            me->reset_calibration();
            status = Q_HANDLED();
        } break;
        default: {
            status = Q_SUPER(&on);
        } break;
    }
    return status;
}

QP::QState Txr::calibrated(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            set_LED_status(ENC_RED_LED, LED_OFF);
            set_LED_status(ENC_GREEN_LED, LED_ON);
            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            status = Q_HANDLED();
        } break;
        case PLAY_BACK_MODE_SIG: {
            status = Q_TRAN(&play_back_mode);
        } break;
        case Z_MODE_SIG: {
            status = Q_TRAN(&z_mode);
        } break;
        case FREE_RUN_MODE_SIG: {
            status = Q_TRAN(&free_run_mode);
        } break;
        case DOUBLE_TAP_SIG: {
            me->double_tapping_ = false;
        } break;
        case ENC_DOWN_SIG: {
            status = Q_HANDLED();
        } break;
        case ENC_UP_SIG: {
            if (me->double_tapping_) {
                me->double_tap_timeout_.disarm();

                // we double-tapped - switch to calibration mode
                me->enc_pushes_ = 0;
                status = Q_TRAN(&flashing);
            } else {
                if (me->encoder_mode_ == ENCODER_MODE_SPEED) {
                    me->encoder_mode_ = ENCODER_MODE_ACCEL;
                } else {
                    me->encoder_mode_ = ENCODER_MODE_SPEED;
                }

                me->double_tap_timeout_.postIn(me, DOUBLE_TAP_TOUT);
                me->double_tapping_ = true;

                status = Q_HANDLED();
            }
        } break;
        case CALIBRATION_SIG: {
            // we've held for ENTER_CALIBRATION_TOUT, go to calibration
        } break;
        default: {
            status = Q_SUPER(&on);
        } break;
    }
    return status;
}

QP::QState Txr::flashing(Txr *const me, QP::QEvt const *const e)
{
    static char led_count = 0;
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            set_LED_status(ENC_RED_LED, LED_ON);
            // set_LED_status(ENC_GREEN_LED, LED_TOGGLE);
            me->flash_timeout_.postEvery(me, FLASH_RATE_TOUT);
            me->calibration_timeout_.postIn(me, FLASH_DURATION_TOUT);
            led_count = 0;
            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            me->flash_timeout_.disarm();
            set_LED_status(ENC_RED_LED, LED_ON);
            // set_LED_status(ENC_GREEN_LED, LED_OFF);
            status = Q_HANDLED();
        } break;
        case CALIBRATION_SIG: {
            // if they've pressed button 2 times calibration should be complete
            if (me->enc_pushes_ >= 2) {
                if (FREESWITCH_ON()) {
                    status = Q_TRAN(&free_run_mode);
                } else if (ZSWITCH_ON()) {
                    status = Q_TRAN(&z_mode);
                } else {
                    status = Q_TRAN(&play_back_mode);
                }
            } else {
                status = Q_TRAN(&uncalibrated);
            }
        } break;
        case FLASH_RATE_SIG: {
            static int red_on = 0;
            analogWrite(SPEED_LED3_1, red_on ? 0xff : 0x00);
            red_on = !red_on;
            status = Q_HANDLED();
        } break;
        case ENC_DOWN_SIG: {
            // here to swallow the encoder press while flashing; else an exception
            // occurs
            status = Q_HANDLED();
        } break;
        default: {
            status = Q_SUPER(&uncalibrated);
        } break;
    }
    return status;
}

QP::QState Txr::free_run_mode(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->double_tapping_ = false;
            me->update_button_LEDs();
            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            set_speed_LEDs_off();
            me->flash_timeout_.disarm();
            status = Q_HANDLED();
        } break;
        case SEND_TIMEOUT_SIG: {
            if (me->encoder_mode_ == ENCODER_MODE_SPEED) {
                me->update_max_speed_using_encoder();
            } else {
                me->update_max_accel_using_encoder();
            }
            me->update_button_LEDs();
            me->update_position();

            status = Q_HANDLED();
        } break;
        case POSITION_BUTTON_SIG: {
            // only save position if finished flashing from previous save
            if (me->flash_timeout_.ctr() == 0) {
                me->prev_position_button_pressed_ =
                    ((PositionButtonEvt *)e)->ButtonNum;
                Q_REQUIRE(me->prev_position_button_pressed_ < NUM_POSITION_BUTTONS);
                me->saved_positions_[me->prev_position_button_pressed_] = me->cur_pos_;
                settings_set_saved_position(me->prev_position_button_pressed_,
                                            (int)me->cur_pos_);
                me->flashing_position_button_ = true;
                me->flash_timeout_.postIn(me, FLASH_RATE_TOUT);
            }

            status = Q_HANDLED();
        } break;
        case FLASH_RATE_SIG: {
            me->flashing_position_button_ = false;
            status = Q_HANDLED();
        } break;
        default: {
            status = Q_SUPER(&calibrated);
        } break;
    }
    return status;
}

QP::QState Txr::play_back_mode(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->double_tapping_ = false;
            me->play_back_target_pos_ = me->cur_pos_;
            PACKET_SEND(PACKET_TARGET_POSITION_SET, target_position_set, me->cur_pos_);
            me->update_button_LEDs();
            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            set_speed_LEDs_off();
            me->flash_timeout_.disarm();
            status = Q_HANDLED();
        } break;
        case SEND_TIMEOUT_SIG: {
            if (me->encoder_mode_ == ENCODER_MODE_SPEED) {
                me->update_max_speed_using_encoder();
            } else {
                me->update_max_accel_using_encoder();
            }

            me->update_button_LEDs();

            me->update_position_play_back();
            status = Q_HANDLED();
        } break;
        case POSITION_BUTTON_SIG: {
            int button_num = ((PositionButtonEvt *)e)->ButtonNum;
            Q_REQUIRE(button_num < NUM_POSITION_BUTTONS);
            me->play_back_target_pos_ = me->saved_positions_[button_num];
            me->prev_position_button_pressed_ = button_num;
            me->flashing_position_button_ = true;
            // disarm first in case a previous flash is in progress
            me->flash_timeout_.disarm(); 
            me->flash_timeout_.postIn(me, FLASH_RATE_TOUT);
            status = Q_HANDLED();
        } break;
        case FLASH_RATE_SIG: {
            me->flashing_position_button_ = false;
            status = Q_HANDLED();
        } break;
        default: {
            status = Q_SUPER(&calibrated);
        } break;
    }
    return status;
}

QP::QState Txr::z_mode(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->double_tapping_ = false;
            set_LED_status(ENC_GREEN_LED, LED_ON);
            set_LED_status(ENC_RED_LED, LED_ON);

            me->reset_calibration();

            status = Q_HANDLED();
        } break;
        case Q_EXIT_SIG: {
            set_speed_LEDs_off();
            status = Q_HANDLED();
        } break;
        case SEND_TIMEOUT_SIG: {
            if (me->encoder_mode_ == ENCODER_MODE_SPEED) {
                me->update_max_speed_using_encoder();
            } else {
                me->update_max_accel_using_encoder();
            }
            me->update_button_LEDs();
            me->update_position();

            int preset_index = settings_get_preset_index();
            if (preset_index != me->previous_preset_index_) {
                set_speed_LED_status(me->previous_preset_index_, LED_OFF);
                set_speed_LED_status(preset_index, LED_ON);
                me->previous_preset_index_ = preset_index;
            }

            status = Q_HANDLED();
        } break;
        case POSITION_BUTTON_SIG: {
            int button_index = ((PositionButtonEvt *)e)->ButtonNum;
            Q_REQUIRE(button_index < NUM_POSITION_BUTTONS);

            log_value(SERIAL_PRESET_INDEX_GET, button_index);
            settings_set_preset_index(button_index);
            me->reset_calibration();
            me->prev_position_button_pressed_ = button_index;
            me->flashing_position_button_ = true;
            // disarm first in case a previous flash is in progress
            me->flash_timeout_.disarm(); 
            me->flash_timeout_.postIn(me, FLASH_RATE_TOUT);

            status = Q_HANDLED();
        } break;
        case FLASH_RATE_SIG: {
            me->flashing_position_button_ = false;
            status = Q_HANDLED();
        } break;
        default: {
            status = Q_SUPER(&calibrated);
        } break;
    }
    return status;
}
