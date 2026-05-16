# Firmware Size Optimization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep one firmware feature set matching the old project behavior while reducing controller and receiver flash usage enough for STC8H1K08 with meaningful headroom.

**Architecture:** Optimize by replacing general-purpose runtime paths with fixed ToyRemote paths while preserving behavior. Measure every task with `./tools/check_all.sh`, keep the single controller/receiver build, and do not create small/full firmware variants.

**Tech Stack:** STC8H1K08, PlatformIO Intel MCS-51, SDCC, `../Stc8hBase`, `drv_nrf24l01`, `proto_rf_link`, fixed-block EEPROM.

---

## Current Size Baseline

Baseline from commit `dfc293b`:

| Target | Current | Limit | Headroom |
| --- | ---: | ---: | ---: |
| controller | `8164` | `8192` | `28` |
| receiver | `7791` | `8192` | `401` |

Old V24 Keil build logs:

| Target | Old code+const |
| --- | ---: |
| Tx | `5913` |
| Rx | `4543` |

Practical target for this project without removing user-visible functionality:

| Target | First milestone | Preferred milestone |
| --- | ---: | ---: |
| controller | `<7600` | `<7200` |
| receiver | `<7200` | `<6800` |

## Optimization Rules

- Keep one version only; no `small`/`full` split.
- Keep old-project-visible behavior: control inputs, motor/servo/light/buzzer, ACK status, voltage display, channel scan, receiver binding, timeout safety.
- Keep `legacy/` read-only.
- Do not copy base-library source into application logic.
- Do not weaken receiver timeout safety.
- Do not direct-send C structs; wireless payloads remain byte-packed.
- If a required reduction clearly belongs in `Stc8hBase`, stop and report a prompt to the user instead of adding an application-side workaround.

## Task 1: Add Size Guardrails

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/tools/check_all.sh`
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/tools/check_firmware_size.sh`

- [ ] **Step 1: Add a size checker script**

Create `tools/check_firmware_size.sh`:

```sh
#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

check_one() {
    name=$1
    map_file=$2
    limit=$3
    target=$4

    used=$(python3 - "$map_file" <<'PY'
import re
import sys

cseg = 0
const = 0
with open(sys.argv[1], "r", encoding="utf-8", errors="ignore") as fh:
    for line in fh:
        parts = line.split()
        if len(parts) >= 3 and parts[0] == "CSEG":
            cseg = int(parts[2], 16)
        elif len(parts) >= 3 and parts[0] == "CONST":
            const = int(parts[2], 16)
print(cseg + const)
PY
)
    printf '%s flash: %s/%s target<=%s\n' "$name" "$used" "$limit" "$target"
    if [ "$used" -gt "$limit" ]; then
        echo "$name exceeds hard limit" >&2
        exit 1
    fi
}

check_one controller "$ROOT_DIR/controller/.pio/build/STC8H1K08/firmware.map" 8192 7600
check_one receiver "$ROOT_DIR/receiver/.pio/build/STC8H1K08/firmware.map" 8192 7200
```

- [ ] **Step 2: Wire the checker into full validation**

Append this line to `tools/check_all.sh` after both `pio run` commands:

```sh
"$ROOT_DIR/tools/check_firmware_size.sh"
```

- [ ] **Step 3: Run validation and capture baseline**

Run:

```sh
./tools/check_all.sh
```

Expected at this step: firmware builds pass, size checker reports current flash numbers. If the target thresholds fail before optimization, keep the checker but temporarily set target values to current measured values so Tasks 2-6 can ratchet them down.

- [ ] **Step 4: Commit**

```sh
git add tools/check_all.sh tools/check_firmware_size.sh
git commit -m "Add firmware size guardrails"
```

## Task 2: Remove Generic Protocol Helpers From Firmware Hot Paths

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/main.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/main.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/shared/toy_remote_protocol.h`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/tests/toy_remote_protocol_test.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/tests/rf_link_integration_test.c`

- [ ] **Step 1: Add fixed payload helpers as macros or static inline code**

In `shared/toy_remote_protocol.h`, add fixed byte writers/readers for fields already used by both firmware and tests:

```c
#define TOY_REMOTE_GET_U16_LE(buf, off) \
    ((stc8h_u16)((stc8h_u16)(buf)[(off)] | ((stc8h_u16)(buf)[(off) + 1u] << 8)))

#define TOY_REMOTE_PUT_U16_LE(buf, off, value) do { \
    (buf)[(off)] = (stc8h_u8)(value); \
    (buf)[(off) + 1u] = (stc8h_u8)((value) >> 8); \
} while (0)
```

- [ ] **Step 2: Pack controller control payload in place**

In `controller/src/main.c`, replace the call to `toy_remote_pack_control()` inside `make_control_packet()` with direct byte writes. Keep the same field order and ranges as `TOY_REMOTE_CONTROL_OFFSET_*`.

- [ ] **Step 3: Unpack receiver control payload in place**

In `receiver/src/main.c`, replace `toy_remote_unpack_control()` in the radio receive path with direct byte reads plus only the checks receiver needs:

```c
payload_len == TOY_REMOTE_CONTROL_PAYLOAD_SIZE
payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] == TOY_REMOTE_PROTOCOL_VERSION
direction <= TOY_REMOTE_DIRECTION_REVERSE
speed <= TOY_REMOTE_CONTROL_SPEED_MAX
brake <= 1
steering_angle <= TOY_REMOTE_STEERING_MAX
light <= 1
buzzer <= 1
aux_pwm <= TOY_REMOTE_CONTROL_AUX_PWM_MAX
request_voltage <= 1
tx_id != 0
```

- [ ] **Step 4: Keep host tests on shared protocol functions**

Do not delete shared pack/unpack tests. They protect protocol format even if firmware uses fixed in-place paths.

- [ ] **Step 5: Run validation and record size delta**

Run:

```sh
./tools/check_all.sh
```

Expected: both builds pass; controller should no longer link `toy_remote_pack_control`; receiver should no longer link `toy_remote_unpack_control`.

- [ ] **Step 6: Commit**

```sh
git add controller/src/main.c receiver/src/main.c shared/toy_remote_protocol.h tests/toy_remote_protocol_test.c tests/rf_link_integration_test.c
git commit -m "Use fixed ToyRemote payload paths"
```

## Task 3: Collapse ACK Status Build and Parse

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/main.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/main.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_status.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_status.h`

- [ ] **Step 1: Build ACK payload directly in receiver**

Move fixed ACK status byte generation into `receiver/src/main.c::prepare_ack_status()`. Keep the status payload exactly:

```text
byte0 protocol version
byte1 link_state
byte2 voltage_int
byte3 voltage_dec
byte4 tx_id low
byte5 tx_id high
```

- [ ] **Step 2: Keep voltage sampling as the only app_status responsibility**

If `app_status_update()` remains useful only for voltage sampling, rename or reduce it to a smaller helper that updates `status.voltage_int` and `status.voltage_dec` when requested. If reducing does not save measurable ROM, keep the existing file but avoid pulling `toy_remote_validate_status()`.

- [ ] **Step 3: Confirm unused status helpers are not linked**

Run:

```sh
./tools/check_all.sh
rg -n "toy_remote_validate_status|toy_remote_status_set_voltage" receiver/.pio/build/STC8H1K08/firmware.map
```

Expected: build passes. If status voltage conversion remains linked, inspect whether direct centivolt split in `app_status` is smaller.

- [ ] **Step 4: Commit**

```sh
git add controller/src/main.c receiver/src/main.c receiver/src/app_status.c receiver/src/app_status.h
git commit -m "Inline fixed ACK status handling"
```

## Task 4: Remove Remaining 32-bit Arithmetic From Receiver

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_outputs.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_status.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/tests/toy_remote_protocol_test.c`

- [ ] **Step 1: Locate 32-bit library pulls**

Run:

```sh
rg -n "__divulong|__mullong" receiver/.pio/build/STC8H1K08/firmware.map
```

Expected baseline currently includes both symbols.

- [ ] **Step 2: Replace servo and motor math with 16-bit fixed formulas**

Keep observable output ranges the same. Use 16-bit formulas and clamp explicitly before calling PWM duty APIs.

- [ ] **Step 3: Replace receiver voltage conversion with 16-bit bounded formula**

Use the controller-side style: average ADC into 16-bit, avoid 32-bit constants, clamp invalid or very small ADC values to display maximum.

- [ ] **Step 4: Run validation and confirm no long math**

Run:

```sh
./tools/check_all.sh
rg -n "__divulong|__mullong" receiver/.pio/build/STC8H1K08/firmware.map
```

Expected: build passes and no receiver long arithmetic symbols remain.

- [ ] **Step 5: Commit**

```sh
git add receiver/src/app_outputs.c receiver/src/app_status.c tests/toy_remote_protocol_test.c
git commit -m "Remove receiver long arithmetic"
```

## Task 5: Reduce Cross-File Application Boundaries

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.h`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/main.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.h`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/main.c`

- [ ] **Step 1: Move one-call getters into headers**

Replace tiny cross-file getters such as `app_radio_get_ack_len()` and `app_radio_get_ack_packet()` with direct state ownership in `main.c` or header-level static inline accessors only if the map shows a flash reduction.

- [ ] **Step 2: Combine radio send result handling**

Keep behavior identical, but reduce one-layer wrappers around send/read ACK if the wrapper only forwards to `drv_nrf24l01` and adds no reusable policy.

- [ ] **Step 3: Run validation and inspect DSEG**

Run:

```sh
./tools/check_all.sh
rg -n "^DSEG|^OSEG|_PARM" controller/.pio/build/STC8H1K08/firmware.map receiver/.pio/build/STC8H1K08/firmware.map
```

Expected: no DSEG/OSEG link failures; cross-file parameter pressure should not increase.

- [ ] **Step 4: Commit**

```sh
git add controller/src/app_radio.c controller/src/app_radio.h controller/src/main.c receiver/src/app_radio.c receiver/src/app_radio.h receiver/src/main.c
git commit -m "Tighten radio application boundaries"
```

## Task 6: Ratchet Size Targets and Update Docs

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/tools/check_firmware_size.sh`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/docs/06-verification.md`

- [ ] **Step 1: Set guardrail targets to achieved values plus margin**

After Tasks 2-5, edit `tools/check_firmware_size.sh` so target values reflect the achieved first milestone. Do not set targets higher than:

```text
controller target <= 7600
receiver target <= 7200
```

- [ ] **Step 2: Update verification documentation**

Append the final optimization result to `docs/06-verification.md` with:

```text
controller flash:
receiver flash:
remaining headroom:
features retained:
```

- [ ] **Step 3: Run final validation**

Run:

```sh
./tools/check_all.sh
git diff --check
git status --short
```

Expected: validation passes and only intended files are modified.

- [ ] **Step 4: Commit**

```sh
git add tools/check_firmware_size.sh docs/06-verification.md
git commit -m "Document firmware size optimization results"
```

## Stop Conditions

Stop and report to the user before continuing if any of these occur:

- A task would remove user-visible old-project behavior.
- A task requires copying base-library implementation into this project.
- A task needs a new base-library fixed path to avoid application-side workaround.
- A task reduces flash but breaks receiver timeout safety.
- Controller remains above `7800` bytes after Task 3, because additional application-only reductions are unlikely to produce enough headroom.
