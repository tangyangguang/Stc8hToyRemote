#include <assert.h>

#include "../controller/src/app_ec11_speed.h"

int main(void)
{
    assert(app_ec11_speed_scale_delta(1, 100u, 0u) == 1);
    assert(app_ec11_speed_scale_delta(-1, 100u, 0u) == -1);

    assert(app_ec11_speed_scale_delta(1, 100u, 1u) == 1);
    assert(app_ec11_speed_scale_delta(-1, 100u, 1u) == -1);

    assert(app_ec11_speed_scale_delta(1, 30u, 1u) == 5);
    assert(app_ec11_speed_scale_delta(-1, 30u, 1u) == -5);

    assert(app_ec11_speed_scale_delta(1, 12u, 1u) == 10);
    assert(app_ec11_speed_scale_delta(-1, 12u, 1u) == -10);

    assert(app_ec11_speed_scale_delta(1, 6u, 1u) == 20);
    assert(app_ec11_speed_scale_delta(-1, 6u, 1u) == -20);

    return 0;
}
