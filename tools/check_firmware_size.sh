#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

(cd "$ROOT_DIR/controller" && pio run >/dev/null)
(cd "$ROOT_DIR/receiver" && pio run >/dev/null)

check_one() {
    name=$1
    map_file=$2
    limit=$3
    target=$4
    min_stack=$5
    min_spare_internal_ram=$6

    mem_file=${map_file%.map}.mem
    python3 "$ROOT_DIR/tools/check_firmware_size.py" \
        "$name" "$mem_file" "$limit" "$target" "$min_stack" "$min_spare_internal_ram"
}

check_one controller "$ROOT_DIR/controller/.pio/build/STC8H1K08/firmware.map" 8192 7800 135 0
check_one receiver "$ROOT_DIR/receiver/.pio/build/STC8H1K08/firmware.map" 8192 6904 145 2
