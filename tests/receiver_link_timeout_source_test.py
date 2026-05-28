#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "receiver" / "src" / "main.c"


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


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    assert "APP_RECEIVER_IDLE_POLL_LIMIT" not in source
    assert "idle_polls" not in source
    assert "#define APP_RECEIVER_LINK_TIMEOUT_MS 300u" in source
    assert "static stc8h_u16 last_packet_tick;" in source

    handle_packet = function_body(source, "handle_packet")
    handle_idle = function_body(source, "handle_idle_poll")

    assert "last_packet_tick = app_tick_now();" in handle_packet
    assert handle_packet.index("last_packet_tick = app_tick_now();") < handle_packet.index("link_lost = 0u;")
    assert "elapsed_ms = (stc8h_u16)(app_tick_now() - last_packet_tick);" in handle_idle
    assert "elapsed_ms >= APP_RECEIVER_LINK_TIMEOUT_MS" in handle_idle
    assert "if (link_lost != 0u)" in handle_idle
    assert handle_idle.index("if (link_lost != 0u)") < handle_idle.index("elapsed_ms =")
    assert "apply_safe_state();" in handle_idle


if __name__ == "__main__":
    main()
