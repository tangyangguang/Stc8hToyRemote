# Controller Channel Lifecycle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the confirmed controller/receiver lifecycle: default channel 76, manual scan only from `Lxxx`, `P1..P4` config mode, receiver channel preset pool, and link success based on valid STATUS ACK.

**Architecture:** Keep the wire payload byte-packed in `shared/`. Move display and channel-pool formatting into tiny host-testable helpers. Refactor controller `main.c` into a compact explicit state machine without introducing dynamic allocation or large abstractions, because both firmware images must stay below 8KB.

**Tech Stack:** STC8H C firmware, PlatformIO `intel_mcs51`, `../Stc8hBase` nRF24/TM1637/EEPROM/proto helpers, host C tests compiled by `tools/check_all.sh`.

---

### Task 1: Display And Channel Helpers

**Files:**
- Modify: `controller/src/app_display.h`
- Create: `shared/toy_remote_channels.h`
- Modify: `tests/controller_display_test.c`
- Create: `tests/channel_pool_test.c`
- Modify: `tools/check_all.sh`

- [ ] **Step 1: Write failing display tests**

Add tests that call:

```c
app_display_prefixed_channel_segments(APP_DISPLAY_C, 76u, 0u, segments);
app_display_prefixed_channel_segments(APP_DISPLAY_L, 76u, 0u, segments);
app_display_prefixed_channel_segments(APP_DISPLAY_S, 76u, 0u, segments);
app_display_prefixed_channel_segments(APP_DISPLAY_F, 76u, 0u, segments);
app_display_prefixed_channel_segments(APP_DISPLAY_H, 76u, 0u, segments);
app_display_error_segments(1u, segments);
app_display_config_segments(1u, 1u, segments);
app_display_config_segments(3u, 45u, segments);
```

Expected segment intent:

```c
assert(segments[0] == APP_DISPLAY_C);
assert(segments[1] == app_display_digit(0u));
assert(segments[2] == app_display_digit(7u));
assert(segments[3] == app_display_digit(6u));
assert(segments[0] == APP_DISPLAY_E);
assert(segments[1] == app_display_digit(0u));
assert(segments[2] == app_display_digit(0u));
assert(segments[3] == app_display_digit(1u));
assert(segments[0] == APP_DISPLAY_P);
assert(segments[1] == (app_display_digit(1u) | APP_DISPLAY_COLON));
```

- [ ] **Step 2: Run display test and verify RED**

Run:

```sh
cc -std=c99 -Wall -Wextra -Icontroller/src -I../Stc8hBase/core tests/controller_display_test.c -o /tmp/controller_display_test
```

Expected: compile fails because `APP_DISPLAY_C`, `APP_DISPLAY_P`, `app_display_prefixed_channel_segments`, `app_display_error_segments`, and `app_display_config_segments` do not exist yet.

- [ ] **Step 3: Write failing channel pool tests**

Create `tests/channel_pool_test.c` with:

```c
#include "toy_remote_channels.h"
#include <assert.h>

static void test_default_and_pool_order(void)
{
    assert(TOY_REMOTE_DEFAULT_RF_CHANNEL == 76u);
    assert(toy_remote_channel_pool_value(0u) == 76u);
    assert(toy_remote_channel_pool_value(1u) == 72u);
    assert(toy_remote_channel_pool_value(15u) == 16u);
}

static void test_pool_wraps_from_unknown_channel(void)
{
    assert(toy_remote_channel_pool_next(76u) == 72u);
    assert(toy_remote_channel_pool_prev(76u) == 16u);
    assert(toy_remote_channel_pool_next(40u) == 36u);
    assert(toy_remote_channel_pool_next(41u) == 76u);
    assert(toy_remote_channel_pool_prev(41u) == 76u);
}

int main(void)
{
    test_default_and_pool_order();
    test_pool_wraps_from_unknown_channel();
    return 0;
}
```

- [ ] **Step 4: Run channel pool test and verify RED**

Run:

```sh
cc -std=c99 -Wall -Wextra -Ishared -I../Stc8hBase/core tests/channel_pool_test.c -o /tmp/channel_pool_test
```

Expected: compile fails because `toy_remote_channels.h` does not exist.

- [ ] **Step 5: Implement minimal helpers**

Add `shared/toy_remote_channels.h` as a header-only helper:

```c
#ifndef TOY_REMOTE_CHANNELS_H
#define TOY_REMOTE_CHANNELS_H

#include "stc8h_config.h"

#define TOY_REMOTE_DEFAULT_RF_CHANNEL 76u
#define TOY_REMOTE_CHANNEL_POOL_COUNT 16u

static stc8h_u8 toy_remote_channel_pool_value(stc8h_u8 index)
{
    static const STC8H_CODE stc8h_u8 pool[TOY_REMOTE_CHANNEL_POOL_COUNT] = {
        76u, 72u, 68u, 64u, 60u, 56u, 52u, 48u,
        44u, 40u, 36u, 32u, 28u, 24u, 20u, 16u
    };
    return pool[(index < TOY_REMOTE_CHANNEL_POOL_COUNT) ? index : 0u];
}

static stc8h_u8 toy_remote_channel_pool_index(stc8h_u8 channel)
{
    stc8h_u8 i;
    for (i = 0u; i < TOY_REMOTE_CHANNEL_POOL_COUNT; ++i) {
        if (toy_remote_channel_pool_value(i) == channel) {
            return i;
        }
    }
    return 0xFFu;
}

static stc8h_u8 toy_remote_channel_pool_next(stc8h_u8 channel)
{
    stc8h_u8 index = toy_remote_channel_pool_index(channel);
    if (index == 0xFFu) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    ++index;
    return toy_remote_channel_pool_value((index >= TOY_REMOTE_CHANNEL_POOL_COUNT) ? 0u : index);
}

static stc8h_u8 toy_remote_channel_pool_prev(stc8h_u8 channel)
{
    stc8h_u8 index = toy_remote_channel_pool_index(channel);
    if (index == 0xFFu) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    return toy_remote_channel_pool_value((index == 0u) ? (TOY_REMOTE_CHANNEL_POOL_COUNT - 1u) : (stc8h_u8)(index - 1u));
}

#endif
```

Extend `controller/src/app_display.h` with segment constants and helper functions for `Cxxx/Lxxx/Sxxx/Fxxx/Hxxx/E001/Pn:value`.

- [ ] **Step 6: Run helper tests and verify GREEN**

Run:

```sh
sh tools/check_all.sh
```

Expected: host helper tests pass; firmware builds may still reflect old behavior but must compile.

- [ ] **Step 7: Commit**

```sh
git add controller/src/app_display.h shared/toy_remote_channels.h tests/controller_display_test.c tests/channel_pool_test.c tools/check_all.sh
git commit -m "feat: add channel display helpers"
```

### Task 2: Default Channel And Receiver Preset Buttons

**Files:**
- Modify: `controller/src/app_config.h`
- Modify: `receiver/src/app_config.c`
- Modify: `receiver/src/main.c`
- Test: `tests/channel_pool_test.c`

- [ ] **Step 1: Write failing expectation**

Extend `tests/channel_pool_test.c` to assert all default helpers use `76`; this already fails until firmware defaults include `toy_remote_channels.h`.

- [ ] **Step 2: Implement defaults and receiver P30/P31 pool**

Include `toy_remote_channels.h`. Set controller `APP_DEFAULT_RF_CHANNEL` to `TOY_REMOTE_DEFAULT_RF_CHANNEL`; set receiver `APP_CONFIG_DEFAULT_CHANNEL` to `TOY_REMOTE_DEFAULT_RF_CHANNEL`. In receiver `handle_channel_buttons()`, replace `+1/-1` with `toy_remote_channel_pool_next()` and `toy_remote_channel_pool_prev()`.

- [ ] **Step 3: Verify**

Run:

```sh
sh tools/check_all.sh
```

Expected: tests and builds pass; firmware size targets may need exact update if usage grows.

- [ ] **Step 4: Commit**

```sh
git add controller/src/app_config.h receiver/src/app_config.c receiver/src/main.c tests/channel_pool_test.c
git commit -m "feat: use default channel pool"
```

### Task 3: Receiver Indicator States

**Files:**
- Modify: `receiver/src/app_indicator.h`
- Modify: `receiver/src/app_indicator.c`
- Modify: `receiver/src/main.c`
- Modify: `tests/app_indicator_test.c`

- [ ] **Step 1: Write failing tests**

Add tests for:

```c
app_indicator_set_state(&indicator, APP_INDICATOR_STATE_WAITING_UNBOUND, now);
app_indicator_set_state(&indicator, APP_INDICATOR_STATE_WAITING_BOUND, now);
app_indicator_set_state(&indicator, APP_INDICATOR_STATE_BINDING_CLEARED, now);
```

Expected patterns: unbound single short flash with long pause; bound/link lost slow 500ms blink; binding cleared fast 6 flashes then unbound.

- [ ] **Step 2: Run RED**

Run:

```sh
cc -std=c99 -Wall -Wextra -Ireceiver/src -I../Stc8hBase/core tests/app_indicator_test.c receiver/src/app_indicator.c -o /tmp/app_indicator_test
```

Expected: compile fails because new state names do not exist.

- [ ] **Step 3: Implement states**

Extend indicator enum and update logic. In receiver main, choose `WAITING_UNBOUND` when `bound_tx_id == 0`, `WAITING_BOUND` when bound but idle/lost, `CONNECTED` when valid control arrives, and `BINDING_CLEARED` after P30+P31 boot clear.

- [ ] **Step 4: Verify**

Run:

```sh
sh tools/check_all.sh
```

- [ ] **Step 5: Commit**

```sh
git add receiver/src/app_indicator.h receiver/src/app_indicator.c receiver/src/main.c tests/app_indicator_test.c
git commit -m "feat: refine receiver status LED"
```

### Task 4: Controller Button Event Helper

**Files:**
- Create: `controller/src/app_button.h`
- Create: `controller/src/app_button.c`
- Create: `tests/button_event_test.c`
- Modify: `tools/check_all.sh`

- [ ] **Step 1: Write failing tests**

Create tests that simulate 10ms ticks and assert:

```c
short press -> APP_BUTTON_EVENT_SHORT
two short presses within double-click window -> APP_BUTTON_EVENT_DOUBLE
held for 500 ticks -> APP_BUTTON_EVENT_LONG_5S
held for 300 ticks in config mode -> APP_BUTTON_EVENT_LONG_3S
```

- [ ] **Step 2: Run RED**

Compile fails because `app_button.h` does not exist.

- [ ] **Step 3: Implement helper**

Implement a tiny state machine that consumes active level and threshold ticks. It must not emit short/double when a long event fires.

- [ ] **Step 4: Verify**

Run `sh tools/check_all.sh`.

- [ ] **Step 5: Commit**

```sh
git add controller/src/app_button.h controller/src/app_button.c tests/button_event_test.c tools/check_all.sh
git commit -m "feat: add controller button events"
```

### Task 5: Controller Lifecycle State Machine And Config Mode

**Files:**
- Modify: `controller/src/main.c`
- Modify: `controller/src/app_config.h`
- Modify: `controller/src/app_config.c`
- Test: existing host tests plus firmware build

- [ ] **Step 1: Add tests where pure helpers exist**

Use display and button tests from prior tasks as regression coverage. Do not add hardware-mocked tests for SPI/nRF24 in this pass.

- [ ] **Step 2: Refactor `main.c` into explicit states**

Add compact state values:

```c
APP_STATE_TRY_SAVED
APP_STATE_CONNECTED
APP_STATE_LOST
APP_STATE_SCAN
APP_STATE_CONFIG
APP_STATE_RADIO_ERROR
```

Use valid STATUS ACK (`rx_status.tx_id == config.tx_id`) as the connected signal. Startup no longer calls `scan_channels()`. Repeated invalid/missing STATUS ACK moves to `LOST`. EC11 double event starts scan only from `LOST`. Display `C/L/S/F/E/P` using `app_display.h` helpers.

- [ ] **Step 3: Implement config draft**

Use a `config_draft` copy in config mode. `P1` direction reverse, `P2` steering reverse, `P3` steering middle, `P4` steering reduce. EC11 rotation edits draft. EC11 short switches item. EC11 long 3s saves draft to EEPROM and exits. Configuration mode sends safe control but applies draft steering settings to let servo be observed.

- [ ] **Step 4: Verify**

Run:

```sh
sh tools/check_all.sh
```

Expected: tests and builds pass, flash under hard 8192 limit. Update `tools/check_firmware_size.sh` targets only after checking actual sizes.

- [ ] **Step 5: Commit**

```sh
git add controller/src/main.c controller/src/app_config.h controller/src/app_config.c tools/check_firmware_size.sh
git commit -m "feat: implement controller lifecycle"
```

### Task 6: Protocol Extension For Future Channel Handoff

**Files:**
- Modify: `shared/toy_remote_protocol.h`
- Modify: `shared/toy_remote_protocol.c`
- Modify: `tests/toy_remote_protocol_test.c`
- Modify: `docs/03-protocol.md`

- [ ] **Step 1: Write failing protocol tests**

Add tests for optional channel handoff fields:

```c
control.channel_command == TOY_REMOTE_CHANNEL_COMMAND_NONE
control.channel_value <= 125
status.pending_channel == TOY_REMOTE_CHANNEL_NONE
status.pending_channel <= 125 or == 0xFF
```

- [ ] **Step 2: Decide implementation based on firmware size**

If Task 5 leaves sufficient flash margin, add two bytes to control payload and one byte to status payload. If flash is too tight, keep this task as documentation-only and defer actual handoff to a separate pass.

- [ ] **Step 3: Verify and commit**

Run `sh tools/check_all.sh`; commit protocol changes only if implementation fits.

### Task 7: Final Verification And Documentation Sync

**Files:**
- Modify: `docs/03-protocol.md`, `docs/04-logic-flows.md`, `docs/06-verification.md` only if implementation differs from target.
- Modify: `tools/check_firmware_size.sh` target values after full build.

- [ ] **Step 1: Run full verification**

```sh
sh tools/check_all.sh
git diff --check
git status --short
```

- [ ] **Step 2: Review scope**

Confirm no `legacy/` files changed, no copied `Stc8hBase` source, and all new docs are Chinese except historical plan headers required by skills.

- [ ] **Step 3: Commit final sync**

```sh
git add docs tools
git commit -m "docs: sync lifecycle implementation notes"
```

---

## Self-Review

Spec coverage:
- Default channel 76: Task 2.
- Manual scan only from `Lxxx`: Task 5.
- Valid STATUS ACK as link success: Task 5.
- Config mode `P1..P4`: Task 1 and Task 5.
- Receiver LED states: Task 3.
- Receiver preset channel pool: Task 2.
- Connected channel handoff: Task 6 if firmware size allows; otherwise explicitly deferred.
- One controller controlling multiple receivers: documented as out of scope in Task 7.

Risk:
- Firmware flash is tight. Task 6 is intentionally gated by measured size.
- Controller `main.c` is currently large; avoid broad refactors beyond the state machine required by this feature.
