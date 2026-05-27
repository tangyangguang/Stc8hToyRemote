#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
APP_RADIO_SOURCE = ROOT / "controller" / "src" / "app_radio.c"
PLATFORMIO_INI = ROOT / "controller" / "platformio.ini"


def main() -> None:
    source = APP_RADIO_SOURCE.read_text(encoding="utf-8")
    ini = PLATFORMIO_INI.read_text(encoding="utf-8")

    assert "drv_nrf24l01_read_fifo_status(" in source, (
        "controller TX completion must check RX FIFO so ACK payloads queued there "
        "are not misclassified as ACK_EMPTY when STATUS.RX_DR lags"
    )
    assert "-DDRV_NRF24L01_ENABLE_READ_FIFO_STATUS=1" in ini
    assert "-DDRV_NRF24L01_ENABLE_TX_RESULT_API=0" in ini
    assert "-DDRV_NRF24L01_ENABLE_READ_OBSERVE_TX=0" in ini


if __name__ == "__main__":
    main()
