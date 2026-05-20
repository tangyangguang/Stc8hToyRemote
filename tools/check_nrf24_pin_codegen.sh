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

(cd "$ROOT_DIR/controller" && pio run -c platformio_diag.ini -e STC8H1K08_radio_diag)
(cd "$ROOT_DIR/receiver" && pio run)

check_rst "controller radio_diag" "$ROOT_DIR/controller/.pio/build/STC8H1K08_radio_diag/src/drv_nrf24l01_wrap.rst"
check_rst "receiver" "$ROOT_DIR/receiver/.pio/build/STC8H1K08/src/drv_nrf24l01_wrap.rst"
