#include "toy_remote_protocol.h"

#include <assert.h>

static void test_control_pack_rejects_invalid_range(void)
{
    stc8h_u8 payload[TOY_REMOTE_CONTROL_PAYLOAD_SIZE];
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = (stc8h_u8)(TOY_REMOTE_CONTROL_SPEED_MAX + 1u);

    assert(toy_remote_validate_control(&control) == STC8H_ERROR);
    assert(toy_remote_pack_control(payload, &control) == STC8H_ERROR);
}

static void test_control_safe_defaults_are_neutral(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);

    assert(control.direction == TOY_REMOTE_DIRECTION_FORWARD);
    assert(control.speed == 0u);
    assert(control.brake == 0u);
    assert(control.steering_angle == TOY_REMOTE_STEERING_CENTER);
    assert(control.light == 0u);
    assert(control.buzzer == 0u);
    assert(control.aux_pwm == 0u);
    assert(control.request_voltage == 0u);
    assert(toy_remote_validate_control(&control) == STC8H_OK);
}

static void test_status_rejects_invalid_decimal_voltage(void)
{
    stc8h_u8 payload[TOY_REMOTE_STATUS_PAYLOAD_SIZE];
    toy_remote_status_t status;

    status.link_state = TOY_REMOTE_LINK_STATE_CONNECTED;
    status.voltage_int = 8u;
    status.voltage_dec = 100u;

    assert(toy_remote_validate_status(&status) == STC8H_ERROR);
    assert(toy_remote_pack_status(payload, &status) == STC8H_ERROR);
}

static void test_control_apply_brake_hold_preserves_speed(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = 67u;

    assert(toy_remote_control_apply_brake(&control, TOY_REMOTE_BRAKE_HOLD_SPEED) == STC8H_OK);
    assert(control.brake == 1u);
    assert(control.speed == 67u);
    assert(toy_remote_validate_control(&control) == STC8H_OK);
}

static void test_control_apply_brake_clear_zeros_speed(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = 67u;

    assert(toy_remote_control_apply_brake(&control, TOY_REMOTE_BRAKE_CLEAR_SPEED) == STC8H_OK);
    assert(control.brake == 1u);
    assert(control.speed == 0u);
    assert(toy_remote_validate_control(&control) == STC8H_OK);
}

static void test_control_release_brake_keeps_speed(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = 42u;
    control.brake = 1u;

    assert(toy_remote_control_apply_brake(&control, TOY_REMOTE_BRAKE_RELEASE) == STC8H_OK);
    assert(control.brake == 0u);
    assert(control.speed == 42u);
    assert(toy_remote_validate_control(&control) == STC8H_OK);
}

static void test_control_set_steering_from_adc_maps_full_range(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);

    assert(toy_remote_control_set_steering_from_adc(&control, 0u, 0u) == STC8H_OK);
    assert(control.steering_angle == TOY_REMOTE_STEERING_MIN);

    assert(toy_remote_control_set_steering_from_adc(&control, 1023u, 0u) == STC8H_OK);
    assert(control.steering_angle == TOY_REMOTE_STEERING_MAX);

    assert(toy_remote_control_set_steering_from_adc(&control, 512u, 0u) == STC8H_OK);
    assert(control.steering_angle == TOY_REMOTE_STEERING_CENTER);
}

static void test_control_set_steering_from_adc_can_reverse(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);

    assert(toy_remote_control_set_steering_from_adc(&control, 0u, 1u) == STC8H_OK);
    assert(control.steering_angle == TOY_REMOTE_STEERING_MAX);

    assert(toy_remote_control_set_steering_from_adc(&control, 1023u, 1u) == STC8H_OK);
    assert(control.steering_angle == TOY_REMOTE_STEERING_MIN);
}

static void test_control_set_steering_from_adc_clamps_high_adc(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);

    assert(toy_remote_control_set_steering_from_adc(&control, 2000u, 0u) == STC8H_OK);
    assert(control.steering_angle == TOY_REMOTE_STEERING_MAX);
}

static void test_status_set_voltage_centivolts_splits_int_and_decimal(void)
{
    toy_remote_status_t status;

    status.link_state = TOY_REMOTE_LINK_STATE_CONNECTED;
    status.voltage_int = 0u;
    status.voltage_dec = 0u;

    assert(toy_remote_status_set_voltage_centivolts(&status, 742u) == STC8H_OK);
    assert(status.voltage_int == 7u);
    assert(status.voltage_dec == 42u);
    assert(toy_remote_validate_status(&status) == STC8H_OK);
}

static void test_status_set_voltage_centivolts_caps_display_range(void)
{
    toy_remote_status_t status;

    status.link_state = TOY_REMOTE_LINK_STATE_CONNECTED;
    status.voltage_int = 0u;
    status.voltage_dec = 0u;

    assert(toy_remote_status_set_voltage_centivolts(&status, 30000u) == STC8H_OK);
    assert(status.voltage_int == TOY_REMOTE_VOLTAGE_INT_MAX);
    assert(status.voltage_dec == TOY_REMOTE_VOLTAGE_DEC_MAX);
    assert(toy_remote_validate_status(&status) == STC8H_OK);
}

static void test_control_adjust_speed_increases_and_caps(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = 95u;

    assert(toy_remote_control_adjust_speed(&control, 10) == STC8H_OK);
    assert(control.speed == TOY_REMOTE_CONTROL_SPEED_MAX);
    assert(toy_remote_validate_control(&control) == STC8H_OK);
}

static void test_control_adjust_speed_decreases_and_floors(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = 5u;

    assert(toy_remote_control_adjust_speed(&control, -10) == STC8H_OK);
    assert(control.speed == 0u);
    assert(toy_remote_validate_control(&control) == STC8H_OK);
}

static void test_control_adjust_speed_rejects_invalid_starting_control(void)
{
    toy_remote_control_t control;

    toy_remote_control_set_safe(&control);
    control.speed = (stc8h_u8)(TOY_REMOTE_CONTROL_SPEED_MAX + 1u);

    assert(toy_remote_control_adjust_speed(&control, 1) == STC8H_ERROR);
}

int main(void)
{
    test_control_pack_rejects_invalid_range();
    test_control_safe_defaults_are_neutral();
    test_status_rejects_invalid_decimal_voltage();
    test_control_apply_brake_hold_preserves_speed();
    test_control_apply_brake_clear_zeros_speed();
    test_control_release_brake_keeps_speed();
    test_control_set_steering_from_adc_maps_full_range();
    test_control_set_steering_from_adc_can_reverse();
    test_control_set_steering_from_adc_clamps_high_adc();
    test_status_set_voltage_centivolts_splits_int_and_decimal();
    test_status_set_voltage_centivolts_caps_display_range();
    test_control_adjust_speed_increases_and_caps();
    test_control_adjust_speed_decreases_and_floors();
    test_control_adjust_speed_rejects_invalid_starting_control();
    return 0;
}
