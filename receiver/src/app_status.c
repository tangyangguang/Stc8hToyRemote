#include "app_status.h"
#include "board_pins.h"
#include "stc8h_adc.h"

#define APP_STATUS_VOLTAGE_REQUEST_SAMPLE_DIVIDER 6u
#define APP_STATUS_VOLTAGE_IDLE_SAMPLE_DIVIDER 50u
#define APP_STATUS_VOLTAGE_FULL_SCALE_CENTIVOLTS 1329u
#define APP_STATUS_ADC_FULL_SCALE_COUNTS 1024u

static stc8h_u8 sample_divider;

static stc8h_u16 app_status_scale_adc_centivolts(stc8h_u16 adc)
{
    stc8h_u16 fractional;

    /* Exact form of floor(adc * 1329 / 1024) without 32-bit math. */
    fractional = (stc8h_u16)((adc & 0x03u) << 8);
    fractional = (stc8h_u16)(fractional + (adc * 49u));
    return (stc8h_u16)(adc + (adc >> 2) + (fractional >> 10));
}

static stc8h_u16 app_status_read_rx_battery_centivolts(void)
{
    stc8h_u8 i;
    stc8h_u16 adc;
    stc8h_u16 sum;

    sum = 0u;
    for (i = 0u; i < 8u; ++i) {
        adc = stc8h_adc_read(TOY_REMOTE_RX_ADC_VOLTAGE_CHANNEL);
        if (adc == STC8H_ADC_INVALID_VALUE) {
            return 0u;
        }
        sum = (stc8h_u16)(sum + adc);
    }

    adc = (stc8h_u16)(sum >> 3);
    return app_status_scale_adc_centivolts(adc);
}

void app_status_init(STC8H_XDATA toy_remote_status_t *status)
{
    P1M0 &= (stc8h_u8)~TOY_REMOTE_RX_ADC_VOLTAGE_MASK;
    P1M1 |= TOY_REMOTE_RX_ADC_VOLTAGE_MASK;
    stc8h_adc_init();
    sample_divider = 0u;

    status->link_state = TOY_REMOTE_LINK_STATE_LOST;
    status->voltage_int = 0u;
    status->voltage_dec = 0u;
}

void app_status_update(STC8H_XDATA toy_remote_status_t *status,
                       const STC8H_XDATA toy_remote_control_t *control,
                       stc8h_u8 link_lost)
{
    stc8h_u16 centivolts;
    stc8h_u8 sample_limit;

    status->link_state = (link_lost == 0u) ? TOY_REMOTE_LINK_STATE_CONNECTED : TOY_REMOTE_LINK_STATE_LOST;

    sample_limit = (control->request_voltage != 0u) ?
        APP_STATUS_VOLTAGE_REQUEST_SAMPLE_DIVIDER :
        APP_STATUS_VOLTAGE_IDLE_SAMPLE_DIVIDER;

    ++sample_divider;
    if (sample_divider < sample_limit) {
        return;
    }
    sample_divider = 0u;

    centivolts = app_status_read_rx_battery_centivolts();
    if (centivolts > 9999u) {
        centivolts = 9999u;
    }
    {
        stc8h_u8 v_int;

        v_int = (stc8h_u8)(centivolts / 100u);
        status->voltage_int = v_int;
        status->voltage_dec = (stc8h_u8)(centivolts - (stc8h_u16)v_int * 100u);
    }
}
