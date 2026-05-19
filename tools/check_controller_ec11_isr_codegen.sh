#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/controller/.pio/build/STC8H1K08"
APP_INPUT_C="$ROOT_DIR/controller/src/app_input.c"
APP_EC11_SPEED_H="$ROOT_DIR/controller/src/app_ec11_speed.h"
CONTROLLER_PLATFORMIO="$ROOT_DIR/controller/platformio.ini"
APP_INPUT_RST="$BUILD_DIR/src/app_input.rst"
DRV_EC11_RST="$BUILD_DIR/src/drv_ec11_wrap.rst"
MAP_FILE="$BUILD_DIR/firmware.map"

if [ ! -f "$APP_INPUT_RST" ] || [ ! -f "$DRV_EC11_RST" ] || [ ! -f "$MAP_FILE" ]; then
    echo "controller build artifacts not found; run controller pio build first" >&2
    exit 1
fi

grep -q "STC8H_DATA drv_ec11_small_t speed_encoder" "$APP_INPUT_C" || {
    echo "controller EC11 ISR state must be internal DATA, not generic/XDATA" >&2
    exit 1
}

grep -q "drv_ec11_small_init_isr" "$APP_INPUT_C" || {
    echo "controller must initialize EC11 through drv_ec11_small_init_isr()" >&2
    exit 1
}

grep -q "drv_ec11_scan_delta_small_isr" "$APP_INPUT_C" || {
    echo "controller Timer0 ISR must use drv_ec11_scan_delta_small_isr()" >&2
    exit 1
}

grep -q "DRV_EC11_ENABLE_SMALL_API=0" "$CONTROLLER_PLATFORMIO" || {
    echo "controller must disable ordinary drv_ec11 small API" >&2
    exit 1
}

grep -q "DRV_EC11_ENABLE_SMALL_ISR_API=1" "$CONTROLLER_PLATFORMIO" || {
    echo "controller must enable drv_ec11 small ISR API" >&2
    exit 1
}

if grep -q "delta[[:space:]]*\\*[[:space:]]*step" "$APP_EC11_SPEED_H"; then
    echo "controller EC11 ISR speed scaling must not call multiplication helpers" >&2
    exit 1
fi

if grep -q "app_ec11_speed_scale_delta" "$APP_INPUT_C"; then
    echo "controller Timer0 ISR must not call non-reentrant app speed scaling helper" >&2
    exit 1
fi

if grep -q "_app_ec11_speed_scale_delta" "$APP_INPUT_RST"; then
    echo "controller build must not emit app speed scaling helper overlay" >&2
    exit 1
fi

if awk '/_app_input_encoder_tick_isr:/{in_isr=1} /_app_input_update:/{in_isr=0} in_isr {print}' "$APP_INPUT_RST" |
    grep -q "lcall[[:space:]]*__"; then
    echo "controller Timer0 ISR must not call SDCC arithmetic/runtime helpers" >&2
    exit 1
fi

grep -q "_drv_ec11_scan_delta_small_isr" "$APP_INPUT_RST" || {
    echo "controller build does not call drv_ec11_scan_delta_small_isr()" >&2
    exit 1
}

if grep -q "_drv_ec11_scan_delta_small_PARM_" "$MAP_FILE" "$APP_INPUT_RST" "$DRV_EC11_RST"; then
    echo "ordinary drv_ec11_scan_delta_small parameter overlay is still linked" >&2
    exit 1
fi

if grep -q "_drv_ec11_transition_PARM_2" "$MAP_FILE" "$DRV_EC11_RST"; then
    echo "EC11 transition helper overlay is still linked into controller" >&2
    exit 1
fi
