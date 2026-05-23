#include "app_outputs_calc.h"

#include <assert.h>

#ifndef APP_OUTPUT_FAST_PWM_HZ
#define APP_OUTPUT_FAST_PWM_HZ 0UL
#endif

#ifndef APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT
#define APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT 0u
#endif

#ifndef APP_OUTPUTS_CALC_EXPECTED_MIN_DUTY
#define APP_OUTPUTS_CALC_EXPECTED_MIN_DUTY_PERCENT 20u
#define APP_OUTPUTS_CALC_EXPECTED_MIN_DUTY 18u
#define APP_OUTPUTS_CALC_EXPECTED_DUTY_25 33u
#define APP_OUTPUTS_CALC_EXPECTED_DUTY_39 44u
#define APP_OUTPUTS_CALC_EXPECTED_DUTY_40 44u
#endif

static void test_fast_pwm_defaults_to_near_20khz(void)
{
    assert(APP_OUTPUT_FAST_PWM_PRESCALER == 5u);
    assert(APP_OUTPUT_FAST_PWM_PERIOD == 91u);
    assert(APP_OUTPUT_FAST_PWM_HZ >= 19900UL);
    assert(APP_OUTPUT_FAST_PWM_HZ <= 20100UL);
}

static void test_motor_min_duty_is_configurable(void)
{
    assert(APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT == APP_OUTPUTS_CALC_EXPECTED_MIN_DUTY_PERCENT);
    assert(APP_OUTPUT_MOTOR_MIN_DUTY == APP_OUTPUTS_CALC_EXPECTED_MIN_DUTY);
}

static void test_motor_speed_maps_from_soft_start_to_full_duty(void)
{
    assert(APP_OUTPUT_MOTOR_DUTY(0u) == 0u);
    assert(APP_OUTPUT_MOTOR_DUTY(4u) == 0u);
    assert(APP_OUTPUT_MOTOR_DUTY(5u) == APP_OUTPUTS_CALC_EXPECTED_MIN_DUTY);
    assert(APP_OUTPUT_MOTOR_DUTY(25u) == APP_OUTPUTS_CALC_EXPECTED_DUTY_25);
    assert(APP_OUTPUT_MOTOR_DUTY(39u) == APP_OUTPUTS_CALC_EXPECTED_DUTY_39);
    assert(APP_OUTPUT_MOTOR_DUTY(40u) == APP_OUTPUTS_CALC_EXPECTED_DUTY_40);
    assert(APP_OUTPUT_MOTOR_DUTY(100u) == APP_OUTPUT_FAST_PWM_PERIOD);
}

static void test_percent_output_is_period_based(void)
{
    assert(APP_OUTPUT_FAST_DUTY(0u) == 0u);
    assert(APP_OUTPUT_FAST_DUTY(50u) == 46u);
    assert(APP_OUTPUT_FAST_DUTY(100u) == APP_OUTPUT_FAST_PWM_PERIOD);
}

static void test_servo_angle_keeps_independent_50hz_range(void)
{
    stc8h_u16 min_duty;
    stc8h_u16 center_duty;
    stc8h_u16 max_duty;

    min_duty = APP_OUTPUT_SERVO_DUTY(0u);
    center_duty = APP_OUTPUT_SERVO_DUTY(90u);
    max_duty = APP_OUTPUT_SERVO_DUTY(180u);

    assert(min_duty == APP_OUTPUT_SERVO_MIN_DUTY);
    assert(center_duty > min_duty);
    assert(center_duty < max_duty);
    assert(max_duty == APP_OUTPUT_SERVO_MAX_DUTY);
}

static void test_motor_pair_applies_direction_and_active_brake(void)
{
    stc8h_u16 fwd;
    stc8h_u16 rev;

    APP_OUTPUT_SET_MOTOR_DUTIES(0u, 25u, 0u, fwd, rev);
    assert(fwd == APP_OUTPUTS_CALC_EXPECTED_DUTY_25);
    assert(rev == 0u);

    APP_OUTPUT_SET_MOTOR_DUTIES(1u, 25u, 0u, fwd, rev);
    assert(fwd == 0u);
    assert(rev == APP_OUTPUTS_CALC_EXPECTED_DUTY_25);

    APP_OUTPUT_SET_MOTOR_DUTIES(0u, 0u, 1u, fwd, rev);
    assert(fwd == APP_OUTPUT_FAST_PWM_PERIOD);
    assert(rev == APP_OUTPUT_FAST_PWM_PERIOD);
}

int main(void)
{
    test_fast_pwm_defaults_to_near_20khz();
    test_motor_min_duty_is_configurable();
    test_motor_speed_maps_from_soft_start_to_full_duty();
    test_percent_output_is_period_based();
    test_servo_angle_keeps_independent_50hz_range();
    test_motor_pair_applies_direction_and_active_brake();
    return 0;
}
