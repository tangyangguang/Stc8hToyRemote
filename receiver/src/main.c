#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static proto_rf_link_t link;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[PROTO_RF_LINK_PAYLOAD_MAX];
static toy_remote_status_t status;

static void enter_safe_state(void)
{
    status.link_state = PROTO_RF_LINK_STATE_LOST;
}

void main(void)
{
    stc8h_spi_init();
    drv_nrf24l01_init_pins();

    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 2u, 1u);

    status.link_state = PROTO_RF_LINK_STATE_IDLE;
    status.voltage_int = 0u;
    status.voltage_dec = 0u;

    (void)toy_remote_pack_status(payload, &status);
    (void)proto_rf_link_send_status(&link, packet, payload, TOY_REMOTE_STATUS_PAYLOAD_SIZE);
    enter_safe_state();

    while (1) {
    }
}
