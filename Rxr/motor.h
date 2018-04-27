#ifndef lenzhound_motor_h
#define lenzhound_motor_h

#include <Encoder.h>
#include <PID_v1.h>

void motor_run();
void motor_stop();
void move_to(long new_position);
long getActualPosition();
void setAccuracy(unsigned int);  
void setCurrentPosition(long);   // Override encoder value
void motor_set_dir_forward();
void motor_set_dir_backward();

#endif //lenzhound_motor_h
