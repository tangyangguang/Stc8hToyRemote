#ifndef APP_OUTPUTS_CALC_H
#define APP_OUTPUTS_CALC_H

#include "stc8h_config.h"

#define APP_OUTPUT_SERVO_PRESCALER 11u
#define APP_OUTPUT_SERVO_PERIOD 18431u
#define APP_OUTPUT_SERVO_MIN_DUTY 461u
#define APP_OUTPUT_SERVO_MAX_DUTY 2303u
#define APP_OUTPUT_SERVO_CENTER_DUTY 1382u

#define APP_OUTPUT_FAST_PWM_PRESCALER 11u
#define APP_OUTPUT_FAST_PWM_PERIOD 100u
#define APP_OUTPUT_MOTOR_MIN_SPEED 5u
#define APP_OUTPUT_MOTOR_MIN_DUTY 40u

#define APP_OUTPUT_FAST_DUTY(percent) \
    (((percent) > APP_OUTPUT_FAST_PWM_PERIOD) ? APP_OUTPUT_FAST_PWM_PERIOD : (percent))

#define APP_OUTPUT_MOTOR_SUPPLY_DUTY(speed, brake) \
    ((((brake) != 0u) || ((speed) >= APP_OUTPUT_MOTOR_MIN_SPEED)) ? APP_OUTPUT_FAST_PWM_PERIOD : 0u)

#define APP_OUTPUT_MOTOR_DUTY(speed) \
    (((speed) < APP_OUTPUT_MOTOR_MIN_SPEED) ? 0u : \
     (APP_OUTPUT_MOTOR_MIN_DUTY + (((stc8h_u16)(speed) * 3u) / 5u)))

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
