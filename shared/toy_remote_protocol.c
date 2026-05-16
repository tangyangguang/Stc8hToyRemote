#include "toy_remote_protocol.h"

#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL || TOY_REMOTE_ENABLE_VALIDATE_STATUS
static stc8h_status_t toy_remote_validate_bool(stc8h_u8 value)
{
    return (value <= 1u) ? STC8H_OK : STC8H_ERROR;
}
#endif

#if TOY_REMOTE_ENABLE_CONTROL_SET_SAFE
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
    control->tx_id = 0u;
}
#endif

#if TOY_REMOTE_ENABLE_CONTROL_APPLY_BRAKE
stc8h_status_t toy_remote_control_apply_brake(toy_remote_control_t *control, stc8h_u8 brake_action)
{
#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL
    if (toy_remote_validate_control(control) != STC8H_OK) {
        return STC8H_ERROR;
    }
#else
    if (control == 0) {
        return STC8H_ERROR;
    }
#endif

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

#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL
    return toy_remote_validate_control(control);
#else
    return STC8H_OK;
#endif
}
#endif

#if TOY_REMOTE_ENABLE_CONTROL_SET_STEERING_FROM_ADC
stc8h_status_t toy_remote_control_set_steering_from_adc(toy_remote_control_t *control, stc8h_u16 adc_value, stc8h_u8 reverse)
{
    stc8h_u16 scaled;

    if (control == 0) {
        return STC8H_ERROR;
    }
    if (reverse > 1u) {
        return STC8H_ERROR;
    }

    if (adc_value > TOY_REMOTE_STEERING_ADC_MAX) {
        adc_value = TOY_REMOTE_STEERING_ADC_MAX;
    }

    scaled = (stc8h_u16)(((adc_value * 45u) + 128u) >> 8);
    if (scaled > TOY_REMOTE_STEERING_MAX) {
        scaled = TOY_REMOTE_STEERING_MAX;
    }

    if (reverse != 0u) {
        scaled = TOY_REMOTE_STEERING_MAX - scaled;
    }

    control->steering_angle = (stc8h_u8)scaled;
#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL
    return toy_remote_validate_control(control);
#else
    return STC8H_OK;
#endif
}
#endif

#if TOY_REMOTE_ENABLE_CONTROL_ADJUST_SPEED
stc8h_status_t toy_remote_control_adjust_speed(toy_remote_control_t *control, stc8h_s16 delta)
{
    stc8h_s16 speed;

#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL
    if (toy_remote_validate_control(control) != STC8H_OK) {
        return STC8H_ERROR;
    }
#else
    if (control == 0) {
        return STC8H_ERROR;
    }
#endif

    speed = (stc8h_s16)control->speed + delta;
    if (speed < 0) {
        speed = 0;
    } else if (speed > (stc8h_s16)TOY_REMOTE_CONTROL_SPEED_MAX) {
        speed = (stc8h_s16)TOY_REMOTE_CONTROL_SPEED_MAX;
    }

    control->speed = (stc8h_u8)speed;
    return STC8H_OK;
}
#endif

#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL
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
#endif

#if TOY_REMOTE_ENABLE_VALIDATE_STATUS
stc8h_status_t toy_remote_validate_status(const toy_remote_status_t *status)
{
    if (status == 0) {
        return STC8H_ERROR;
    }
    if (status->link_state > TOY_REMOTE_LINK_STATE_LOST) {
        return STC8H_ERROR;
    }
    if (status->voltage_int > TOY_REMOTE_VOLTAGE_INT_MAX) {
        return STC8H_ERROR;
    }
    if (status->voltage_dec > TOY_REMOTE_VOLTAGE_DEC_MAX) {
        return STC8H_ERROR;
    }
    return STC8H_OK;
}
#endif

#if TOY_REMOTE_ENABLE_STATUS_SET_VOLTAGE
stc8h_status_t toy_remote_status_set_voltage_centivolts(toy_remote_status_t *status, stc8h_u16 centivolts)
{
    if (status == 0) {
        return STC8H_ERROR;
    }

    if (centivolts > 9999u) {
        centivolts = 9999u;
    }

    status->voltage_int = (stc8h_u8)(centivolts / 100u);
    status->voltage_dec = (stc8h_u8)(centivolts % 100u);
    return toy_remote_validate_status(status);
}
#endif

#if TOY_REMOTE_ENABLE_PACK_CONTROL
stc8h_status_t toy_remote_pack_control(stc8h_u8 *payload, const toy_remote_control_t *control)
{
    if ((payload == 0) || (toy_remote_validate_control(control) != STC8H_OK)) {
        return STC8H_ERROR;
    }

    payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] = control->direction;
    payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] = control->speed;
    payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] = control->brake;
    payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] = control->steering_angle;
    payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] = control->light;
    payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] = control->buzzer;
    payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] = control->aux_pwm;
    payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] = control->request_voltage;
    payload[TOY_REMOTE_CONTROL_OFFSET_TX_ID_L] = (stc8h_u8)control->tx_id;
    payload[TOY_REMOTE_CONTROL_OFFSET_TX_ID_H] = (stc8h_u8)(control->tx_id >> 8);
    return STC8H_OK;
}
#endif

#if TOY_REMOTE_ENABLE_UNPACK_CONTROL
stc8h_status_t toy_remote_unpack_control(toy_remote_control_t *control, const stc8h_u8 *payload, stc8h_u8 len)
{
    if ((control == 0) || (payload == 0) || (len < TOY_REMOTE_CONTROL_PAYLOAD_SIZE)) {
        return STC8H_ERROR;
    }
    if (payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) {
        return STC8H_ERROR;
    }

    control->direction = payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION];
    control->speed = payload[TOY_REMOTE_CONTROL_OFFSET_SPEED];
    control->brake = payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE];
    control->steering_angle = payload[TOY_REMOTE_CONTROL_OFFSET_STEERING];
    control->light = payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT];
    control->buzzer = payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER];
    control->aux_pwm = payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM];
    control->request_voltage = payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE];
    control->tx_id = (stc8h_u16)((stc8h_u16)payload[TOY_REMOTE_CONTROL_OFFSET_TX_ID_L] |
                                  ((stc8h_u16)payload[TOY_REMOTE_CONTROL_OFFSET_TX_ID_H] << 8));
    return toy_remote_validate_control(control);
}
#endif

#if TOY_REMOTE_ENABLE_PACK_STATUS
stc8h_status_t toy_remote_pack_status(stc8h_u8 *payload, const toy_remote_status_t *status)
{
    if ((payload == 0) || (toy_remote_validate_status(status) != STC8H_OK)) {
        return STC8H_ERROR;
    }

    payload[TOY_REMOTE_STATUS_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[TOY_REMOTE_STATUS_OFFSET_LINK_STATE] = status->link_state;
    payload[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] = status->voltage_int;
    payload[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] = status->voltage_dec;
    payload[TOY_REMOTE_STATUS_OFFSET_TX_ID_L] = (stc8h_u8)status->tx_id;
    payload[TOY_REMOTE_STATUS_OFFSET_TX_ID_H] = (stc8h_u8)(status->tx_id >> 8);
    return STC8H_OK;
}
#endif

#if TOY_REMOTE_ENABLE_UNPACK_STATUS
stc8h_status_t toy_remote_unpack_status(toy_remote_status_t *status, const stc8h_u8 *payload, stc8h_u8 len)
{
    if ((status == 0) || (payload == 0) || (len < TOY_REMOTE_STATUS_PAYLOAD_SIZE)) {
        return STC8H_ERROR;
    }
    if (payload[TOY_REMOTE_STATUS_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) {
        return STC8H_ERROR;
    }

    status->link_state = payload[TOY_REMOTE_STATUS_OFFSET_LINK_STATE];
    status->voltage_int = payload[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT];
    status->voltage_dec = payload[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC];
    status->tx_id = (stc8h_u16)((stc8h_u16)payload[TOY_REMOTE_STATUS_OFFSET_TX_ID_L] |
                                 ((stc8h_u16)payload[TOY_REMOTE_STATUS_OFFSET_TX_ID_H] << 8));
    return toy_remote_validate_status(status);
}
#endif
