#include "app_radio.h"
#include "drv_nrf24l01.h"

#include <assert.h>

static stc8h_status_t check_present_result;
static stc8h_status_t ack_payload_result;
static stc8h_u8 set_channel_calls;
static stc8h_u8 enable_ack_payload_calls;
static stc8h_u8 enter_tx_calls;
static stc8h_u8 flush_rx_calls;
static stc8h_u8 clear_irq_calls;
static stc8h_u8 read_payload_calls;
static stc8h_u8 read_status_value;
static stc8h_u8 fifo_status_value;
static stc8h_u8 payload_width;

static void reset_stubs(void)
{
    check_present_result = STC8H_OK;
    ack_payload_result = STC8H_OK;
    set_channel_calls = 0u;
    enable_ack_payload_calls = 0u;
    enter_tx_calls = 0u;
    flush_rx_calls = 0u;
    clear_irq_calls = 0u;
    read_payload_calls = 0u;
    read_status_value = 0u;
    fifo_status_value = DRV_NRF24L01_FIFO_RX_EMPTY;
    payload_width = 0u;
}

void drv_nrf24l01_init_pins(void) { }
void drv_nrf24l01_power_down(void) { }
void drv_nrf24l01_flush_tx(void) { }
void drv_nrf24l01_flush_rx(void) { ++flush_rx_calls; }
void drv_nrf24l01_clear_irq(stc8h_u8 flags) { (void)flags; ++clear_irq_calls; }
stc8h_status_t drv_nrf24l01_check_present(void) { return check_present_result; }
stc8h_status_t drv_nrf24l01_set_channel(stc8h_u8 channel)
{
    (void)channel;
    ++set_channel_calls;
    return STC8H_OK;
}
stc8h_status_t drv_nrf24l01_config_pipe0_fixed_code(DRV_NRF24L01_CODE_CONST stc8h_u8 *addr)
{
    (void)addr;
    return STC8H_OK;
}
stc8h_status_t drv_nrf24l01_enable_ack_payload(stc8h_u8 pipe_mask)
{
    (void)pipe_mask;
    ++enable_ack_payload_calls;
    return ack_payload_result;
}
void drv_nrf24l01_set_auto_ack(stc8h_u8 pipe_mask) { (void)pipe_mask; }
stc8h_status_t drv_nrf24l01_set_auto_retransmit(stc8h_u8 delay_code, stc8h_u8 count)
{
    (void)delay_code;
    (void)count;
    return STC8H_OK;
}
stc8h_status_t drv_nrf24l01_set_rate_power(drv_nrf24l01_rate_t rate, drv_nrf24l01_power_t power)
{
    (void)rate;
    (void)power;
    return STC8H_OK;
}
void drv_nrf24l01_enter_tx(void) { ++enter_tx_calls; }
stc8h_u8 drv_nrf24l01_write_payload(const stc8h_u8 *data, stc8h_u8 len)
{
    (void)data;
    (void)len;
    return 0u;
}
stc8h_u8 drv_nrf24l01_write_payload_fixed_xdata(const STC8H_XDATA stc8h_u8 *data)
{
    (void)data;
    return 0u;
}
void drv_nrf24l01_pulse_ce(void) { }
stc8h_u8 drv_nrf24l01_read_status(void) { return read_status_value; }
stc8h_u8 drv_nrf24l01_read_fifo_status(void) { return fifo_status_value; }
stc8h_u8 drv_nrf24l01_read_dynamic_payload_size(void) { return payload_width; }
stc8h_u8 drv_nrf24l01_read_payload(stc8h_u8 *data, stc8h_u8 len)
{
    stc8h_u8 i;

    ++read_payload_calls;
    for (i = 0u; i < len; ++i) {
        data[i] = (stc8h_u8)(i + 1u);
    }
    return 0u;
}

static void test_init_fails_when_radio_absent(void)
{
    reset_stubs();
    check_present_result = STC8H_ERROR;

    assert(app_radio_init_tx(76u) == STC8H_ERROR);
    assert(set_channel_calls == 0u);
    assert(enable_ack_payload_calls == 0u);
    assert(enter_tx_calls == 0u);
}

static void test_init_fails_when_ack_payload_enable_fails(void)
{
    reset_stubs();
    ack_payload_result = STC8H_ERROR;

    assert(app_radio_init_tx(76u) == STC8H_ERROR);
    assert(set_channel_calls == 1u);
    assert(enable_ack_payload_calls == 1u);
    assert(enter_tx_calls == 0u);
}

static void test_init_enters_tx_after_required_checks(void)
{
    reset_stubs();

    assert(app_radio_init_tx(76u) == STC8H_OK);
    assert(set_channel_calls == 1u);
    assert(enable_ack_payload_calls == 1u);
    assert(enter_tx_calls == 1u);
}

static void test_set_channel_stays_in_existing_tx_mode(void)
{
    reset_stubs();

    app_radio_set_channel(23u);

    assert(set_channel_calls == 1u);
    assert(flush_rx_calls == 1u);
    assert(clear_irq_calls == 1u);
    assert(enter_tx_calls == 0u);
}

static void test_send_reads_ack_payload_when_rx_ready_lags_fifo(void)
{
    stc8h_u8 packet[APP_RADIO_PACKET_SIZE] = {0u};

    reset_stubs();
    read_status_value = DRV_NRF24L01_STATUS_TX_DONE;
    fifo_status_value = DRV_NRF24L01_FIFO_TX_EMPTY;
    payload_width = APP_RADIO_STATUS_ACK_SIZE;

    assert(app_radio_send_packet_with_ack(packet) == APP_RADIO_TX_ACK_PAYLOAD_OK);
    assert(app_radio_ack_len == APP_RADIO_STATUS_ACK_SIZE);
    assert(app_radio_ack_packet[0] == 1u);
    assert(read_payload_calls == 1u);
}

int main(void)
{
    test_init_fails_when_radio_absent();
    test_init_fails_when_ack_payload_enable_fails();
    test_init_enters_tx_after_required_checks();
    test_set_channel_stays_in_existing_tx_mode();
    test_send_reads_ack_payload_when_rx_ready_lags_fifo();
    return 0;
}
