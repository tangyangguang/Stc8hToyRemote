#include "app_steering.h"
#include "app_config.h"
#include "toy_remote_protocol.h"

#include <assert.h>

static void expect_angle(stc8h_u8 raw, stc8h_u8 flags, stc8h_u8 deadband, stc8h_u8 reduce, stc8h_u8 expected)
{
    assert(app_steering_apply_config(raw, flags, deadband, reduce) == expected);
}

int main(void)
{
    assert(APP_CONFIG_STEERING_DEADBAND_MIN == 10u);

    expect_angle(0u, 0u, APP_CONFIG_DEFAULT_STEERING_DEADBAND, APP_CONFIG_DEFAULT_STEERING_REDUCE, 20u);
    expect_angle(80u, 0u, APP_CONFIG_DEFAULT_STEERING_DEADBAND, APP_CONFIG_DEFAULT_STEERING_REDUCE, 90u);
    expect_angle(90u, 0u, APP_CONFIG_DEFAULT_STEERING_DEADBAND, APP_CONFIG_DEFAULT_STEERING_REDUCE, 90u);
    expect_angle(100u, 0u, APP_CONFIG_DEFAULT_STEERING_DEADBAND, APP_CONFIG_DEFAULT_STEERING_REDUCE, 90u);
    expect_angle(180u, 0u, APP_CONFIG_DEFAULT_STEERING_DEADBAND, APP_CONFIG_DEFAULT_STEERING_REDUCE, 160u);

    expect_angle(0u, 0u, 10u, 30u, 30u);
    expect_angle(90u, 0u, 10u, 30u, 90u);
    expect_angle(180u, 0u, 10u, 30u, 150u);

    expect_angle(60u, 0u, 30u, 20u, 90u);
    expect_angle(90u, 0u, 30u, 20u, 90u);
    expect_angle(120u, 0u, 30u, 20u, 90u);

    expect_angle(0u, APP_CONFIG_FLAG_STEERING_REVERSE, 10u, 20u, 160u);
    expect_angle(90u, APP_CONFIG_FLAG_STEERING_REVERSE, 10u, 20u, 90u);
    expect_angle(180u, APP_CONFIG_FLAG_STEERING_REVERSE, 10u, 20u, 20u);

    return 0;
}
