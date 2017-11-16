#include "motor.h"
#include "macros.h"

#define NOP __asm__ __volatile__ ("nop\n\t")

void motor_pulse() {
  STEP_PIN(SET); // easydriver
  STEP_PIN(CLR);

  STEP2_PIN(SET); // drv8880
  NOP;NOP; // this seems to smooth out the steps
  STEP2_PIN(CLR);    
}

void motor_set_dir_forward() {
  DIR_PIN(SET); // easydriver
  DIR2_PIN(SET); // drv8880
}

void motor_set_dir_backward() {
  DIR_PIN(CLR); // easydriver
  DIR2_PIN(CLR); // drv8880
}

void motor_sleep() {
  SLEEP_PIN(CLR); // easydriver
  SLEEP2_PIN(CLR); // drv8880
}

void motor_wake() {
  SLEEP_PIN(SET); // easydriver
  SLEEP2_PIN(SET); // drv8880
}

// easydriver microsteps
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
