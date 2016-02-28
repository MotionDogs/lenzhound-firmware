#include "motorcontroller.h"
#include "constants.h"
#include "util.h"
#include "Arduino.h"

#define CONVERSION_FACTOR .01

namespace lh {
MotorController::MotorController() :
    direction_(1),
    max_velocity_(0),
    current_velocity_cap_(0),
    accel_(0),
    mode_(0),
    decel_denominator_(0),
    velocity_(0),
    calculated_position_(0),
    motor_position_(0),
    observed_position_(0),
    run_count_(0),
    sleeping_(true),
    initial_position_set_(false)
{
}

bool MotorController::is_position_initialized()
{
    return initial_position_set_;
}

void MotorController::initialize_position(long position)
{
    motor_position_ = position;
    calculated_position_ = position;
    observed_position_ = position;
    initial_position_set_ = true;
}

void MotorController::move_to_position(long position)
{
    if (position != observed_position_) {
        wake_up();
        run_count_ = 0;
    }
    observed_position_ = position;
}

void MotorController::set_mode(int mode)
{
    mode_ = mode;
}

void MotorController::refresh_velocity()
{
    long max = (mode_ == Z_MODE) ? z_max_velocity_ : max_velocity_;
    current_velocity_cap_ = min32(
        max32(max * velocity_percent_ * CONVERSION_FACTOR, 1L),
        FIXED_ONE);
}

void MotorController::refresh_accel()
{
    if (mode_ == Z_MODE) {
        accel_ = max32(z_max_accel_ * accel_percent_ * CONVERSION_FACTOR, 1L);
    } else {
        accel_ = max32(max_accel_ * accel_percent_ * CONVERSION_FACTOR, 1L);
    }
    decel_denominator_ = fixed_mult(accel_, i32_to_fixed(2L));
}


void MotorController::set_velocity_percent(int velocity)
{
    velocity_percent_ = velocity;
    refresh_velocity();
}

void MotorController::set_accel_percent(int accel)
{
    accel_percent_ = accel;
    refresh_accel();
}

void MotorController::set_velocity(long velocity)
{
    max_velocity_ = velocity;
    refresh_velocity();
}

void MotorController::set_accel(long accel)
{
    max_accel_ = accel;
    refresh_accel();
}

void MotorController::set_z_velocity(long velocity)
{
    z_max_velocity_ = velocity;
    refresh_velocity();
}

void MotorController::set_z_accel(long accel)
{
    z_max_accel_ = accel;
    refresh_accel();
}

long MotorController::get_velocity()
{
    return max_velocity_;
}

long MotorController::get_accel()
{
    return max_accel_;
}

long MotorController::get_z_velocity()
{
    return z_max_velocity_;
}

long MotorController::get_z_accel()
{
    return z_max_accel_;
}

long MotorController::get_decel_threshold()
{
    return fixed_div(fixed_mult(velocity_, velocity_), decel_denominator_);
}

void MotorController::sleep()
{
    sleeping_ = true;
    run_count_ = 0;
    sleep_motor();
}

void MotorController::wake_up()
{
    sleeping_ = false;
    wake_motor();
}

bool MotorController::try_sleep()
{
    if (motor_position_ != observed_position_) {
        run_count_ = 0;
        return false;
    }
    if (sleeping_) {
        return true;
    }
    if (run_count_ > MOTOR_SLEEP_THRESHOLD) {
        sleep();
        return true;
    }
    run_count_++;
    return false;
}

void MotorController::run()
{
    if (try_sleep()) {
        return;
    }
    long steps_to_go = abs32(observed_position_ - calculated_position_);
    if (direction_) {
        if ((calculated_position_ > observed_position_) ||
            (steps_to_go <= get_decel_threshold())) {
            velocity_ -= accel_;
        } else if (calculated_position_ < observed_position_) {
            velocity_ = min32(velocity_ + accel_, current_velocity_cap_);
        }
        calculated_position_ += velocity_;
        if ((motor_position_ < calculated_position_) &&
            (motor_position_ != observed_position_)) {
            motor_position_ += FIXED_ONE;
            pulse_motor();
        }
        if (velocity_ < 0) {
            direction_ = 0;
            set_motor_dir_backward();
        }
    } else {
        if ((calculated_position_ < observed_position_) ||
            (steps_to_go <= get_decel_threshold())) {
            velocity_ += accel_;
        } else if (calculated_position_ > observed_position_) {
            velocity_ = max32(velocity_ - accel_, -current_velocity_cap_);
        }
        calculated_position_ += velocity_;
        if (motor_position_ > calculated_position_ &&
            motor_position_ != observed_position_) {
            motor_position_ -= FIXED_ONE;
            pulse_motor();
        }
        if (velocity_ > 0) {
            direction_ = 1;
            set_motor_dir_forward();
        }
    }
}
}
