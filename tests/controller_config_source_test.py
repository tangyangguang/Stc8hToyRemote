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
    platformio = (ROOT / "controller" / "platformio.ini").read_text(encoding="utf-8")

    assert "#define APP_CONFIG_VERSION 4u" in config_c
    assert "#define APP_CONFIG_LEN 12u" in config_c
    assert "#define APP_CONFIG_LEGACY_DEADBAND_VERSION 3u" in config_c
    assert "#define APP_CONFIG_LEGACY_DEADBAND_LEN 11u" in config_c
    assert "-DSTC8H_EEPROM_FIXED_SIZE=12" in platformio
    assert "steering_trim" in config_h
    assert "steering_trim" in config_c
    assert "steering_trim" in main_c
    assert "steering_deadband" in config_h
    assert "steering_deadband" in config_c
    assert "steering_deadband" in main_c
    assert "steering_middle" not in config_h
    assert "APP_CONFIG_STEERING_MIDDLE" not in config_h
    assert "#define APP_CONFIG_ITEM_STEERING_TRIM 3u" in main_c
    assert "#define APP_CONFIG_ITEM_DEADBAND 4u" in main_c
    assert "#define APP_CONFIG_ITEM_REDUCE 5u" in main_c
    assert "app_config_buf[8] = (stc8h_u8)(config->steering_trim + APP_CONFIG_STEERING_TRIM_STORAGE_BIAS);" in config_c
    assert "app_config_buf[9] = config->steering_deadband;" in config_c
    assert "app_config_buf[10] = config->steering_reduce;" in config_c
    assert "config->steering_trim = (stc8h_s8)((stc8h_s8)app_config_buf[8] - APP_CONFIG_STEERING_TRIM_STORAGE_BIAS);" in config_c
    assert "config->steering_deadband = app_config_buf[9];" in config_c
    assert "config->steering_reduce = app_config_buf[10];" in config_c
    assert "config->steering_trim = APP_CONFIG_DEFAULT_STEERING_TRIM;" in config_c
    assert "config->steering_reduce = app_config_buf[8];" in config_c


if __name__ == "__main__":
    main()
