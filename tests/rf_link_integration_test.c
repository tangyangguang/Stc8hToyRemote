#include "proto_rf_link.h"
#include "toy_remote_protocol.h"

#include <assert.h>

static void test_control_payload_fits_rf_link(void)
{
    assert(TOY_REMOTE_CONTROL_PAYLOAD_SIZE <= PROTO_RF_LINK_PAYLOAD_MAX);
    assert(TOY_REMOTE_STATUS_PAYLOAD_SIZE <= PROTO_RF_LINK_PAYLOAD_MAX);
}

static void test_control_payload_round_trips_through_rf_link_data_packet(void)
{
    proto_rf_link_t tx_link;
    proto_rf_link_t rx_link;
    toy_remote_control_t control;
    toy_remote_control_t decoded_control;
    stc8h_u8 control_payload[TOY_REMOTE_CONTROL_PAYLOAD_SIZE];
    stc8h_u8 decoded_payload[PROTO_RF_LINK_PAYLOAD_MAX];
    stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
    stc8h_u8 packet_type;
    stc8h_u8 decoded_len;

    proto_rf_link_init(&tx_link);
    proto_rf_link_init(&rx_link);
    proto_rf_link_set_ids(&tx_link, 1u, 2u);
    proto_rf_link_set_ids(&rx_link, 2u, 1u);

    toy_remote_control_set_safe(&control);
    control.direction = TOY_REMOTE_DIRECTION_REVERSE;
    control.speed = 73u;
    control.brake = 1u;
    control.steering_angle = 123u;
    control.light = 1u;
    control.buzzer = 0u;
    control.aux_pwm = 35u;
    control.request_voltage = 1u;
    control.tx_id = 0x4A21u;

    assert(toy_remote_pack_control(control_payload, &control) == STC8H_OK);
    assert(proto_rf_link_send_data(&tx_link, packet, control_payload, TOY_REMOTE_CONTROL_PAYLOAD_SIZE) == STC8H_OK);
    assert(proto_rf_link_poll(&rx_link, packet, &packet_type, decoded_payload, &decoded_len) == PROTO_RF_LINK_EVENT_DATA);
    assert(packet_type == PROTO_RF_LINK_PACKET_DATA);
    assert(decoded_len == TOY_REMOTE_CONTROL_PAYLOAD_SIZE);
    assert(toy_remote_unpack_control(&decoded_control, decoded_payload, decoded_len) == STC8H_OK);

    assert(decoded_control.direction == control.direction);
    assert(decoded_control.speed == control.speed);
    assert(decoded_control.brake == control.brake);
    assert(decoded_control.steering_angle == control.steering_angle);
    assert(decoded_control.light == control.light);
    assert(decoded_control.buzzer == control.buzzer);
    assert(decoded_control.aux_pwm == control.aux_pwm);
    assert(decoded_control.request_voltage == control.request_voltage);
    assert(decoded_control.tx_id == control.tx_id);
}

static void test_status_payload_round_trips_through_rf_link_status_packet(void)
{
    proto_rf_link_t tx_link;
    proto_rf_link_t rx_link;
    toy_remote_status_t status;
    toy_remote_status_t decoded_status;
    stc8h_u8 status_payload[TOY_REMOTE_STATUS_PAYLOAD_SIZE];
    stc8h_u8 decoded_payload[PROTO_RF_LINK_PAYLOAD_MAX];
    stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
    stc8h_u8 packet_type;
    stc8h_u8 decoded_len;

    proto_rf_link_init(&tx_link);
    proto_rf_link_init(&rx_link);
    proto_rf_link_set_ids(&tx_link, 2u, 1u);
    proto_rf_link_set_ids(&rx_link, 1u, 2u);

    status.link_state = TOY_REMOTE_LINK_STATE_CONNECTED;
    status.tx_id = 0x4A21u;
    assert(toy_remote_status_set_voltage_centivolts(&status, 742u) == STC8H_OK);

    assert(toy_remote_pack_status(status_payload, &status) == STC8H_OK);
    assert(proto_rf_link_send_status(&tx_link, packet, status_payload, TOY_REMOTE_STATUS_PAYLOAD_SIZE) == STC8H_OK);
    assert(proto_rf_link_poll(&rx_link, packet, &packet_type, decoded_payload, &decoded_len) == PROTO_RF_LINK_EVENT_STATUS);
    assert(packet_type == PROTO_RF_LINK_PACKET_STATUS);
    assert(decoded_len == TOY_REMOTE_STATUS_PAYLOAD_SIZE);
    assert(toy_remote_unpack_status(&decoded_status, decoded_payload, decoded_len) == STC8H_OK);

    assert(decoded_status.link_state == status.link_state);
    assert(decoded_status.voltage_int == status.voltage_int);
    assert(decoded_status.voltage_dec == status.voltage_dec);
    assert(decoded_status.tx_id == status.tx_id);
}

int main(void)
{
    test_control_payload_fits_rf_link();
    test_control_payload_round_trips_through_rf_link_data_packet();
    test_status_payload_round_trips_through_rf_link_status_packet();
    return 0;
}
