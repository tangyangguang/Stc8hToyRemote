#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONFIG_C = ROOT / "controller" / "src" / "app_config.c"
CONFIG_H = ROOT / "controller" / "src" / "app_config.h"
MAIN_C = ROOT / "controller" / "src" / "main.c"


def main() -> None:
    config_c = CONFIG_C.read_text(encoding="utf-8")
    config_h = CONFIG_H.read_text(encoding="utf-8")
    main_c = MAIN_C.read_text(encoding="utf-8")

    assert "#define APP_CONFIG_VERSION 3u" in config_c
    assert "steering_deadband" in config_h
    assert "steering_deadband" in config_c
    assert "steering_deadband" in main_c
    assert "steering_middle" not in config_h
    assert "APP_CONFIG_STEERING_MIDDLE" not in config_h
    assert "app_config_buf[9] = config->steering_deadband;" in config_c
    assert "config->steering_deadband = app_config_buf[9];" in config_c


if __name__ == "__main__":
    main()
