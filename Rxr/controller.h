#ifndef lenzhound_motor_controller_h
#define lenzhound_motor_controller_h
#include "motor.h"
#include "constants.h"

struct controller_state_t {
  bool direction;
  int mode;
  long max_speed;
  long current_speed_cap;
  long accel;
  long max_accel;
  long decel_denominator;
  long max_decel_denominator;
  long velocity;
  long calculated_position;
  long motor_position;
  long target_position;
  long run_count;
  long sleeping;
  bool initial_position_set;
  int speed_percent;
  int accel_percent;
};

void controller_init();
void controller_run();
void controller_move_to_position(long position);
void controller_initialize_position(long position);
void controller_set_speed_percent(int speed);
void controller_set_accel_percent(int accel);
void controller_set_speed(long speed);
void controller_set_accel(long accel);
long controller_get_target_position();
long controller_get_speed();
long controller_get_accel();
void controller_set_mode(int mode);
void controller_refresh_accel();
void controller_refresh_speed();
bool controller_is_position_initialized();

#endif //lenzhound_motor_controller_h
