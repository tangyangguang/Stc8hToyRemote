#include "toy_remote_protocol.h"

static stc8h_status_t toy_remote_validate_bool(stc8h_u8 value)
{
    return (value <= 1u) ? STC8H_OK : STC8H_ERROR;
}

void toy_remote_control_set_safe(toy_remote_control_t *control)
{
    if (control == 0) {
        return;
    }

    control->direction = TOY_REMOTE_DIRECTION_FORWARD;
    control->speed = 0u;
    control->brake = 0u;
    control->steering_angle = TOY_REMOTE_STEERING_CENTER;
    control->light = 0u;
    control->buzzer = 0u;
    control->aux_pwm = 0u;
    control->request_voltage = 0u;
}

stc8h_status_t toy_remote_control_apply_brake(toy_remote_control_t *control, stc8h_u8 brake_action)
{
    if (control == 0) {
        return STC8H_ERROR;
    }

    if (brake_action == TOY_REMOTE_BRAKE_RELEASE) {
        control->brake = 0u;
    } else if (brake_action == TOY_REMOTE_BRAKE_HOLD_SPEED) {
        control->brake = 1u;
    } else if (brake_action == TOY_REMOTE_BRAKE_CLEAR_SPEED) {
        control->brake = 1u;
        control->speed = 0u;
    } else {
        return STC8H_ERROR;
    }

    return toy_remote_validate_control(control);
}

stc8h_status_t toy_remote_control_set_steering_from_adc(toy_remote_control_t *control, stc8h_u16 adc_value, stc8h_u8 reverse)
{
    stc8h_u32 scaled;

    if (control == 0) {
        return STC8H_ERROR;
    }
    if (reverse > 1u) {
        return STC8H_ERROR;
    }

    if (adc_value > TOY_REMOTE_STEERING_ADC_MAX) {
        adc_value = TOY_REMOTE_STEERING_ADC_MAX;
    }

    scaled = (stc8h_u32)adc_value * TOY_REMOTE_STEERING_MAX;
    scaled = (scaled + (TOY_REMOTE_STEERING_ADC_MAX / 2u)) / TOY_REMOTE_STEERING_ADC_MAX;

    if (reverse != 0u) {
        scaled = TOY_REMOTE_STEERING_MAX - scaled;
    }

    control->steering_angle = (stc8h_u8)scaled;
    return toy_remote_validate_control(control);
}

stc8h_status_t toy_remote_validate_control(const toy_remote_control_t *control)
{
    if (control == 0) {
        return STC8H_ERROR;
    }
    if (control->direction > TOY_REMOTE_DIRECTION_REVERSE) {
        return STC8H_ERROR;
    }
    if (control->speed > TOY_REMOTE_CONTROL_SPEED_MAX) {
        return STC8H_ERROR;
    }
    if (toy_remote_validate_bool(control->brake) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (control->steering_angle > TOY_REMOTE_STEERING_MAX) {
        return STC8H_ERROR;
    }
    if (toy_remote_validate_bool(control->light) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (toy_remote_validate_bool(control->buzzer) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (control->aux_pwm > TOY_REMOTE_CONTROL_AUX_PWM_MAX) {
        return STC8H_ERROR;
    }
    if (toy_remote_validate_bool(control->request_voltage) != STC8H_OK) {
        return STC8H_ERROR;
    }
    return STC8H_OK;
}

stc8h_status_t toy_remote_validate_status(const toy_remote_status_t *status)
{
    if (status == 0) {
        return STC8H_ERROR;
    }
    if (status->link_state > TOY_REMOTE_LINK_STATE_LOST) {
        return STC8H_ERROR;
    }
    if (status->voltage_dec > TOY_REMOTE_VOLTAGE_DEC_MAX) {
        return STC8H_ERROR;
    }
    return STC8H_OK;
}

stc8h_status_t toy_remote_pack_control(stc8h_u8 *payload, const toy_remote_control_t *control)
{
    if ((payload == 0) || (toy_remote_validate_control(control) != STC8H_OK)) {
        return STC8H_ERROR;
    }

    payload[0] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[1] = control->direction;
    payload[2] = control->speed;
    payload[3] = control->brake;
    payload[4] = control->steering_angle;
    payload[5] = control->light;
    payload[6] = control->buzzer;
    payload[7] = control->aux_pwm;
    payload[8] = control->request_voltage;
    return STC8H_OK;
}

stc8h_status_t toy_remote_unpack_control(toy_remote_control_t *control, const stc8h_u8 *payload, stc8h_u8 len)
{
    if ((control == 0) || (payload == 0) || (len < TOY_REMOTE_CONTROL_PAYLOAD_SIZE)) {
        return STC8H_ERROR;
    }
    if (payload[0] != TOY_REMOTE_PROTOCOL_VERSION) {
        return STC8H_ERROR;
    }

    control->direction = payload[1];
    control->speed = payload[2];
    control->brake = payload[3];
    control->steering_angle = payload[4];
    control->light = payload[5];
    control->buzzer = payload[6];
    control->aux_pwm = payload[7];
    control->request_voltage = payload[8];
    return toy_remote_validate_control(control);
}

stc8h_status_t toy_remote_pack_status(stc8h_u8 *payload, const toy_remote_status_t *status)
{
    if ((payload == 0) || (toy_remote_validate_status(status) != STC8H_OK)) {
        return STC8H_ERROR;
    }

    payload[0] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[1] = status->link_state;
    payload[2] = status->voltage_int;
    payload[3] = status->voltage_dec;
    return STC8H_OK;
}

stc8h_status_t toy_remote_unpack_status(toy_remote_status_t *status, const stc8h_u8 *payload, stc8h_u8 len)
{
    if ((status == 0) || (payload == 0) || (len < TOY_REMOTE_STATUS_PAYLOAD_SIZE)) {
        return STC8H_ERROR;
    }
    if (payload[0] != TOY_REMOTE_PROTOCOL_VERSION) {
        return STC8H_ERROR;
    }

    status->link_state = payload[1];
    status->voltage_int = payload[2];
    status->voltage_dec = payload[3];
    return toy_remote_validate_status(status);
}
