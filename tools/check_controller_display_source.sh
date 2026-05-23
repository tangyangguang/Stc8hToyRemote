#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

if awk '
    /static void display_commit_raw4\(void\)/ { in_fn = 1 }
    in_fn && /EA = 0/ { disables_irq = 1 }
    in_fn && /drv_tm1637_display_raw4/ { calls_tm1637 = 1 }
    in_fn && /^}/ {
        if (disables_irq && calls_tm1637) {
            exit 1
        }
        in_fn = 0
        disables_irq = 0
        calls_tm1637 = 0
    }
' "$ROOT_DIR/controller/src/main.c"; then
    exit 0
fi

echo "controller display_commit_raw4 must not mask Timer0 while bit-banging TM1637" >&2
exit 1
