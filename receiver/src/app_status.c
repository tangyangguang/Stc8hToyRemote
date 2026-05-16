#include "app_status.h"
#include "board_pins.h"
#include "stc8h_adc.h"

#define APP_STATUS_VOLTAGE_SAMPLE_DIVIDER 6u

static stc8h_u8 sample_divider;

static stc8h_u16 app_status_read_rx_battery_centivolts(void)
{
    stc8h_u8 i;
    stc8h_u16 adc;
    stc8h_u16 sum;
    stc8h_u32 centivolts;

    sum = 0u;
    for (i = 0u; i < 8u; ++i) {
        adc = stc8h_adc_read(TOY_REMOTE_RX_ADC_VOLTAGE_CHANNEL);
        if (adc == STC8H_ADC_INVALID_VALUE) {
            return 0u;
        }
        sum = (stc8h_u16)(sum + adc);
    }

    adc = (stc8h_u16)(sum >> 3);
    centivolts = (stc8h_u32)adc * 13286u;
    centivolts = (centivolts + 512u) / 1024u;
    return (stc8h_u16)centivolts;
}

void app_status_init(toy_remote_status_t *status)
{
    stc8h_adc_init();
    sample_divider = 0u;
    if (status == 0) {
        return;
    }

    status->link_state = TOY_REMOTE_LINK_STATE_LOST;
    status->voltage_int = 0u;
    status->voltage_dec = 0u;
}

void app_status_update(toy_remote_status_t *status, const toy_remote_control_t *control, stc8h_u8 link_lost)
{
    stc8h_u16 centivolts;

    if (status == 0) {
        return;
    }

    status->link_state = (link_lost == 0u) ? TOY_REMOTE_LINK_STATE_CONNECTED : TOY_REMOTE_LINK_STATE_LOST;

    if ((control == 0) || (control->request_voltage == 0u)) {
        return;
    }

    ++sample_divider;
    if (sample_divider < APP_STATUS_VOLTAGE_SAMPLE_DIVIDER) {
        return;
    }
    sample_divider = 0u;

    centivolts = app_status_read_rx_battery_centivolts();
    if (centivolts > 9999u) {
        centivolts = 9999u;
    }
    status->voltage_int = (stc8h_u8)(centivolts / 100u);
    status->voltage_dec = (stc8h_u8)(centivolts % 100u);
}
