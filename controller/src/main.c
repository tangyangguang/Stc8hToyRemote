#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static proto_rf_link_t link;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[PROTO_RF_LINK_PAYLOAD_MAX];
static toy_remote_control_t control;

void main(void)
{
    stc8h_spi_init();
    drv_nrf24l01_init_pins();

    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 1u, 2u);

    control.direction = 0u;
    control.speed = 0u;
    control.brake = 1u;
    control.steering_angle = 90u;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;

    (void)toy_remote_pack_control(payload, &control);
    (void)proto_rf_link_send_data(&link, packet, payload, TOY_REMOTE_CONTROL_PAYLOAD_SIZE);

    while (1) {
    }
}
