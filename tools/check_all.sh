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

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/app_outputs_calc_test.c" \
    -o /tmp/app_outputs_calc_test
/tmp/app_outputs_calc_test

cc -std=c99 -Wall -Wextra \
    -DAPP_OUTPUT_MOTOR_MIN_DUTY=15u \
    -DAPP_OUTPUTS_CALC_EXPECTED_MIN_DUTY=15u \
    -DAPP_OUTPUTS_CALC_EXPECTED_DUTY_25=32u \
    -DAPP_OUTPUTS_CALC_EXPECTED_DUTY_39=45u \
    -DAPP_OUTPUTS_CALC_EXPECTED_DUTY_40=46u \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/app_outputs_calc_test.c" \
    -o /tmp/app_outputs_calc_test_min15
/tmp/app_outputs_calc_test_min15

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
    -I"$ROOT_DIR/controller/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/button_event_test.c" \
    -o /tmp/button_event_test
/tmp/button_event_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/controller/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/controller_display_test.c" \
    -o /tmp/controller_display_test
/tmp/controller_display_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/controller/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/controller_radio_config_test.c" \
    -o /tmp/controller_radio_config_test
/tmp/controller_radio_config_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/receiver_radio_config_test.c" \
    -o /tmp/receiver_radio_config_test
/tmp/receiver_radio_config_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/receiver_board_pins_test.c" \
    -o /tmp/receiver_board_pins_test
/tmp/receiver_board_pins_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/receiver_channel_policy_test.c" \
    -o /tmp/receiver_channel_policy_test_fixed
/tmp/receiver_channel_policy_test_fixed

cc -std=c99 -Wall -Wextra \
    -DAPP_RECEIVER_ENABLE_CHANNEL_BUTTONS=1 \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/receiver_channel_policy_test.c" \
    -o /tmp/receiver_channel_policy_test_buttons
/tmp/receiver_channel_policy_test_buttons

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/shared" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/channel_pool_test.c" \
    -o /tmp/channel_pool_test
/tmp/channel_pool_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/controller/src" \
    -I"$ROOT_DIR/shared" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/controller_config_defaults_test.c" \
    -o /tmp/controller_config_defaults_test
/tmp/controller_config_defaults_test

cc -std=c99 -Wall -Wextra \
    -I"$ROOT_DIR/receiver/src" \
    -I"$ROOT_DIR/shared" \
    -I"$ROOT_DIR/../Stc8hBase/core" \
    "$ROOT_DIR/tests/receiver_config_defaults_test.c" \
    -o /tmp/receiver_config_defaults_test
/tmp/receiver_config_defaults_test

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
sh "$ROOT_DIR/tools/check_controller_input_diag_source.sh"
(cd "$ROOT_DIR/receiver" && pio run)
"$ROOT_DIR/tools/check_firmware_size.sh"
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_motor_diag)
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_control_diag)
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_radio_motor_diag)
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_spi_motor_diag)
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_nrfpins_after_outputs_motor_diag)
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_radio_reinit_motor_diag)
(cd "$ROOT_DIR/receiver" && pio run -c platformio_diag.ini -e STC8H1K08_control_gpio_diag)
sh "$ROOT_DIR/tools/check_nrf24_pin_codegen.sh"
