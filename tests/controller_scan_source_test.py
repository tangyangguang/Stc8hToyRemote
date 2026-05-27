#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "controller" / "src" / "main.c"


def function_body(source: str, name: str) -> str:
    marker = f"static stc8h_u8 {name}(void)"
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
    raise AssertionError(f"{name} body not found")


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    send_body = function_body(source, "send_control_packet")
    probe_body = function_body(source, "probe_current_channel")

    assert "APP_RADIO_TX_ACK_PAYLOAD_OK" in send_body
    assert "return 2u;" in send_body
    assert "for (i = 0u; i < 2u; ++i)" in probe_body
    assert "if (result == 1u)" in probe_body
    assert "if (result == 2u)" in probe_body
    assert "stc8h_delay_ms(5u);" in probe_body

    ack_delay = probe_body.index("if (result == 2u)")
    fixed_delay = probe_body.index("stc8h_delay_ms(5u);")
    assert ack_delay < fixed_delay, "5ms scan delay must only follow app-level ACK mismatch"


if __name__ == "__main__":
    main()
