#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

check_one() {
    name=$1
    map_file=$2
    limit=$3
    target=$4

    mem_file=${map_file%.map}.mem
    used=$(python3 - "$mem_file" <<'PY'
import sys

with open(sys.argv[1], "r", encoding="utf-8", errors="ignore") as fh:
    for line in fh:
        if "ROM/EPROM/FLASH" in line:
            print(int(line.split()[3]))
            break
    else:
        raise SystemExit("ROM/EPROM/FLASH line not found")
PY
)
    printf '%s flash: %s/%s target<=%s\n' "$name" "$used" "$limit" "$target"
    if [ "$used" -gt "$limit" ]; then
        echo "$name exceeds hard limit" >&2
        exit 1
    fi
    if [ "$used" -gt "$target" ]; then
        echo "$name exceeds target" >&2
        exit 1
    fi
}

check_one controller "$ROOT_DIR/controller/.pio/build/STC8H1K08/firmware.map" 8192 6453
check_one receiver "$ROOT_DIR/receiver/.pio/build/STC8H1K08/firmware.map" 8192 5363
