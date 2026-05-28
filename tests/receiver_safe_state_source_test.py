#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN_SOURCE = ROOT / "receiver" / "src" / "main.c"
OUTPUTS_SOURCE = ROOT / "receiver" / "src" / "app_outputs.c"


def function_body(source: str, signature: str) -> str:
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


def main() -> None:
    main_source = MAIN_SOURCE.read_text(encoding="utf-8")
    outputs_source = OUTPUTS_SOURCE.read_text(encoding="utf-8")

    apply_safe_state = function_body(main_source, "static void apply_safe_state(void)")
    app_outputs_apply_safe = function_body(outputs_source, "void app_outputs_apply_safe(void)")

    assert "control.speed = 0u;" in apply_safe_state
    assert "control.brake = 0u;" in apply_safe_state
    assert "control.light = 0u;" in apply_safe_state
    assert "control.buzzer = 0u;" in apply_safe_state
    assert "control.aux_pwm = 0u;" in apply_safe_state
    assert "control.steering_angle" not in apply_safe_state

    assert "stc8h_pwm_set_duty_b6(0u);" in app_outputs_apply_safe
    assert "stc8h_pwm_set_duty_b7(0u);" in app_outputs_apply_safe
    assert "stc8h_pwm_set_duty_b8(0u);" in app_outputs_apply_safe
    assert "TOY_REMOTE_RX_MOTOR_STOP();" in app_outputs_apply_safe
    assert "TOY_REMOTE_RX_LIGHT_OFF();" in app_outputs_apply_safe
    assert "TOY_REMOTE_RX_BUZZER_OFF();" in app_outputs_apply_safe
    assert "stc8h_pwm_set_duty_a1" not in app_outputs_apply_safe
    assert "APP_OUTPUT_SERVO_CENTER_DUTY" not in app_outputs_apply_safe
    assert "APP_OUTPUT_SERVO_DUTY" not in app_outputs_apply_safe
    assert "app_outputs_write_pwm" not in app_outputs_apply_safe


if __name__ == "__main__":
    main()
