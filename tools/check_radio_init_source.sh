#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

if grep -q "DRV_NRF24L01_ENABLE_CHECK_PRESENT=0" "$ROOT_DIR/controller/platformio.ini"; then
    echo "controller must keep nRF24 check_present enabled for E001 init failure detection" >&2
    exit 1
fi

if ! grep -q "drv_nrf24l01_check_present() != STC8H_OK" "$ROOT_DIR/controller/src/app_radio.c"; then
    echo "controller app_radio_init_tx must fail when nRF24 check_present fails" >&2
    exit 1
fi

if grep -q "(void)drv_nrf24l01_enable_ack_payload" "$ROOT_DIR/controller/src/app_radio.c" \
   "$ROOT_DIR/receiver/src/app_radio.c"; then
    echo "app_radio init must not ignore enable_ack_payload failure" >&2
    exit 1
fi

if ! grep -q "drv_nrf24l01_enable_ack_payload(DRV_NRF24L01_PIPE0) != STC8H_OK" \
    "$ROOT_DIR/controller/src/app_radio.c"; then
    echo "controller app_radio_init_tx must fail when ACK payload cannot be enabled" >&2
    exit 1
fi

if ! grep -q "drv_nrf24l01_enable_ack_payload(DRV_NRF24L01_PIPE0) != STC8H_OK" \
    "$ROOT_DIR/receiver/src/app_radio.c"; then
    echo "receiver app_radio_init_rx must fail when ACK payload cannot be enabled" >&2
    exit 1
fi
