#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/receiver/.pio/build/STC8H1K08"

if [ ! -f "$BUILD_DIR/firmware.map" ]; then
    echo "receiver firmware.map missing; run pio build first" >&2
    exit 1
fi

if rg -q "__mullong" "$BUILD_DIR/firmware.map" "$BUILD_DIR/src/app_status.rst"; then
    echo "receiver status voltage path must not pull in SDCC 32-bit multiply helper" >&2
    exit 1
fi
