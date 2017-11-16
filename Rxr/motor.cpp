#include "motor.h"
#include "macros.h"

void motor_pulse() {
    STEP_PIN(SET);
    STEP_PIN(CLR);
}

void motor_set_dir_forward() {
    DIR_PIN(SET);
}

void motor_set_dir_backward() {
    DIR_PIN(CLR);
}

void motor_sleep() {
    SLEEP_PIN(CLR);
}

void motor_wake() {
    SLEEP_PIN(SET);
}

void motor_set_steps(int steps) {
    switch (steps) {
        case FULL_STEPS: {
            MS1_PIN(CLR);
            MS2_PIN(CLR);
        } break;
        case HALF_STEPS: {
            MS1_PIN(CLR);
            MS2_PIN(SET);
        } break;
        case QUARTER_STEPS: {
            MS1_PIN(SET);
            MS2_PIN(CLR);
        } break;
        case EIGHTH_STEPS: {
            MS1_PIN(SET);
            MS2_PIN(SET);
        } break;
    }
}
