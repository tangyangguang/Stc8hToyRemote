#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/shared" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/toy_remote_protocol_test.c" \
    "$ROOT_DIR/shared/toy_remote_protocol.c" \
    -o /tmp/toy_remote_protocol_test
/tmp/toy_remote_protocol_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/app_indicator_test.c" \
    "$ROOT_DIR/receiver/src/app_indicator.c" \
    -o /tmp/app_indicator_test
/tmp/app_indicator_test

cc -std=c99 -Wall -Wextra -Wno-duplicate-decl-specifier \
    -DTEST_STEPS=4 \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    -I"$ROOT_DIR/../Stc8hBase/drivers" \
    "$ROOT_DIR/tests/ec11_small_sequence_test.c" \
    -o /tmp/ec11_small_sequence_test_4
/tmp/ec11_small_sequence_test_4

cc -std=c99 -Wall -Wextra -Wno-duplicate-decl-specifier \
    -DTEST_STEPS=2 \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    -I"$ROOT_DIR/../Stc8hBase/drivers" \
    "$ROOT_DIR/tests/ec11_small_sequence_test.c" \
    -o /tmp/ec11_small_sequence_test_2
/tmp/ec11_small_sequence_test_2

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/controller/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/ec11_speed_accel_test.c" \
    -o /tmp/ec11_speed_accel_test
/tmp/ec11_speed_accel_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/shared" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    -I"$ROOT_DIR/../Stc8hBase/protocols" \
    "$ROOT_DIR/tests/rf_link_integration_test.c" \
    "$ROOT_DIR/shared/toy_remote_protocol.c" \
    "$ROOT_DIR/../Stc8hBase/protocols/proto_rf_link.c" \
    -o /tmp/rf_link_integration_test
/tmp/rf_link_integration_test

(cd "$ROOT_DIR/controller" && pio run)
sh "$ROOT_DIR/tools/check_controller_ec11_isr_codegen.sh"
(cd "$ROOT_DIR/receiver" && pio run)
"$ROOT_DIR/tools/check_firmware_size.sh"
