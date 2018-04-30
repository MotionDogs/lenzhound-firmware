#ifndef lenzhound_motor_h
#define lenzhound_motor_h

enum {
    EIGHTH_STEPS,
    QUARTER_STEPS,
    HALF_STEPS,
    FULL_STEPS
};

void motor_pulse();
void motor_set_dir_forward();
void motor_set_dir_backward();
void motor_set_steps(int steps);
void motor_trq1_on();
void motor_trq1_off();
void motor_driver_a3967();
void motor_driver_drv8880();
void motor_sleep_a3967();
void motor_sleep_drv8880();
void motor_wake_a3967();
void motor_wake_drv8880();

#endif //lenzhound_motor_h
