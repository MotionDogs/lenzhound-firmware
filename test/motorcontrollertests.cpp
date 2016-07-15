#include <vector>
#include "gtest/gtest.h"
#include "motorcontroller.h"
#include "util.h"
#include "constants.h"

struct MotorPulse { long run_count, direction; };
struct MotorContext {
  long boundary_, position_, direction_, run_count_;
  std::vector<MotorPulse> pulses_;
};
static MotorContext context = {};

void reset_context() {
  context.boundary_ = 0;
  context.position_ = 0;
  context.run_count_ = 0;
  context.direction_ = 1;
  context.pulses_.clear();
}

void motor_pulse() {
  context.position_ += context.direction_;
  ASSERT_LE(context.position_, context.boundary_);
  MotorPulse pulse = { context.run_count_, context.direction_ };
  context.pulses_.push_back(pulse);
}
void motor_set_dir_forward() {
  context.direction_ = 1;
}
void motor_set_dir_backward() {
  context.direction_ = -1;
}
void motor_sleep() {
}
void motor_wake() {
}
void motor_set_steps(int steps) {
}

TEST(MotorController, HitsItsTarget) {
  reset_context();
  lh::MotorController controller = lh::MotorController();
  long target = 5000;

  controller.set_accel(50);
  controller.set_velocity(6000);
  controller.set_velocity_percent(100);
  controller.move_to_position(i32_to_fixed(target));

  // make sure we don't go over target
  context.boundary_ = target;
  for (int i = 0; i < 35000; ++i) {
    controller.run();
  }
  EXPECT_EQ(context.position_, target);
}

TEST(MotorController, HandlesSlowSpeeds) {
  reset_context();
  lh::MotorController controller = lh::MotorController();
  long target = 300;

  controller.set_accel(50);
  controller.set_velocity(1);
  controller.set_velocity_percent(100);
  controller.move_to_position(i32_to_fixed(target));

  // make sure we don't go over target
  context.boundary_ = target;
  for (int i = 0; i < 10000000; ++i) {
    controller.run();
  }
  EXPECT_EQ(context.position_, target);
}

TEST(MotorController, DoesNotChangeConcavity) {
  reset_context();
  lh::MotorController controller = lh::MotorController();
  long target = 6000;

  controller.set_accel(32);
  controller.set_velocity(48000);
  controller.set_velocity_percent(100);
  controller.set_accel_percent(100);
  controller.move_to_position(i32_to_fixed(target));

  context.boundary_ = target;

  for (int i = 0; i < 60000; ++i) {
    controller.run();
    ++context.run_count_;
  }

  EXPECT_EQ(context.position_, target);

  std::vector<MotorPulse> pulses = context.pulses_;
  long prev_delta = pulses[1].run_count - pulses[0].run_count;
  bool decel_begun = false;
  for (int i = 2; i < pulses.size(); ++i) {
    long next_delta = pulses[i].run_count - pulses[i - 1].run_count;
    // std::cout << next_delta << "\n"; // can chart using vanilla csv graphing tools

    if (!decel_begun) {
      // allow for the previous delta to be a little bit bigger because
      // of the digital/stepwise nature of the algorithm
      decel_begun = next_delta > prev_delta + 1;
    } else {
      // we only allow for one change of concavity - i.e. once we start
      // decelerating, we shouldn't start accelerating again
      ASSERT_GE(next_delta, prev_delta - 1);
    }

    prev_delta = next_delta;
  }
}