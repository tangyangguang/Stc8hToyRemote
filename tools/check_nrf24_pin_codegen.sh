#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

check_rst() {
    label=$1
    rst_file=$2

    if [ ! -f "$rst_file" ]; then
        echo "$label nRF24 build artifact not found: $rst_file" >&2
        exit 1
    fi
    if ! grep -Eq 'anl[[:space:]]+_P1M0,#0xbb' "$rst_file"; then
        echo "$label nRF24 init does not set CE/CSN P1M0 bits to quasi-bidirectional" >&2
        exit 1
    fi
    if ! grep -Eq 'anl[[:space:]]+_P1M1,#0xbb' "$rst_file"; then
        echo "$label nRF24 init does not set CE/CSN P1M1 bits to quasi-bidirectional" >&2
        exit 1
    fi
    if ! grep -Eq 'orl[[:space:]]+_P1,#0x44' "$rst_file"; then
        echo "$label nRF24 init does not release CE/CSN latches before selecting idle levels" >&2
        exit 1
    fi
    if ! grep -Eq 'anl[[:space:]]+_P1,#0xbf' "$rst_file"; then
        echo "$label nRF24 init does not drive CE low after pin mode configuration" >&2
        exit 1
    fi
}

check_early_init_order() {
    label=$1
    rst_file=$2

    if [ ! -f "$rst_file" ]; then
        echo "$label main build artifact not found: $rst_file" >&2
        exit 1
    fi

    nrf_line=$(awk '/lcall[[:space:]]+_drv_nrf24l01_init_pins/{ print NR; exit }' "$rst_file")
    spi_line=$(awk '/lcall[[:space:]]+_stc8h_spi_init/{ print NR; exit }' "$rst_file")

    if [ -z "$nrf_line" ]; then
        echo "$label must drive nRF24 CE/CSN idle levels directly at boot" >&2
        exit 1
    fi
    if [ -z "$spi_line" ]; then
        echo "$label does not call stc8h_spi_init()" >&2
        exit 1
    fi
    if [ "$nrf_line" -gt "$spi_line" ]; then
        echo "$label initializes SPI before forcing nRF24 CE/CSN idle levels" >&2
        exit 1
    fi
}

check_receiver_ack_preload_loop() {
    rst_file=$1

    if ! awk '
        /lcall[[:space:]]+_drv_nrf24l01_write_ack_payload/ { saw_write = 1 }
        saw_write && /cjne[[:space:]]+r[0-7],#0x03/ { saw_count = 1 }
        END { exit saw_count ? 0 : 1 }
    ' "$rst_file"; then
        echo "receiver must preload all 3 nRF24 ACK payload FIFO slots" >&2
        exit 1
    fi

    if ! grep -Eq 'mov[[:space:]]+_drv_nrf24l01_write_ack_payload_PARM_3,#0x0f' "$rst_file"; then
        echo "receiver must write 15-byte status ACK payloads" >&2
        exit 1
    fi
}

check_controller_ack_read_len() {
    rst_file=$1

    if ! grep -Eq 'mov[[:space:]]+_drv_nrf24l01_read_payload_PARM_2,#0x0f' "$rst_file"; then
        echo "controller must read fixed 15-byte status ACK payloads in normal build" >&2
        exit 1
    fi
    if ! grep -Eq 'mov[[:space:]]+_app_radio_ack_len,#0x0f' "$rst_file"; then
        echo "controller must report fixed 15-byte status ACK length in normal build" >&2
        exit 1
    fi
}

check_receiver_runtime_channel() {
    rst_file=$1

    if ! awk '
        /mov[[:space:]]+dpl,[[:space:]]*#0x4c/ { saw_default_channel = NR }
        /lcall[[:space:]]+_app_radio_init_rx/ {
            if ((saw_default_channel > 0) && ((NR - saw_default_channel) <= 3)) {
                saw_init = 1
            }
        }
        END { exit saw_init ? 0 : 1 }
    ' "$rst_file"; then
        echo "receiver default build must initialize nRF24 RX on fixed channel 76" >&2
        exit 1
    fi
}

(cd "$ROOT_DIR/controller" && pio run)
check_early_init_order "controller" "$ROOT_DIR/controller/.pio/build/STC8H1K08/src/main.rst"
check_rst "controller" "$ROOT_DIR/controller/.pio/build/STC8H1K08/src/drv_nrf24l01_wrap.rst"
check_controller_ack_read_len "$ROOT_DIR/controller/.pio/build/STC8H1K08/src/app_radio.rst"

(cd "$ROOT_DIR/controller" && pio run -c platformio_diag.ini -e STC8H1K08_radio_diag)
check_early_init_order "controller radio_diag" "$ROOT_DIR/controller/.pio/build/STC8H1K08_radio_diag/src/radio_diag_main.rst"
check_rst "controller radio_diag" "$ROOT_DIR/controller/.pio/build/STC8H1K08_radio_diag/src/drv_nrf24l01_wrap.rst"

(cd "$ROOT_DIR/receiver" && pio run)
check_early_init_order "receiver" "$ROOT_DIR/receiver/.pio/build/STC8H1K08/src/main.rst"
check_rst "receiver" "$ROOT_DIR/receiver/.pio/build/STC8H1K08/src/drv_nrf24l01_wrap.rst"
check_receiver_ack_preload_loop "$ROOT_DIR/receiver/.pio/build/STC8H1K08/src/main.rst"
check_receiver_runtime_channel "$ROOT_DIR/receiver/.pio/build/STC8H1K08/src/main.rst"
