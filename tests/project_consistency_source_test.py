#!/usr/bin/env python3
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_controller_timing_docs_match_source() -> None:
    docs = "\n".join(
        read(path)
        for path in (
            "docs/01-requirements.md",
            "docs/04-logic-flows.md",
            "docs/05-power.md",
            "docs/07-ec11-input.md",
            "遥控器与接收机核心逻辑说明.md",
        )
    )
    assert "50ms" not in docs
    assert "160ms" not in docs
    assert "6.25Hz" not in docs
    assert "约 20Hz" not in docs
    assert "每秒约发送 20" not in docs
    assert "20ms" in docs
    assert "约 50Hz" in docs


def test_receiver_safe_state_docs_match_source() -> None:
    doc = read("遥控器与接收机核心逻辑说明.md")
    assert "APP_RECEIVER_IDLE_POLL_LIMIT" not in doc
    assert "空轮询" not in doc
    assert "安全态中位" not in doc
    assert "安全中位" not in doc
    assert "300ms" in doc
    assert "不改舵机" in doc


def test_control_gpio_diag_uses_single_bit_writes() -> None:
    source = read("receiver/src/control_gpio_diag_main.c")
    assert "TOY_REMOTE_RX_MOTOR_IN1_BIT" in source
    assert "TOY_REMOTE_RX_MOTOR_IN2_BIT" in source
    assert not re.search(r"\bP3\s*(?:\|=|&=|=)", source)


def test_speed_update_uses_signed_temp_before_clamp() -> None:
    source = read("controller/src/app_input.c")
    assert "stc8h_s16 speed;" in source
    assert "speed = (stc8h_s16)((stc8h_s16)control->speed + delta);" in source
    assert "(stc8h_u16)((stc8h_s16)control->speed + delta)" not in source


def test_controller_excludes_unused_channel_pool_helpers() -> None:
    source = read("controller/src/main.c")
    assert "#define TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT 0" in source
    assert "#define TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV 0" in source


def test_receiver_excludes_unused_channel_pool_helper() -> None:
    source = read("receiver/src/main.c")
    assert "#define TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE 0" in source


if __name__ == "__main__":
    test_controller_timing_docs_match_source()
    test_receiver_safe_state_docs_match_source()
    test_control_gpio_diag_uses_single_bit_writes()
    test_speed_update_uses_signed_temp_before_clamp()
    test_controller_excludes_unused_channel_pool_helpers()
    test_receiver_excludes_unused_channel_pool_helper()
