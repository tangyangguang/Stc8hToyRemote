#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN_SOURCE = ROOT / "receiver" / "src" / "main.c"
CONFIG_H = ROOT / "receiver" / "src" / "app_config.h"
CORE_DOC = ROOT / "遥控器与接收机核心逻辑说明.md"
OPS_DOC = ROOT / "遥控器操作与界面配置说明.md"


def function_body(source: str, name: str) -> str:
    marker = f"{name}(void)"
    start = source.rindex("static ", 0, source.index(marker))
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:pos]
    raise AssertionError(f"{name} body not found")


def function_body_by_signature(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:pos]
    raise AssertionError(f"{signature} body not found")


def block_after(source: str, marker: str) -> str:
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
    raise AssertionError(f"{marker} block not found")


def main() -> None:
    source = MAIN_SOURCE.read_text(encoding="utf-8")
    config_h = CONFIG_H.read_text(encoding="utf-8")
    core_doc = CORE_DOC.read_text(encoding="utf-8")
    ops_doc = OPS_DOC.read_text(encoding="utf-8")

    assert "#define APP_RECEIVER_ENABLE_CHANNEL_BUTTONS 1" in config_h
    assert "#define APP_RECEIVER_CHANNEL_BUTTON_DEBOUNCE_MS 30u" in source
    assert "P3 |= (TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK);" in source
    assert "P_SW2 |= 0x80u;" in source
    assert "P3IE |= (TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK);" in source
    assert "P3PU |= (TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK);" in source

    change_body = function_body_by_signature(source, "static void apply_receiver_channel_change(stc8h_u8 channel)")
    assert "if (channel == config.rf_channel)" in change_body
    assert "config.rf_channel = channel;" in change_body
    assert "app_radio_set_channel(config.rf_channel);" in change_body
    assert "apply_safe_state();" in change_body
    assert "app_indicator_set_state(&indicator, app_waiting_indicator_state(), app_tick_now());" in change_body
    assert "prepare_ack_status(APP_RADIO_ACK_PAYLOAD_REPLACE_ON_RECOVER);" in change_body
    assert "(void)app_config_save(&config);" in change_body
    assert change_body.index("config.rf_channel = channel;") < change_body.index("app_radio_set_channel(config.rf_channel);")
    assert change_body.index("app_radio_set_channel(config.rf_channel);") < change_body.index("apply_safe_state();")
    assert change_body.index("apply_safe_state();") < change_body.index("prepare_ack_status(APP_RADIO_ACK_PAYLOAD_REPLACE_ON_RECOVER);")

    button_body = function_body(source, "handle_channel_buttons")
    assert "button_bits = (stc8h_u8)(P3 & (TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK));" in button_body
    assert "next_state = (stc8h_u8)(button_bits ^ (TOY_REMOTE_RX_RF_CH_ADD_MASK | TOY_REMOTE_RX_RF_CH_MINUS_MASK));" in button_body
    assert "APP_RECEIVER_CHANNEL_BUTTON_BLOCKED" in button_body
    assert "channel_button_sample" in button_body
    assert "channel_button_stable" in button_body
    assert "if (channel_button_sample != next_state)" in button_body
    assert "elapsed_ms < APP_RECEIVER_CHANNEL_BUTTON_DEBOUNCE_MS" in button_body
    assert "if (channel_button_stable == next_state)" in button_body
    assert "toy_remote_channel_pool_next(config.rf_channel)" in button_body
    assert "toy_remote_channel_pool_prev(config.rf_channel)" in button_body
    assert "apply_receiver_channel_change(toy_remote_channel_pool_next(config.rf_channel));" in button_body
    assert "apply_receiver_channel_change(toy_remote_channel_pool_prev(config.rf_channel));" in button_body

    clear_block = block_after(
        source,
        "if ((TOY_REMOTE_RX_RF_CH_ADD_ACTIVE() != 0u) && (TOY_REMOTE_RX_RF_CH_MINUS_ACTIVE() != 0u))",
    )
    assert "config.bound_tx_id = 0u;" in clear_block
    assert "app_config_save(&config)" in clear_block
    assert "config.rf_channel" not in clear_block

    for doc in (core_doc, ops_doc):
        assert "接收机本机频道操作" in doc
        assert "稳定约 30ms" in doc
        assert "内部上拉" in doc
        assert "烧录时不要按住" in doc
        assert "P30+P31" in doc
        assert "不清绑定、不切频道" in doc


if __name__ == "__main__":
    main()
