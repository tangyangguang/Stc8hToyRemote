#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "controller" / "src" / "main.c"


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
    voltage_body = function_body(source, "update_voltage_display")
    ui_body = function_body(source, "run_ui_slice")

    assert "#define APP_VOLTAGE_DISPLAY_TICKS 100u" in source
    assert "#define APP_VOLTAGE_LABEL_TICKS 25u" in source
    assert "static stc8h_u8 voltage_display_active;" in source
    assert "static void reset_voltage_display(void)" in source
    assert "static void display_voltage_source(stc8h_u8 source)" in source

    assert "if (control.request_voltage != 0u)" in ui_body
    assert "(app_state == APP_STATE_CONNECTED) && (control.request_voltage != 0u)" not in ui_body
    assert ui_body.index("if (control.request_voltage != 0u)") < ui_body.index("if (app_state == APP_STATE_TRY_SAVED)")
    assert "reset_voltage_display();" in ui_body
    assert ui_body.index("reset_voltage_display();") < ui_body.index("if (app_state == APP_STATE_TRY_SAVED)")

    assert "voltage_display_active = 1u;" in voltage_body
    assert "show_rx_voltage = 0u;" in voltage_body
    assert "if (voltage_display_divider < APP_VOLTAGE_LABEL_TICKS)" in voltage_body
    assert "display_voltage_source(APP_DISPLAY_R)" in voltage_body
    assert "display_voltage_source(APP_DISPLAY_T)" in voltage_body
    assert "tx_battery_centivolts = app_input_read_tx_battery_centivolts();" in voltage_body
    assert voltage_body.index("voltage_display_active == 0u") < voltage_body.index("if (voltage_display_divider < APP_VOLTAGE_LABEL_TICKS)")
    assert voltage_body.index("if (voltage_display_divider < APP_VOLTAGE_LABEL_TICKS)") < voltage_body.index("++voltage_display_divider")
    assert "voltage_display_divider >= APP_VOLTAGE_DISPLAY_TICKS" in voltage_body
    assert "rx_status.voltage_int" in voltage_body
    assert "rx_status.voltage_dec" in voltage_body


if __name__ == "__main__":
    main()
