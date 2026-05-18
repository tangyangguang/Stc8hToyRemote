#include <assert.h>

#ifndef TEST_STEPS
#define TEST_STEPS 4
#endif

#define DRV_EC11_ENABLE_FULL_API 0
#define DRV_EC11_ENABLE_SMALL_API 1
#define DRV_EC11_SMALL_STEPS_PER_DETENT TEST_STEPS
#define DRV_EC11_SMALL_REVERSE 0
#define DRV_EC11_ENABLE_NULL_CHECK 0

#include "drv_ec11.h"
#include "../../Stc8hBase/drivers/drv_ec11.c"

static stc8h_s8 scan_sequence(drv_ec11_small_t *ec11, const stc8h_u8 *states, stc8h_u8 count)
{
    stc8h_u8 i;
    stc8h_s8 total;

    total = 0;
    for (i = 0u; i < count; ++i) {
        total = (stc8h_s8)(total + drv_ec11_scan_delta_small(ec11,
            (states[i] & 0x02u) ? 1u : 0u,
            (states[i] & 0x01u) ? 1u : 0u));
    }
    return total;
}

static stc8h_s8 run_left_full_detent(void)
{
    drv_ec11_small_t ec11;
    static const stc8h_u8 states[] = {0x03u, 0x02u, 0x00u, 0x01u, 0x03u};

    drv_ec11_small_init(&ec11);
    return scan_sequence(&ec11, states, (stc8h_u8)(sizeof(states) / sizeof(states[0])));
}

static stc8h_s8 run_right_full_detent(void)
{
    drv_ec11_small_t ec11;
    static const stc8h_u8 states[] = {0x03u, 0x01u, 0x00u, 0x02u, 0x03u};

    drv_ec11_small_init(&ec11);
    return scan_sequence(&ec11, states, (stc8h_u8)(sizeof(states) / sizeof(states[0])));
}

int main(void)
{
    stc8h_s8 left;
    stc8h_s8 right;

    left = run_left_full_detent();
    right = run_right_full_detent();

#if TEST_STEPS == 4
    assert(left != 0);
    assert(right != 0);
    assert(left == (stc8h_s8)(0 - right));
#elif TEST_STEPS == 2
    assert(left != 0);
    assert(right != 0);
    assert(left == (stc8h_s8)(0 - right));
    assert((left == 1) || (left == -1));
#else
#error "Unexpected TEST_STEPS"
#endif

    return 0;
}
