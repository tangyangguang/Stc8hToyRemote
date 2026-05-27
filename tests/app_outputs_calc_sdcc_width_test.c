#include "app_outputs_calc.h"

typedef char app_outputs_motor_min_duty_uses_wide_math[
    (APP_OUTPUT_MOTOR_MIN_DUTY == APP_OUTPUT_FAST_PWM_PERIOD) ? 1 : -1
];

void main(void)
{
}
