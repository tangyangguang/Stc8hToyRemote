#include "app_steering.h"

#include <assert.h>

#ifndef APP_STEERING_STEP_EXPECTED_DEFAULT
#define APP_STEERING_STEP_EXPECTED_DEFAULT 1u
#endif

#ifndef APP_STEERING_STEP_EXPECTED_SMALL_MOVE
#define APP_STEERING_STEP_EXPECTED_SMALL_MOVE 91u
#endif

#ifndef APP_STEERING_STEP_EXPECTED_EDGE_MOVE
#define APP_STEERING_STEP_EXPECTED_EDGE_MOVE 92u
#endif

#ifndef APP_STEERING_STEP_EXPECTED_LARGE_MOVE
#define APP_STEERING_STEP_EXPECTED_LARGE_MOVE 93u
#endif

int main(void)
{
    assert(APP_STEERING_MIN_STEP_DEGREES == APP_STEERING_STEP_EXPECTED_DEFAULT);
    assert(app_steering_apply_min_step(90u, 90u) == 90u);
    assert(app_steering_apply_min_step(90u, 91u) == APP_STEERING_STEP_EXPECTED_SMALL_MOVE);
    assert(app_steering_apply_min_step(90u, 92u) == APP_STEERING_STEP_EXPECTED_EDGE_MOVE);
    assert(app_steering_apply_min_step(90u, 93u) == APP_STEERING_STEP_EXPECTED_LARGE_MOVE);
    assert(app_steering_apply_config(90u, 0u, 0, 0u, 0u) == 90u);
    return 0;
}
