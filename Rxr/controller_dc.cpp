#if !IS_STEPPER_MOTOR
#include "controller.h"
#include "constants.h"
#include "util.h"
#include <Encoder.h>
#include <PID_v1.h>
#include <TimerThree.h>

#define pin_dcmoto_dir1 11
#define pin_dcmoto_dir2 12
#define pin_dcmoto_pwm_out 14
#define pin_dcmoto_encode1 5
#define pin_dcmoto_encode2 6

#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))


// Encoder and PID configuration
Encoder encPos(pin_dcmoto_encode1, pin_dcmoto_encode2);
uint8_t pwmOut = 0;
uint8_t pwmSkip = 100;
int positionAccuracy = 30;
double PIDIn = encPos.read(), PIDOut = 0, PIDSetpoint = PIDIn, oldPos = 0;

double Kp=0.1, Ki=0.2, Kd=0.1; // tune this
PID myPID(&PIDIn, &PIDOut, &PIDSetpoint, Kp, Ki, Kd, DIRECT);


controller_state_t state = {0};

void controller_init()
{
    state.direction = 1;
    state.max_speed = 0;
    state.accel = 0;
    state.mode = 0;
    state.decel_denominator = 0;
    state.velocity = 0;
    state.calculated_position = 0;
    state.motor_position = 0;
    state.target_position = 0;
    state.run_count = 0;
    state.sleeping = true;
    state.initial_position_set = false;
    state.current_level = 0;
    //state.real_position = 0;

    Encoder encPos(pin_dcmoto_encode1, pin_dcmoto_encode2);
    int pwmOut = 0;
    int pwmSkip = 50;
    double PIDIn = encPos.read(), PIDOut = 0, PIDSetpoint = PIDIn;
    
    double Kp=0.1, Ki=0.2, Kd=0.1; // tune this
    PID myPID(&PIDIn, &PIDOut, &PIDSetpoint, Kp, Ki, Kd, DIRECT);
    
    myPID.SetSampleTime(50);
    myPID.SetOutputLimits(pwmSkip-255, 255-pwmSkip);

    myPID.SetMode(AUTOMATIC);

}

void controller_uninitialize_position()
{
    state.initial_position_set = false;
}

bool controller_is_position_initialized()
{
    return state.initial_position_set;
}

void controller_initialize_position(long position)
{
    state.motor_position = position;
    state.calculated_position = position;
    state.target_position = position;
    state.initial_position_set = true;
}

void controller_move_to_position(long position)
{
    //position = i32_to_fixed(position);
    if (position != state.target_position) {
        state.run_count = 0;
    }
    state.target_position = position;
}

long controller_get_target_position()
{
    return state.target_position;
    //return fixed_to_i32(state.target_position);
}

void controller_set_mode(int mode)
{
    state.mode = mode;
}

void controller_set_speed(long speed)
{
    state.max_speed = clamp32(speed, 1L, FIXED_ONE);
}

void controller_set_accel(long accel)
{
    state.accel = clamp32(accel, 1L, 256L);
    state.decel_denominator = fixed_mult(state.accel, i32_to_fixed(2L));
}

long controller_get_speed()
{
    return state.max_speed;
}

long controller_get_accel()
{
    return state.accel;
}

long controller_get_decel_threshold()
{
    return fixed_div(
        fixed_mult(state.velocity, state.velocity),
        state.decel_denominator);
}

unsigned int controller_get_current_level() {
    return state.current_level;
}

void controller_set_current_level(unsigned int current_level)
{
    state.current_level = current_level;
    if(state.current_level == 0)
    {
    //  motor_trq1_off();
    } else {
    //  motor_trq1_on();
    }
}

void controller_run()
{
    //long steps_to_go = abs32(state.target_position - state.calculated_position);
    long steps_to_go = abs32(state.target_position - encPos.read());

    PIDIn = encPos.read();

    if(steps_to_go > positionAccuracy + PIDIn)
    {
      myPID.Compute();
      pwmOut = abs(PIDOut) + pwmSkip;   
      if(pwmOut != oldPos){
        oldPos = pwmOut;
        //TCCR3A |= _BV(COM3A1);
        //sbi(TCCR3A, COM3A1);
        //OCR3A = 255;
        analogWrite(pin_dcmoto_pwm_out,pwmOut);
        Serial.println(steps_to_go); 
      }
    }

    if(abs(PIDSetpoint - PIDIn) < positionAccuracy)
    {
      myPID.SetMode(MANUAL);
      myPID.SetMode(AUTOMATIC);
      PIDOut = 0;
      pwmOut = 0;
    } else {
      myPID.SetMode(AUTOMATIC);
    }

    // Set direction
    if (PIDOut < 0)
    {
      motor_set_dir_forward();
    } else {
      motor_set_dir_backward();
    }

    PIDSetpoint = state.target_position;

    

//    if ((state.calculated_position > state.target_position) ||
//       (steps_to_go <= controller_get_decel_threshold())) {
//    
//       state.velocity -= state.accel;
//    
//    } else if (state.calculated_position < state.target_position) {
//    
//       state.velocity = min32(state.velocity + state.accel,
//    
//           state.max_speed);
//    }
//    
//    state.calculated_position += state.velocity;
//    
//    if (state.motor_position < state.calculated_position &&
//       state.motor_position != state.target_position) {
//    
//       state.motor_position += FIXED_ONE;
//    
//       analogWrite(pin_dcmoto_pwm_out, pwmOut);
//    }
//    
//    if (state.velocity < 0) {
//    
//       state.direction = 0;
//    
//       motor_set_dir_backward();
//    }    
    

//    servo.run();

    //if(servo.finished())
    //{
//      if(steps_to_go >= 20){
//        if(servo.finished()){
//        servo.move(state.target_position);
//        }
//        Serial.print(servo.getActualPosition());
//        Serial.print(" Steps Left:");
//        Serial.println(steps_to_go);
//      } else {
//        servo.stop();
        //motor_stop();
     // }
    //}

// //  NOTE(doug): inverted elements are highlighted
// //  CASE: POSITIVE -------------------------------------------------------------
//     if (state.direction) {
// //                                     |
// //                                     v
//         if ((state.calculated_position > state.target_position) ||
//             (steps_to_go <= controller_get_decel_threshold())) {
// //                         |
// //                         v
//             state.velocity -= state.accel;
// //                                           |
// //                                           v
//         } else if (state.calculated_position < state.target_position) {
// //                             |                  |
// //                             v                  v
//             state.velocity = min32(state.velocity + state.accel,
// //              |
// //              v
//                 state.max_speed);
//         }
//
//         state.calculated_position += state.velocity;
// //                               |
// //                               v
//         if (state.motor_position < state.calculated_position &&
//             state.motor_position != state.target_position) {
// //                               |
// //                               v
//             state.motor_position += FIXED_ONE;
//
//             servo.move(state.target_position);
//         }
// //                         |
// //                         v
//         if (state.velocity < 0) {
// //                            |
// //                            v
//             state.direction = 0;
// //                            |
// //                            v
//             motor_set_dir_backward();
//         }
// //  CASE: NEGATIVE -------------------------------------------------------------
//     } else {
// //                                     |
// //                                     v
//         if ((state.calculated_position < state.target_position) ||
//             (steps_to_go <= controller_get_decel_threshold())) {
// //                         |
// //                         v
//             state.velocity += state.accel;
// //                                           |
// //                                           v
//         } else if (state.calculated_position > state.target_position) {
// //                             |                  |
// //                             v                  v
//             state.velocity = max32(state.velocity - state.accel,
// //              |
// //              v
//                 -state.max_speed);
//         }
//         state.calculated_position += state.velocity;
// //                               |
// //                               v
//         if (state.motor_position > state.calculated_position &&
//             state.motor_position != state.target_position) {
// //                               |
// //                               v
//             state.motor_position -= FIXED_ONE;
//
//             servo.move(state.target_position);
//         }
// //                         |
// //                         v
//         if (state.velocity > 0) {
// //                            |
// //                            v
//             state.direction = 1;
// //                            |
// //                            v
//             motor_set_dir_forward();
//         }
//
//
//     }
// //  END ------------------------------------------------------------------------
}

#endif
