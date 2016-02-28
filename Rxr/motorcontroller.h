#ifndef lenzhound_motor_controller_h
#define lenzhound_motor_controller_h
#include "motor.h"
#include "constants.h"

namespace lh {

class MotorController {
public:
  MotorController();
  void configure(long accel, long max_velocity, long z_accel, long z_velocity);
  void run();
  void move_to_position(long position);
  void initialize_position(long position);
  void set_velocity_percent(int velocity);
  void set_accel_percent(int accel);
  void set_velocity(long velocity);
  void set_accel(long accel);
  void set_z_velocity(long velocity);
  void set_z_accel(long accel);
  long get_velocity();
  long get_accel();
  long get_z_velocity();
  long get_z_accel();
  void set_mode(int mode);
  void refresh_accel();
  void refresh_velocity();
  bool is_position_initialized();

private:
  bool direction_;
  int mode_;
  long max_velocity_;
  long current_velocity_cap_;
  long accel_;
  long max_accel_;
  long z_max_accel_;
  long decel_denominator_;
  long max_decel_denominator_;
  long velocity_;
  long z_max_velocity_;
  long calculated_position_;
  long motor_position_;
  long observed_position_;
  long run_count_;
  long sleeping_;
  bool initial_position_set_;
  int velocity_percent_;
  int accel_percent_;

  long get_decel_threshold();
  bool try_sleep();
  void sleep();
  void wake_up();
};

}
#endif //lenzhound_motor_controller_h
