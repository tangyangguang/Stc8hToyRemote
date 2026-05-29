#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "controller" / "src" / "main.c"
INPUT_SOURCE = ROOT / "controller" / "src" / "app_input.c"


def body_from_marker(source: str, marker: str) -> str:
    start = source.index(marker)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:pos]
    raise AssertionError(f"{marker} body not found")


def function_body(source: str, name: str) -> str:
    marker = f"{name}(void)"
    start = source.rindex("static ", 0, source.index(marker))
    return body_from_marker(source, source[start:source.index("{", start)].strip())


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    input_source = INPUT_SOURCE.read_text(encoding="utf-8")
    button_source = (ROOT / "controller" / "src" / "app_button.h").read_text(encoding="utf-8")
    assert "#define APP_BUTTON_RELEASE_TICKS 20u" in source
    assert "#define APP_BUTTON_FIXED_LONG_TICKS 10000u" in source
    assert "#define APP_BUTTON_LONG_NORMAL_TICKS APP_BUTTON_FIXED_LONG_TICKS" in source
    assert "#define APP_BUTTON_DOUBLE_TICKS 1000u" in source
    assert "#define APP_SCAN_PROBE_PACKETS 2u" in source
    assert "#define APP_SCAN_POOL_PASSES 1u" in source
    assert "#define APP_LOOP_INTERVAL_MS 20u" in source
    assert "APP_INPUT_ADC_DIVIDER" not in input_source
    assert "APP_BUTTON_LONG_CONFIG_TICKS" not in source

    send_body = function_body(source, "send_control_packet")
    ack_body = function_body(source, "handle_ack_status")
    probe_body = function_body(source, "probe_current_channel")
    scan_body = function_body(source, "scan_channels")
    ui_body = function_body(source, "run_ui_slice")
    enter_config_body = function_body(source, "enter_config_mode")
    exit_config_body = function_body(source, "exit_config_mode_save")
    input_update_body = body_from_marker(input_source, "stc8h_s16 app_input_update(STC8H_XDATA toy_remote_control_t *control)")

    assert "APP_RADIO_TX_ACK_PAYLOAD_OK" in send_body
    assert "return 2u;" in send_body
    assert "body[TOY_REMOTE_STATUS_OFFSET_LINK_STATE] != TOY_REMOTE_LINK_STATE_CONNECTED" in ack_body
    assert "rx_status.tx_id" not in source
    assert "(void)send_control_packet();" not in source
    assert "app_input_update_speed(&control);" in send_body
    assert "app_input_update_discrete(&control);" in send_body
    assert "app_input_update_steering(&control);" in send_body
    assert send_body.index("app_input_update_speed(&control);") < send_body.index("app_input_update_discrete(&control);")
    assert send_body.index("app_input_update_discrete(&control);") < send_body.index("make_control_packet();")
    assert send_body.index("app_input_update_steering(&control);") < send_body.index("make_control_packet();")
    assert "stc8h_s16 app_input_update_speed(STC8H_XDATA toy_remote_control_t *control)" in input_source
    assert "void app_input_update_discrete(STC8H_XDATA toy_remote_control_t *control)" in input_source
    assert "app_input_update_steering(control);" not in input_update_body
    assert "static void app_button_init(STC8H_DATA app_button_t *button)" in button_source
    assert "static app_button_event_t app_button_update_elapsed(STC8H_DATA app_button_t *button" in button_source
    assert "for (i = 0u; i < APP_SCAN_PROBE_PACKETS; ++i)" in probe_body
    assert "if (result == 1u)" in probe_body
    assert "if (result == 2u)" in probe_body
    assert "stc8h_delay_ms(5u);" in probe_body

    ack_delay = probe_body.index("if (result == 2u)")
    fixed_delay = probe_body.index("stc8h_delay_ms(5u);")
    assert ack_delay < fixed_delay, "5ms scan delay must only follow app-level ACK mismatch"

    pool_pass_loop = scan_body.index("pool_pass < APP_SCAN_POOL_PASSES")
    pool_loop = scan_body.index("index < TOY_REMOTE_CHANNEL_POOL_COUNT")
    full_loop = scan_body.index("channel <= 125u")
    assert pool_pass_loop < pool_loop
    assert pool_loop < full_loop, "scan must try the built-in channel pool before full fallback"
    assert "toy_remote_channel_pool_value(index)" in scan_body
    assert "scan_one_channel(channel)" in scan_body
    assert "((channel & 0x03u) != 0u)" not in scan_body

    assert "app_button_update_elapsed(&ec11_button" in source
    assert "app_input_tick_half_ms()" in ui_body
    assert "app_button_init(&ec11_button)" not in enter_config_body
    assert "app_button_init(&ec11_button)" not in exit_config_body
    assert "ec11_button_press_speed" in source
    assert "ec11_active" in ui_body
    assert "ec11_button_press_speed = control.speed;" in ui_body
    assert ui_body.index("ec11_button_press_speed = control.speed;") < ui_body.index("app_input_update(&control)")
    assert ui_body.index("app_input_tick_half_ms()") < ui_body.index("app_button_update_elapsed(&ec11_button")
    assert "button_event == APP_BUTTON_EVENT_LONG" in ui_body
    assert "ec11_button_press_speed == 0u" in ui_body
    assert "TOY_REMOTE_TX_BRAKE_ACTIVE() && TOY_REMOTE_TX_EC11_SW_ACTIVE()" not in source
    assert "TOY_REMOTE_TX_BRAKE_ACTIVE() || TOY_REMOTE_TX_EC11_SW_ACTIVE()" not in source


if __name__ == "__main__":
    main()
