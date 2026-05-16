#ifndef TOY_REMOTE_PROTOCOL_H
#define TOY_REMOTE_PROTOCOL_H

#include "stc8h_config.h"

#define TOY_REMOTE_PROTOCOL_VERSION 1u

#define TOY_REMOTE_CONTROL_PAYLOAD_SIZE 9u
#define TOY_REMOTE_STATUS_PAYLOAD_SIZE 4u

#define TOY_REMOTE_DIRECTION_FORWARD 0u
#define TOY_REMOTE_DIRECTION_REVERSE 1u

#define TOY_REMOTE_CONTROL_SPEED_MAX 100u
#define TOY_REMOTE_CONTROL_AUX_PWM_MAX 100u
#define TOY_REMOTE_STEERING_MIN 0u
#define TOY_REMOTE_STEERING_CENTER 90u
#define TOY_REMOTE_STEERING_MAX 180u

#define TOY_REMOTE_LINK_STATE_IDLE 0u
#define TOY_REMOTE_LINK_STATE_CONNECTING 1u
#define TOY_REMOTE_LINK_STATE_CONNECTED 2u
#define TOY_REMOTE_LINK_STATE_LOST 3u

#define TOY_REMOTE_VOLTAGE_DEC_MAX 99u

typedef struct {
    stc8h_u8 direction;
    stc8h_u8 speed;
    stc8h_u8 brake;
    stc8h_u8 steering_angle;
    stc8h_u8 light;
    stc8h_u8 buzzer;
    stc8h_u8 aux_pwm;
    stc8h_u8 request_voltage;
} toy_remote_control_t;

typedef struct {
    stc8h_u8 link_state;
    stc8h_u8 voltage_int;
    stc8h_u8 voltage_dec;
} toy_remote_status_t;

void toy_remote_control_set_safe(toy_remote_control_t *control);
stc8h_status_t toy_remote_validate_control(const toy_remote_control_t *control);
stc8h_status_t toy_remote_validate_status(const toy_remote_status_t *status);

stc8h_status_t toy_remote_pack_control(stc8h_u8 *payload, const toy_remote_control_t *control);
stc8h_status_t toy_remote_unpack_control(toy_remote_control_t *control, const stc8h_u8 *payload, stc8h_u8 len);
stc8h_status_t toy_remote_pack_status(stc8h_u8 *payload, const toy_remote_status_t *status);
stc8h_status_t toy_remote_unpack_status(toy_remote_status_t *status, const stc8h_u8 *payload, stc8h_u8 len);

#endif
