#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MAIN_C="$ROOT_DIR/controller/src/main.c"
DIAG_INI="$ROOT_DIR/controller/platformio_diag.ini"

diag_body=$(awk '
    /static void display_input_diag/ { in_diag = 1 }
    in_diag { print }
    in_diag && /^}/ { exit }
' "$MAIN_C")

require_in_diag_body() {
    pattern=$1
    message=$2
    if ! printf '%s\n' "$diag_body" | grep -q "$pattern"; then
        echo "controller input diag missing: $message" >&2
        exit 1
    fi
}

require_in_diag_body 'TOY_REMOTE_TX_EC11_SW_ACTIVE' 'EC11 switch state'
require_in_diag_body 'TOY_REMOTE_TX_BRAKE_ACTIVE' 'P3.0 brake switch state'
require_in_diag_body 'TOY_REMOTE_TX_DIR_REVERSE' 'direction switch state'
require_in_diag_body 'control.speed' 'current speed value'

if ! awk '
    /^\[env:STC8H1K08_diag\]/ { in_env = 1; next }
    /^\[env:/ { in_env = 0 }
    in_env && /-DAPP_INPUT_DIAG_DISPLAY=1/ { has_input = 1 }
    in_env && /-DAPP_DISABLE_RADIO=1/ { has_no_radio = 1 }
    END { exit ! (has_input && has_no_radio) }
' "$DIAG_INI"; then
    echo "controller input diag target must enable APP_INPUT_DIAG_DISPLAY and APP_DISABLE_RADIO" >&2
    exit 1
fi
