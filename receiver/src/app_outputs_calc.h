#ifndef APP_OUTPUTS_CALC_H
#define APP_OUTPUTS_CALC_H

#include "stc8h_config.h"

#define APP_OUTPUT_SERVO_PRESCALER 11u
#define APP_OUTPUT_SERVO_PERIOD 18431u
#define APP_OUTPUT_SERVO_MIN_DUTY 461u
#define APP_OUTPUT_SERVO_MAX_DUTY 2303u
#define APP_OUTPUT_SERVO_CENTER_DUTY 1382u

#ifndef APP_OUTPUT_FAST_PWM_PRESCALER
#define APP_OUTPUT_FAST_PWM_PRESCALER 5u
#endif

#ifndef APP_OUTPUT_FAST_PWM_PERIOD
#define APP_OUTPUT_FAST_PWM_PERIOD 91u
#endif

#define APP_OUTPUT_FAST_PWM_HZ \
    (STC8H_SYSCLK_HZ / \
     (((stc8h_u32)APP_OUTPUT_FAST_PWM_PRESCALER + 1UL) * \
      ((stc8h_u32)APP_OUTPUT_FAST_PWM_PERIOD + 1UL)))

#define APP_OUTPUT_MOTOR_MIN_SPEED 5u
#define APP_OUTPUT_MOTOR_MAX_SPEED 100u

#ifndef APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT
#define APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT 20u
#endif

#if APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT > 100u
#error "APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT must be 0..100"
#endif

#if APP_OUTPUT_FAST_PWM_PERIOD > 655u
#error "APP_OUTPUT_FAST_PWM_PERIOD must be <= 655 for 16-bit percent duty math"
#endif

#ifndef APP_OUTPUT_MOTOR_MIN_DUTY
#define APP_OUTPUT_MOTOR_MIN_DUTY \
    ((stc8h_u16)((APP_OUTPUT_FAST_PWM_PERIOD * APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT + 50u) / 100u))
#endif

#define APP_OUTPUT_MOTOR_DUTY_RANGE (APP_OUTPUT_FAST_PWM_PERIOD - APP_OUTPUT_MOTOR_MIN_DUTY)
#define APP_OUTPUT_MOTOR_SPEED_RANGE (APP_OUTPUT_MOTOR_MAX_SPEED - APP_OUTPUT_MOTOR_MIN_SPEED)

#define APP_OUTPUT_FAST_DUTY(percent) \
    (((percent) >= 100u) ? APP_OUTPUT_FAST_PWM_PERIOD : \
     ((stc8h_u16)(((stc8h_u16)(percent) * APP_OUTPUT_FAST_PWM_PERIOD + 50u) / 100u)))

#define APP_OUTPUT_MOTOR_DUTY(speed) \
    (((speed) < APP_OUTPUT_MOTOR_MIN_SPEED) ? 0u : \
     (APP_OUTPUT_MOTOR_MIN_DUTY + \
      ((((stc8h_u16)(speed) - APP_OUTPUT_MOTOR_MIN_SPEED) * APP_OUTPUT_MOTOR_DUTY_RANGE) / \
       APP_OUTPUT_MOTOR_SPEED_RANGE)))

#define APP_OUTPUT_SERVO_RAW_DUTY(angle) \
    (APP_OUTPUT_SERVO_MIN_DUTY + ((stc8h_u16)(angle) * 10u) + (((angle) + 2u) >> 2))

#define APP_OUTPUT_SERVO_DUTY(angle) \
    ((APP_OUTPUT_SERVO_RAW_DUTY(angle) > APP_OUTPUT_SERVO_MAX_DUTY) ? \
     APP_OUTPUT_SERVO_MAX_DUTY : APP_OUTPUT_SERVO_RAW_DUTY(angle))

#define APP_OUTPUT_SET_MOTOR_DUTIES(direction, speed, brake, fwd, rev) do { \
    if ((brake) != 0u) { \
        (fwd) = APP_OUTPUT_FAST_PWM_PERIOD; \
        (rev) = APP_OUTPUT_FAST_PWM_PERIOD; \
    } else { \
        stc8h_u16 app_outputs_motor_duty__ = APP_OUTPUT_MOTOR_DUTY(speed); \
        if ((direction) != 0u) { \
            (fwd) = 0u; \
            (rev) = app_outputs_motor_duty__; \
        } else { \
            (fwd) = app_outputs_motor_duty__; \
            (rev) = 0u; \
        } \
    } \
} while (0)

#endif
