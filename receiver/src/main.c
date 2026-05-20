#include "app_config.h"
#include "app_indicator.h"
#include "app_radio.h"
#include "app_outputs.h"
#include "app_status.h"
#include "board_pins.h"
#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_interrupt.h"
#include "stc8h_sfr.h"
#include "stc8h_spi.h"
#include "stc8h_timer.h"
#include "toy_remote_channels.h"
#include "toy_remote_protocol.h"

static volatile stc8h_u16 tick_ms;
static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA app_indicator_t indicator;
static STC8H_XDATA app_config_t config;
static STC8H_XDATA toy_remote_control_t control;
static STC8H_XDATA toy_remote_status_t status;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 status_packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[PROTO_RF_LINK_PAYLOAD_MAX];
static stc8h_u16 idle_polls;
static stc8h_u8 radio_error;
static stc8h_u8 link_lost;
#ifndef APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
#define APP_RECEIVER_ENABLE_CHANNEL_BUTTONS 0
#endif
#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
static stc8h_u8 ch_add_pressed;
static stc8h_u8 ch_minus_pressed;
#endif

#define APP_RECEIVER_IDLE_POLL_LIMIT 60000u

STC8H_INTERRUPT(timer0_isr, STC8H_VECTOR_TIMER0)
{
    TF0 = 0;
    ++tick_ms;
}

static stc8h_u16 app_tick_now(void)
{
    stc8h_u16 now;
    stc8h_u8 interrupt_enabled;

    interrupt_enabled = (EA != 0u) ? 1u : 0u;
    EA = 0;
    now = tick_ms;
    if (interrupt_enabled != 0u) {
        EA = 1;
    }

    return now;
}

static void app_indicator_write(stc8h_u8 led_on)
{
    if (led_on != 0u) {
        TOY_REMOTE_RX_LED_ON();
    } else {
        TOY_REMOTE_RX_LED_OFF();
    }
}

static void app_indicator_service(void)
{
    app_indicator_write(app_indicator_update(&indicator, app_tick_now()));
}

static void app_timer0_init_indicator_tick(void)
{
    if (stc8h_timer_init_1ms(STC8H_TIMER0) == STC8H_OK) {
        ET0 = 1;
        EA = 1;
        TR0 = 1;
    }
}

static app_indicator_state_t app_waiting_indicator_state(void)
{
    return (config.bound_tx_id == 0u) ?
        APP_INDICATOR_STATE_WAITING_UNBOUND :
        APP_INDICATOR_STATE_WAITING_BOUND;
}

static void apply_safe_state(void)
{
    control.direction = TOY_REMOTE_DIRECTION_FORWARD;
    control.speed = 0u;
    control.brake = 0u;
    control.steering_angle = TOY_REMOTE_STEERING_CENTER;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;
    control.tx_id = 0u;
    app_outputs_apply_safe();
    link_lost = 1u;
}

static void prepare_ack_status(void)
{
    stc8h_u8 i;

    app_status_update(&status, &control, link_lost);
    status.tx_id = config.bound_tx_id;

    for (i = 0u; i < APP_RADIO_PACKET_SIZE; ++i) {
        status_packet[i] = 0u;
    }
    status_packet[0] = PROTO_RF_LINK_MAGIC;
    status_packet[1] = PROTO_RF_LINK_VERSION;
    status_packet[2] = PROTO_RF_LINK_PACKET_STATUS;
    status_packet[3] = link.seq_tx;
    status_packet[4] = packet[3];
    status_packet[5] = 0u;
    status_packet[6] = 2u;
    status_packet[7] = 1u;
    status_packet[8] = TOY_REMOTE_STATUS_PAYLOAD_SIZE;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_LINK_STATE] = status.link_state;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] = status.voltage_int;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] = status.voltage_dec;
    TOY_REMOTE_PUT_U16_LE(status_packet, PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_TX_ID_L, config.bound_tx_id);
    ++link.seq_tx;
    drv_nrf24l01_flush_tx();
    for (i = 0u; i < APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT; ++i) {
        (void)drv_nrf24l01_write_ack_payload(0u, status_packet, APP_RADIO_PACKET_SIZE);
    }
}

static stc8h_status_t unpack_control_payload(void)
{
    if ((payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] > TOY_REMOTE_DIRECTION_REVERSE) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] > TOY_REMOTE_CONTROL_SPEED_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] > TOY_REMOTE_STEERING_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] > TOY_REMOTE_CONTROL_AUX_PWM_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] > 1u)) {
        return STC8H_ERROR;
    }

    control.direction = payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION];
    control.speed = payload[TOY_REMOTE_CONTROL_OFFSET_SPEED];
    control.brake = payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE];
    control.steering_angle = payload[TOY_REMOTE_CONTROL_OFFSET_STEERING];
    control.light = payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT];
    control.buzzer = payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER];
    control.aux_pwm = payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM];
    control.request_voltage = payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE];
    control.tx_id = TOY_REMOTE_GET_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L);
    return (control.tx_id == 0u) ? STC8H_ERROR : STC8H_OK;
}

static void handle_packet(void)
{
    stc8h_u8 save_binding;

    if (proto_rf_link_poll_data_fixed(&link, packet, payload) == STC8H_OK) {
        if (unpack_control_payload() == STC8H_OK) {
            save_binding = 0u;
            if (config.bound_tx_id == 0u) {
                config.bound_tx_id = control.tx_id;
                save_binding = 1u;
            } else if (control.tx_id != config.bound_tx_id) {
                return;
            }
            idle_polls = 0u;
            link_lost = 0u;
            app_indicator_set_state(&indicator, APP_INDICATOR_STATE_CONNECTED, app_tick_now());
            app_outputs_apply_control(&control);
            prepare_ack_status();
            if (save_binding != 0u) {
                (void)app_config_save(&config);
            }
            return;
        }
    }
}

static void handle_channel_buttons(void)
{
#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
    if (TOY_REMOTE_RX_RF_CH_ADD_ACTIVE() != 0u) {
        if (ch_add_pressed == 0u) {
            ch_add_pressed = 1u;
            config.rf_channel = toy_remote_channel_pool_next(config.rf_channel);
            app_radio_set_channel(config.rf_channel);
            (void)app_config_save(&config);
            apply_safe_state();
            app_indicator_set_state(&indicator, app_waiting_indicator_state(), app_tick_now());
            prepare_ack_status();
        }
    } else {
        ch_add_pressed = 0u;
    }

    if (TOY_REMOTE_RX_RF_CH_MINUS_ACTIVE() != 0u) {
        if (ch_minus_pressed == 0u) {
            ch_minus_pressed = 1u;
            config.rf_channel = toy_remote_channel_pool_prev(config.rf_channel);
            app_radio_set_channel(config.rf_channel);
            (void)app_config_save(&config);
            apply_safe_state();
            app_indicator_set_state(&indicator, app_waiting_indicator_state(), app_tick_now());
            prepare_ack_status();
        }
    } else {
        ch_minus_pressed = 0u;
    }
#endif
}

static void handle_idle_poll(void)
{
    if (idle_polls < APP_RECEIVER_IDLE_POLL_LIMIT) {
        ++idle_polls;
        if (idle_polls == APP_RECEIVER_IDLE_POLL_LIMIT) {
            apply_safe_state();
            app_indicator_set_state(&indicator, app_waiting_indicator_state(), app_tick_now());
        }
    }
}

void main(void)
{
    drv_nrf24l01_init_pins();
    stc8h_spi_init();
    app_outputs_init();
    app_indicator_init(&indicator, 0u);
    app_indicator_write(APP_INDICATOR_LED_ON);
    app_timer0_init_indicator_tick();
    app_status_init(&status);
    if (app_config_load(&config) != STC8H_OK) {
        (void)app_config_save(&config);
    }
#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
    P3M1 &= (stc8h_u8)~(TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK);
    P3M0 &= (stc8h_u8)~(TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK);
    if ((TOY_REMOTE_RX_RF_CH_ADD_ACTIVE() != 0u) && (TOY_REMOTE_RX_RF_CH_MINUS_ACTIVE() != 0u)) {
        config.bound_tx_id = 0u;
        (void)app_config_save(&config);
        app_indicator_set_state(&indicator, APP_INDICATOR_STATE_BINDING_CLEARED, app_tick_now());
    }
#endif
    status.tx_id = config.bound_tx_id;
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 2u, 1u);
    apply_safe_state();

    if (app_radio_init_rx(config.rf_channel) != STC8H_OK) {
        radio_error = 1u;
        app_indicator_set_state(&indicator, APP_INDICATOR_STATE_RADIO_ERROR, app_tick_now());
    } else {
        if (indicator.requested_state != APP_INDICATOR_STATE_BINDING_CLEARED) {
            app_indicator_set_state(&indicator, app_waiting_indicator_state(), app_tick_now());
        }
        prepare_ack_status();
    }

    while (1) {
        app_indicator_service();
#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
        handle_channel_buttons();
#endif
        if (radio_error == 0u) {
            if (app_radio_receive_packet(packet) == APP_RADIO_RX_PACKET) {
                handle_packet();
            } else {
                handle_idle_poll();
            }
        } else {
            apply_safe_state();
        }
    }
}
