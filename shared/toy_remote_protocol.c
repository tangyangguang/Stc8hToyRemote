#include "toy_remote_protocol.h"

stc8h_status_t toy_remote_pack_control(stc8h_u8 *payload, const toy_remote_control_t *control)
{
    if ((payload == 0) || (control == 0)) {
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
    return STC8H_OK;
}

stc8h_status_t toy_remote_pack_status(stc8h_u8 *payload, const toy_remote_status_t *status)
{
    if ((payload == 0) || (status == 0)) {
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
    return STC8H_OK;
}
