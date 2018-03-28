#include "motor.h"
#include "macros.h"

#define NOP __asm__ __volatile__ ("nop\n\t")

void motor_run(long motor_speed){
  DCSPEED_PIN(SET); // implement PWM for acceleration
}

void motor_stop(){
  DCSPEED_PIN(CLR);
}

void motor_set_dir_forward() {
  DCIN1_PIN(SET); // dcdvr
  DCIN2_PIN(CLR);
}

void motor_set_dir_backward() {
  DCIN1_PIN(CLR); // dcdvr
  DCIN2_PIN(SET);
}
