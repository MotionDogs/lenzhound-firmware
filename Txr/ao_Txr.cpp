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
#include "EncVelManager.h"
#include "Arduino.h" // if I start having problems with RF24, consider removing this
#include "serial_api.h"
#include "console.h"
#include "settings.h"
#include "static.h"

Q_DEFINE_THIS_FILE


// various timeouts in ticks
enum TxrTimeouts {
    SEND_ENCODER_TOUT  = BSP_TICKS_PER_SEC / 100,   // how often to send encoder position
    FLASH_RATE_TOUT = BSP_TICKS_PER_SEC / 2,        // how quick to flash LED
    FLASH_DURATION_TOUT = BSP_TICKS_PER_SEC * 2,    // how long to flash LED for
    ENTER_CALIBRATION_TOUT = BSP_TICKS_PER_SEC * 2, // how long to hold calibration button before reentering calibration
    ALIVE_DURATION_TOUT = BSP_TICKS_PER_SEC * 5     // how often to check that transmitter is still powered (in case of low battery)
};

// todo: move this somewhere else or do differently
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


class Txr :
    public QP::QActive {
private:
QTimeEvt mAliveTimeout;
QTimeEvt mFlashTimeout;
QTimeEvt mSendTimeout;
QTimeEvt mCalibrationTimeout;
float mCurPos;    // float to save partial moves needed by encoder resolution division
long mPrevPos1;   // used to prevent jitter
long mPrevPos2;   // used to prevent jitter
long mPrevEncoderCnt;
long mCalibrationPos1;
long mCalibrationPos2;
char mEncPushes;
char mCalibrationMultiplier;
long mSavedPositions[NUM_POSITION_BUTTONS];
EncVelManager mVelocityManager;
unsigned char mPrevPositionButtonPressed;
char mZModeSavedVelocity;
char mZModeSavedAcceleration;

public:
Txr() :
    QActive((QStateHandler) & Txr::initial),
    mFlashTimeout(FLASH_RATE_SIG), mSendTimeout(SEND_TIMEOUT_SIG),
    mCalibrationTimeout(CALIBRATION_SIG), mAliveTimeout(ALIVE_SIG)
{
}

protected:
static QP::QState initial(Txr *const me, QP::QEvt const *const e);
static QP::QState on(Txr *const me, QP::QEvt const *const e);
static QP::QState uncalibrated(Txr *const me, QP::QEvt const *const e);
static QP::QState calibrated(Txr *const me, QP::QEvt const *const e);
static QP::QState flashing(Txr *const me, QP::QEvt const *const e);
static QP::QState freeRun(Txr *const me, QP::QEvt const *const e);
static QP::QState playBack(Txr *const me, QP::QEvt const *const e);
static QP::QState zmode(Txr *const me, QP::QEvt const *const e);
void UpdatePosition(Txr *const me);
void UpdatePositionCalibration(Txr *const me);
void UpdatePositionZMode(Txr *const me);
void UpdatePositionPlayBack(Txr *const me);
void UpdateCalibrationMultiplier(int setting);
void LogValue(char key, long value);
void SetLEDStatus(int led, int status);
void QueueRadioValue(char key, long value);
};


static Txr l_Txr;                   // the single instance of Txr active object (local)
QActive *const AO_Txr = &l_Txr;     // the global opaque pointer

enum {
    LED_OFF,
    LED_ON,
    LED_TOGGLE
};

void Txr::SetLEDStatus(int led, int status)
{
    if (status == LED_ON) {
        switch (led) {
        case SPEED_LED1: {
            RED_LED_ON();
        } break;
        case SPEED_LED2: {
            AMBER_LED_ON();
        } break;
        case SPEED_LED3: {
            AMBER2_LED_ON();
        } break;
        case SPEED_LED4: {
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
        case SPEED_LED3: {
            AMBER2_LED_OFF();
        } break;
        case SPEED_LED4: {
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
        case SPEED_LED3: {
            AMBER2_LED_TOGGLE();
        } break;
        case SPEED_LED4: {
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

void Txr::LogValue(char key, long value)
{
    char buffer[32];
    sprintf(buffer, "%c=%d", key, value);

    serial_api_queue_output(
        get_serial_api_state(), SERIAL_API_SRC_CONSOLE, buffer);
}

void Txr::QueueRadioValue(char type, long value)
{
    radio_packet_t packet;
    // BSP_assert(value < (1 << 15) && value > (-1 << 15));
    int val16 = (int)value;
    packet.typed_val.type = type;
    packet.typed_val.val = val16;
    radio_queue_message(get_radio_state(), packet);
}

void Txr::UpdatePositionCalibration(Txr *const me)
{
    long curEncoderCnt = BSP_get_encoder();

    if (curEncoderCnt != me->mPrevEncoderCnt) {
        LogValue(SERIAL_API_CMD_GET_ENCODER, curEncoderCnt);
    }

    // since it's 4 counts per detent, let's make it a detent per motor count
    // this is a test, remove if not necessary
    float amountToMove = curEncoderCnt - me->mPrevEncoderCnt;
    amountToMove /= 4.0;
    amountToMove *= me->mCalibrationMultiplier;
    me->mCurPos += amountToMove; //(curEncoderCnt - me->mPrevEncoderCnt) * me->mCalibrationMultiplier;
    me->mPrevEncoderCnt = curEncoderCnt;

    me->QueueRadioValue(PACKET_SET_TARGET_POSITION, me->mCurPos);
}

void Txr::UpdatePosition(Txr *const me)
{
    long newPos = BSP_get_pot();

    // only update the current position if it's not jittering between two values
    if (newPos != me->mPrevPos1 && newPos != me->mPrevPos2) {
        LogValue(SERIAL_API_CMD_GET_POT, newPos);

        me->mPrevPos1 = me->mPrevPos2;
        me->mPrevPos2 = newPos;
        me->mCurPos = map(newPos, MIN_POT_VAL, MAX_POT_VAL,
                          me->mCalibrationPos1, me->mCalibrationPos2);
    }

    // this still needs to be updated and sent so that velocity changes happen (for ZMode) and possibly other cases

    me->QueueRadioValue(PACKET_SET_TARGET_POSITION, me->mCurPos);
}

void Txr::UpdatePositionZMode(Txr *const me)
{
    me->QueueRadioValue(PACKET_SET_VELOCITY_PERCENT, me->mVelocityManager.GetVelocityPercent());

    me->mVelocityManager.SetLEDs(false);
    me->UpdatePosition(me);
}

void Txr::UpdatePositionPlayBack(Txr *const me)
{
    me->QueueRadioValue(PACKET_SET_TARGET_POSITION, me->mCurPos);
    me->QueueRadioValue(PACKET_SET_VELOCITY_PERCENT, me->mVelocityManager.GetVelocityPercent());
    me->mVelocityManager.SetLEDs(false);
}

void Txr::UpdateCalibrationMultiplier(int setting)
{
    switch (setting) {
    case PLAYBACK_MODE:
        mCalibrationMultiplier = 40;
        break;
    case Z_MODE:
        mCalibrationMultiplier = 80;
        break;
    case FREE_MODE:
    default:
        mCalibrationMultiplier = 8;
        break;
    }
}

QP::QState Txr::initial(Txr *const me, QP::QEvt const *const e)
{
    me->mCalibrationMultiplier = 1;
    me->mCurPos = 0;
    me->mPrevEncoderCnt = 0;
    me->QueueRadioValue(PACKET_SET_TARGET_POSITION, 0);
    me->mZModeSavedVelocity = 50;
    me->mZModeSavedAcceleration = 100;
    me->mPrevPos1 = -1;
    me->mPrevPos2 = -1;
    me->subscribe(ENC_DOWN_SIG);
    me->subscribe(ENC_UP_SIG);
    me->subscribe(PLAY_MODE_SIG);
    me->subscribe(FREE_MODE_SIG);
    me->subscribe(Z_MODE_SIG);
    me->subscribe(POSITION_BUTTON_SIG);
    me->subscribe(UPDATE_PARAMS_SIG);
    me->mSendTimeout.postEvery(me, SEND_ENCODER_TOUT);
    me->mAliveTimeout.postEvery(me, ALIVE_DURATION_TOUT);
    me->mCalibrationPos1 = settings_get_calibration_position_1();
    me->mCalibrationPos2 = settings_get_calibration_position_2();

    if (!settings_get_start_in_calibration_mode()) {
        for (int i = 0; i < NUM_POSITION_BUTTONS; i++) {
            me->mSavedPositions[i] = settings_get_saved_position(i);
        }

        if (FREESWITCH_ON()) {
            return Q_TRAN(&freeRun);
        } else if (ZSWITCH_ON()) {
            return Q_TRAN(&zmode);
        } else {
            return Q_TRAN(&playBack);
        }
    }
    return Q_TRAN(&uncalibrated);
}

QP::QState Txr::on(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        status_ = Q_HANDLED();
        break;
    }
    case ALIVE_SIG:
    {
        if (radio_is_alive()) {
            me->SetLEDStatus(GREEN_LED, LED_OFF);
        } else {
            me->SetLEDStatus(GREEN_LED, LED_ON);
        }
        status_ = Q_HANDLED();
        break;
    }
    case UPDATE_PARAMS_SIG:
    {
        int channel = settings_get_channel();
        radio_set_channel(channel);
        status_ = Q_HANDLED();
        break;
    }
    default:
    {
        status_ = Q_SUPER(&QP::QHsm::top);
        break;
    }
    }
    return status_;
}

QP::QState Txr::uncalibrated(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        me->SetLEDStatus(ENC_RED_LED, LED_ON);
        me->SetLEDStatus(ENC_GREEN_LED, LED_OFF);
        me->Txr::QueueRadioValue(PACKET_SET_MODE, FREE_MODE);
        me->mPrevEncoderCnt = BSP_get_encoder();
        me->UpdateCalibrationMultiplier(BSP_get_mode());
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        status_ = Q_HANDLED();
        break;
    }
    case SEND_TIMEOUT_SIG:
    {
        me->UpdatePositionCalibration(me);
        status_ = Q_HANDLED();
        break;
    }
    case ENC_DOWN_SIG:
    {
        // if this is first time button press, just save the position
        if ((me->mEncPushes)++ == 0) {
            me->mCalibrationPos1 = me->mCurPos;
            settings_set_calibration_position_1(me->mCalibrationPos1);
        }
        // if this is second time, determine whether swapping is necessary
        // to map higher calibrated position with higher motor position
        else {
            me->mCalibrationPos2 = me->mCurPos;
            settings_set_calibration_position_2(me->mCalibrationPos2);
        }
        status_ = Q_TRAN(&flashing);
        break;
    }
    case PLAY_MODE_SIG:
    {
        me->UpdateCalibrationMultiplier(PLAYBACK_MODE);
        status_ = Q_HANDLED();
        break;
    }
    case Z_MODE_SIG:
    {
        me->UpdateCalibrationMultiplier(Z_MODE);
        status_ = Q_HANDLED();
        break;
    }
    case FREE_MODE_SIG:
    {
        me->UpdateCalibrationMultiplier(FREE_MODE);
        status_ = Q_HANDLED();
        break;
    }
    default:
    {
        status_ = Q_SUPER(&on);
        break;
    }
    }
    return status_;
}

QP::QState Txr::calibrated(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        status_ = Q_HANDLED();
        break;
    }
    case ENC_DOWN_SIG:
    {
        // reenter calibration if held down for 2 seconds
        me->mCalibrationTimeout.postIn(me, ENTER_CALIBRATION_TOUT);
        status_ = Q_HANDLED();
        break;
    }
    case ENC_UP_SIG:
    {
        // if they released the button before the time is up, stop waiting for the timeout
        me->mCalibrationTimeout.disarm();
        status_ = Q_HANDLED();
        break;
    }
    case CALIBRATION_SIG:
    {
        // we've held for 2 seconds, go to calibration
        me->mEncPushes = 0;
        status_ = Q_TRAN(&flashing);
        break;
    }
    case PLAY_MODE_SIG:
    {
        status_ = Q_TRAN(&playBack);
        break;
    }
    case Z_MODE_SIG:
    {
        status_ = Q_TRAN(&zmode);
        break;
    }
    case FREE_MODE_SIG:
    {
        status_ = Q_TRAN(&freeRun);
        break;
    }
    default:
    {
        status_ = Q_SUPER(&on);
        break;
    }
    }
    return status_;
}

QP::QState Txr::flashing(Txr *const me, QP::QEvt const *const e)
{
    static char ledCnt = 0;
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        me->SetLEDStatus(ENC_RED_LED, LED_ON);
        me->SetLEDStatus(ENC_GREEN_LED, LED_TOGGLE);
        me->mFlashTimeout.postEvery(me, FLASH_RATE_TOUT);
        me->mCalibrationTimeout.postIn(me, FLASH_DURATION_TOUT);
        ledCnt = 0;
        /*if (FREESWITCH_ON()) {
         *  me->UpdatePosition(me);
         * }
         * else if (ZSWITCH_ON()) {
         *  me->UpdatePositionZMode(me);
         * }
         * else {
         *  me->UpdatePositionPlayBack(me);
         * }*/
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        me->mFlashTimeout.disarm();
        me->SetLEDStatus(ENC_RED_LED, LED_ON);
        me->SetLEDStatus(ENC_GREEN_LED, LED_OFF);
        status_ = Q_HANDLED();
        break;
    }
    case CALIBRATION_SIG:
    {
        // if they've pressed button 2 times calibration should be complete
        if (me->mEncPushes >= 2) {
            if (FREESWITCH_ON()) {
                status_ = Q_TRAN(&freeRun);
            } else if (ZSWITCH_ON()) {
                status_ = Q_TRAN(&zmode);
            } else {
                status_ = Q_TRAN(&playBack);
            }
        } else {
            status_ = Q_TRAN(&uncalibrated);
        }
        break;
    }
    case FLASH_RATE_SIG:
    {
        me->SetLEDStatus(ENC_GREEN_LED, LED_TOGGLE);
        status_ = Q_HANDLED();
        break;
    }
    case ENC_DOWN_SIG:
    {
        // here to swallow the encoder press while flashing; else an exception occurs
        status_ = Q_HANDLED();
        break;
    }
    default:
    {
        status_ = Q_SUPER(&uncalibrated);
        break;
    }
    }
    return status_;
}

QP::QState Txr::freeRun(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        me->SetLEDStatus(ENC_RED_LED, LED_ON);
        analogWrite(ENC_GREEN_LED, 15);

        me->QueueRadioValue(PACKET_SET_MODE, FREE_MODE);
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        status_ = Q_HANDLED();
        break;
    }
    case SEND_TIMEOUT_SIG:
    {
        me->UpdatePosition(me);
        status_ = Q_HANDLED();
        break;
    }
    case POSITION_BUTTON_SIG:
    {
        // only save position if finished flashing from previous save
        if (me->mFlashTimeout.ctr() == 0) {
            me->mPrevPositionButtonPressed =
                ((PositionButtonEvt *)e)->ButtonNum;
            Q_REQUIRE(me->mPrevPositionButtonPressed < NUM_POSITION_BUTTONS);
            me->mSavedPositions[me->mPrevPositionButtonPressed] = me->mCurPos;
            settings_set_saved_position((int)me->mCurPos,
                                        me->mPrevPositionButtonPressed);
            BSP_turn_on_speed_LED(me->mPrevPositionButtonPressed);
            me->mFlashTimeout.postIn(me, FLASH_RATE_TOUT);
        }

        status_ = Q_HANDLED();
        break;
    }
    case FLASH_RATE_SIG:
    {
        // turn off flashed LED
        BSP_turn_off_speed_LED(me->mPrevPositionButtonPressed);
        status_ = Q_HANDLED();
        break;
    }
    default:
    {
        status_ = Q_SUPER(&calibrated);
        break;
    }
    }
    return status_;
}

QP::QState Txr::playBack(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        me->SetLEDStatus(ENC_GREEN_LED, LED_ON);
        me->SetLEDStatus(ENC_RED_LED, LED_ON);
        me->QueueRadioValue(PACKET_SET_MODE, PLAYBACK_MODE);
        me->QueueRadioValue(PACKET_SET_TARGET_POSITION, me->mCurPos);
        me->mVelocityManager.Init(50); // init at 50% speed
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        me->mVelocityManager.SetAllLEDsOff();
        status_ = Q_HANDLED();
        break;
    }
    case SEND_TIMEOUT_SIG:
    {
        me->UpdatePositionPlayBack(me);
        status_ = Q_HANDLED();
        break;
    }
    case POSITION_BUTTON_SIG:
    {
        int buttonNum = ((PositionButtonEvt *)e)->ButtonNum;
        Q_REQUIRE(buttonNum < NUM_POSITION_BUTTONS);
        me->mCurPos = me->mSavedPositions[buttonNum];
        status_ = Q_HANDLED();
        break;
    }
    default:
    {
        status_ = Q_SUPER(&calibrated);
        break;
    }
    }
    return status_;
}

QP::QState Txr::zmode(Txr *const me, QP::QEvt const *const e)
{
    QP::QState status_;

    switch (e->sig) {
    case Q_ENTRY_SIG:
    {
        me->SetLEDStatus(ENC_GREEN_LED, LED_ON);
        me->SetLEDStatus(ENC_RED_LED, LED_ON);

        me->QueueRadioValue(PACKET_SET_MODE, Z_MODE);
        me->QueueRadioValue(PACKET_SET_TARGET_POSITION, me->mCurPos);
        me->QueueRadioValue(PACKET_SET_ACCEL_PERCENT, me->mZModeSavedAcceleration);
        me->mVelocityManager.Init(me->mZModeSavedVelocity);
        status_ = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG:
    {
        me->mVelocityManager.SetAllLEDsOff();
        me->mZModeSavedVelocity = me->mVelocityManager.GetVelocityPercent();
        status_ = Q_HANDLED();
        break;
    }
    case SEND_TIMEOUT_SIG:
    {
        me->UpdatePositionZMode(me);
        status_ = Q_HANDLED();
        break;
    }
    case POSITION_BUTTON_SIG:
    {
        int buttonNum = ((PositionButtonEvt *)e)->ButtonNum;
        Q_REQUIRE(buttonNum < NUM_POSITION_BUTTONS);
        me->mZModeSavedAcceleration = (buttonNum + 1) * 25; // 25, 50, 75, or 100%
        me->QueueRadioValue(PACKET_SET_ACCEL_PERCENT, me->mZModeSavedAcceleration);
        status_ = Q_HANDLED();
        break;
    }
    default:
    {
        status_ = Q_SUPER(&calibrated);
        break;
    }
    }
    return status_;
}
