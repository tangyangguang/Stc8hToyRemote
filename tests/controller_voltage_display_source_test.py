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
    marker = f"{name}("
    start = source.rindex("static ", 0, source.index(marker))
    return body_from_marker(source, source[start:source.index("{", start)].strip())


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    voltage_body = function_body(source, "update_voltage_display")
    ui_body = function_body(source, "run_ui_slice")

    assert "#define APP_VOLTAGE_DISPLAY_128MS_TICKS 12u" in source
    assert "#define APP_VOLTAGE_LABEL_128MS_TICKS 2u" in source
    assert "static stc8h_u8 voltage_display_active;" in source
    assert "static stc8h_u8 voltage_display_start_128ms;" in source
    assert "static stc8h_u16 rx_battery_centivolts;" in source
    assert "static STC8H_XDATA toy_remote_status_t rx_status;" not in source
    assert "static STC8H_BIT rx_voltage_valid;" in source
    assert "static void update_voltage_display(stc8h_u16 now_half_ms)" in source

    assert "if (control.request_voltage != 0u)" in ui_body
    assert "update_voltage_display(ui_tick_half_ms);" in ui_body
    assert "(app_state == APP_STATE_CONNECTED) && (control.request_voltage != 0u)" not in ui_body
    assert ui_body.index("if (control.request_voltage != 0u)") < ui_body.index("if (app_state == APP_STATE_TRY_SAVED)")
    assert "voltage_display_active = 0u;" in ui_body
    assert ui_body.index("voltage_display_active = 0u;") < ui_body.index("if (app_state == APP_STATE_TRY_SAVED)")

    assert "voltage_display_active = 1u;" in voltage_body
    assert "show_rx_voltage = 0u;" in voltage_body
    assert "now_128ms = (stc8h_u8)(now_half_ms >> 8);" in voltage_body
    assert "elapsed_128ms = (stc8h_u8)(now_128ms - voltage_display_start_128ms);" in voltage_body
    assert "elapsed_128ms = 0u;" in voltage_body
    assert voltage_body.index("elapsed_128ms >= APP_VOLTAGE_DISPLAY_128MS_TICKS") < voltage_body.index("if (elapsed_128ms < APP_VOLTAGE_LABEL_128MS_TICKS)")
    assert "if (elapsed_128ms < APP_VOLTAGE_LABEL_128MS_TICKS)" in voltage_body
    assert "APP_DISPLAY_H" in voltage_body
    assert "app_display_source_segments(APP_DISPLAY_DASH, display_segments);" in voltage_body
    assert "rx_voltage_valid == 0u" in voltage_body
    assert "tx_battery_centivolts = app_input_read_tx_battery_centivolts();" in voltage_body
    assert voltage_body.index("voltage_display_active == 0u") < voltage_body.index("if (elapsed_128ms < APP_VOLTAGE_LABEL_128MS_TICKS)")
    assert "elapsed_128ms >= APP_VOLTAGE_DISPLAY_128MS_TICKS" in voltage_body
    assert "APP_VOLTAGE_DISPLAY_TICKS" not in source
    assert "APP_VOLTAGE_LABEL_TICKS" not in source
    assert "display_voltage(rx_battery_centivolts);" in voltage_body
    assert "rx_voltage_valid = 1u;" in source


if __name__ == "__main__":
    main()
