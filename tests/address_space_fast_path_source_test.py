#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def assert_has(text: str, needle: str, label: str) -> None:
    assert needle in text, f"{label} must contain {needle}"


def assert_not_has(text: str, needle: str, label: str) -> None:
    assert needle not in text, f"{label} must not contain {needle}"


def main() -> None:
    controller_ini = read("controller/platformio.ini")
    receiver_ini = read("receiver/platformio.ini")
    controller_main = read("controller/src/main.c")
    receiver_main = read("receiver/src/main.c")
    controller_radio = read("controller/src/app_radio.c")
    receiver_radio = read("receiver/src/app_radio.c")
    receiver_control_diag = read("receiver/src/control_diag_main.c")
    receiver_gpio_diag = read("receiver/src/control_gpio_diag_main.c")
    controller_radio_diag = read("controller/src/radio_diag_main.c")

    for label, ini in (("controller", controller_ini), ("receiver", receiver_ini)):
        assert_has(ini, "-DPROTO_RF_LINK_ENABLE_INIT=0", label)
        assert_has(ini, "-DPROTO_RF_LINK_ENABLE_SET_IDS=0", label)
        assert_has(ini, "-DPROTO_RF_LINK_ENABLE_XDATA_FIXED_API=1", label)
        assert_has(ini, "-DDRV_NRF24L01_ENABLE_PIPE0_FIXED_API=0", label)
        assert_has(ini, "-DDRV_NRF24L01_ENABLE_CODE_ADDRESS_API=1", label)
        assert_has(ini, "-DDRV_NRF24L01_ENABLE_FIXED_PAYLOAD_API=0", label)
        assert_has(ini, "-DDRV_NRF24L01_ENABLE_XDATA_PAYLOAD_API=1", label)

    assert_has(controller_ini, "-DPROTO_RF_LINK_ENABLE_SEND_DATA_FIXED=0", "controller")
    assert_has(receiver_ini, "-DPROTO_RF_LINK_ENABLE_POLL_DATA_FIXED=0", "receiver")
    assert_has(controller_ini, "-DDRV_TM1637_ENABLE_DISPLAY_RAW4=0", "controller")
    assert_has(controller_ini, "-DDRV_TM1637_ENABLE_DISPLAY_RAW4_DATA=1", "controller")

    assert_has(controller_main, "proto_rf_link_init_xdata(&link);", "controller main")
    assert_has(controller_main, "proto_rf_link_set_ids_xdata(&link, 1u, 2u);", "controller main")
    assert_has(controller_main, "proto_rf_link_send_data_fixed_xdata(&link, packet, payload)", "controller main")
    assert_has(controller_main, "drv_tm1637_display_raw4_data(display_segments)", "controller main")
    assert_not_has(controller_main, "proto_rf_link_init(&link)", "controller main")
    assert_not_has(controller_main, "proto_rf_link_set_ids(&link", "controller main")
    assert_not_has(controller_main, "proto_rf_link_send_data_fixed(&link", "controller main")
    assert_not_has(controller_main, "drv_tm1637_display_raw4(display_segments)", "controller main")

    assert_has(receiver_main, "proto_rf_link_init_xdata(&link);", "receiver main")
    assert_has(receiver_main, "proto_rf_link_set_ids_xdata(&link, 2u, 1u);", "receiver main")
    assert_has(receiver_main, "proto_rf_link_poll_data_fixed_xdata(&link, packet, payload)", "receiver main")
    assert_not_has(receiver_main, "proto_rf_link_init(&link)", "receiver main")
    assert_not_has(receiver_main, "proto_rf_link_set_ids(&link", "receiver main")
    assert_not_has(receiver_main, "proto_rf_link_poll_data_fixed(&link", "receiver main")

    for label, source in (("receiver control diag", receiver_control_diag),
                          ("receiver gpio diag", receiver_gpio_diag)):
        assert_has(source, "proto_rf_link_init_xdata(&link);", label)
        assert_has(source, "proto_rf_link_set_ids_xdata(&link, 2u, 1u);", label)
        assert_has(source, "proto_rf_link_poll_data_fixed_xdata(&link, packet, payload)", label)
        assert_not_has(source, "proto_rf_link_init(&link)", label)
        assert_not_has(source, "proto_rf_link_set_ids(&link", label)
        assert_not_has(source, "proto_rf_link_poll_data_fixed(&link", label)

    for label, source in (("controller app radio", controller_radio),
                          ("receiver app radio", receiver_radio)):
        assert_has(source, "static DRV_NRF24L01_CODE_CONST stc8h_u8 app_radio_addr", label)
        assert_has(source, "drv_nrf24l01_config_pipe0_fixed_code(app_radio_addr)", label)
        assert_not_has(source, "drv_nrf24l01_config_pipe0_fixed(app_radio_addr)", label)

    assert_has(controller_radio, "const STC8H_XDATA stc8h_u8 *packet", "controller app radio")
    assert_has(controller_radio, "drv_nrf24l01_write_payload_fixed_xdata(packet)", "controller app radio")
    assert_not_has(controller_radio, "drv_nrf24l01_write_payload_fixed(packet)", "controller app radio")

    assert_has(receiver_radio, "app_radio_receive_packet(STC8H_XDATA stc8h_u8 *packet)", "receiver app radio")
    assert_has(receiver_radio, "drv_nrf24l01_read_payload_fixed_xdata(packet)", "receiver app radio")
    assert_not_has(receiver_radio, "drv_nrf24l01_read_payload_fixed(packet)", "receiver app radio")

    assert_has(controller_radio_diag, "static stc8h_u8 segments[4];", "controller radio diag")
    assert_has(controller_radio_diag, "drv_tm1637_display_raw4_data(segments)", "controller radio diag")
    assert_not_has(controller_radio_diag, "drv_tm1637_display_raw4(segments)", "controller radio diag")


if __name__ == "__main__":
    main()
