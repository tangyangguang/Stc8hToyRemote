# nRF24 Fixed Link Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a minimal fixed-address, fixed-channel nRF24L01 two-board communication loop for controller and receiver.

**Architecture:** Keep `Stc8hBase` as the only nRF24/SPI implementation. Add small application-side radio helpers in controller and receiver so main loops can configure the radio, send or receive one 32-byte packet, and report `TX_DONE`, `MAX_RETRY`, and `RX_READY` without copying legacy code.

**Tech Stack:** STC8H1K08, PlatformIO, SDCC, `drv_nrf24l01`, `stc8h_spi`, `proto_rf_link`.

---

## File Structure

- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/main.c`
  - Owns controller boot flow and periodic fixed test packet transmission.
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.h`
  - Declares controller-side radio setup and send helpers.
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.c`
  - Configures nRF24 as PTX and sends one fixed 32-byte packet.
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/main.c`
  - Owns receiver boot flow and fixed test packet receive loop.
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.h`
  - Declares receiver-side radio setup and receive helpers.
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.c`
  - Configures nRF24 as PRX and receives one fixed 32-byte packet.
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/docs/rewrite-plan.md`
  - Record that phase 2 fixed-link implementation is present and what remains hardware-only.

Do not modify anything under `/Users/tyg/dir/codex_dir/Stc8hToyRemote/legacy`.

## Constants

Use these fixed values in both controller and receiver helpers:

```c
#define APP_RADIO_PACKET_SIZE 32u
#define APP_RADIO_CHANNEL 40u
#define APP_RADIO_ADDR_LEN 5u
static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};
```

Use `DRV_NRF24L01_RATE_1MBPS` and `DRV_NRF24L01_POWER_0DBM` for the first fixed-link build. Lower data rate can be tested later on hardware if range is insufficient.

## Task 1: Controller Radio Helper

**Files:**
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.h`
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/main.c`

- [ ] **Step 1: Write the controller radio header**

Create `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.h`:

```c
#ifndef CONTROLLER_APP_RADIO_H
#define CONTROLLER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u

typedef enum {
    APP_RADIO_TX_IDLE = 0,
    APP_RADIO_TX_DONE,
    APP_RADIO_TX_MAX_RETRY,
    APP_RADIO_TX_ERROR
} app_radio_tx_result_t;

stc8h_status_t app_radio_init_tx(void);
app_radio_tx_result_t app_radio_send_packet(const stc8h_u8 *packet, stc8h_u8 len);

#endif
```

- [ ] **Step 2: Implement controller radio configuration**

Create `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.c`:

```c
#include "app_radio.h"
#include "drv_nrf24l01.h"

#define APP_RADIO_CHANNEL 40u
#define APP_RADIO_ADDR_LEN 5u

static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};

stc8h_status_t app_radio_init_tx(void)
{
    drv_nrf24l01_init_pins();
    drv_nrf24l01_power_down();
    drv_nrf24l01_flush_tx();
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(0x70u);

    if (drv_nrf24l01_check_present() != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_channel(APP_RADIO_CHANNEL) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_address_width(APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_tx_address(app_radio_addr, APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rx_address(0u, app_radio_addr, APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_payload_size(0u, APP_RADIO_PACKET_SIZE) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_set_auto_ack(DRV_NRF24L01_PIPE0);
    if (drv_nrf24l01_set_auto_retransmit(3u, 10u) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rate_power(DRV_NRF24L01_RATE_1MBPS, DRV_NRF24L01_POWER_0DBM) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_enter_tx();
    return STC8H_OK;
}
```

- [ ] **Step 3: Implement controller packet send**

Append to `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/app_radio.c`:

```c
app_radio_tx_result_t app_radio_send_packet(const stc8h_u8 *packet, stc8h_u8 len)
{
    stc8h_u8 status;
    stc8h_u16 wait;

    if ((packet == 0) || (len != APP_RADIO_PACKET_SIZE)) {
        return APP_RADIO_TX_ERROR;
    }

    drv_nrf24l01_flush_tx();
    (void)drv_nrf24l01_write_payload(packet, len);
    drv_nrf24l01_pulse_ce();

    for (wait = 0u; wait < 60000u; ++wait) {
        status = drv_nrf24l01_read_status();
        if ((status & DRV_NRF24L01_STATUS_TX_DONE) != 0u) {
            drv_nrf24l01_clear_irq(status);
            return APP_RADIO_TX_DONE;
        }
        if ((status & DRV_NRF24L01_STATUS_MAX_RETRY) != 0u) {
            drv_nrf24l01_flush_tx();
            drv_nrf24l01_clear_irq(status);
            return APP_RADIO_TX_MAX_RETRY;
        }
    }

    status = drv_nrf24l01_read_status();
    drv_nrf24l01_flush_tx();
    drv_nrf24l01_clear_irq(status);
    return APP_RADIO_TX_ERROR;
}
```

- [ ] **Step 4: Wire controller main to send a fixed packet**

Replace `/Users/tyg/dir/codex_dir/Stc8hToyRemote/controller/src/main.c` with:

```c
#include "app_radio.h"
#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static STC8H_XDATA stc8h_u8 packet[APP_RADIO_PACKET_SIZE];
static stc8h_u8 seq;
static app_radio_tx_result_t last_tx_result;

static void make_fixed_packet(void)
{
    stc8h_u8 i;

    for (i = 0u; i < APP_RADIO_PACKET_SIZE; ++i) {
        packet[i] = 0u;
    }

    packet[0] = 0xA5u;
    packet[1] = 0x01u;
    packet[2] = seq;
    packet[3] = (stc8h_u8)last_tx_result;
    ++seq;
}

void main(void)
{
    stc8h_spi_init();
    last_tx_result = APP_RADIO_TX_IDLE;

    if (app_radio_init_tx() != STC8H_OK) {
        last_tx_result = APP_RADIO_TX_ERROR;
    }

    while (1) {
        make_fixed_packet();
        if (last_tx_result != APP_RADIO_TX_ERROR) {
            last_tx_result = app_radio_send_packet(packet, APP_RADIO_PACKET_SIZE);
        }
    }
}
```

- [ ] **Step 5: Build controller**

Run:

```sh
pio run
```

Expected: controller build succeeds. Record flash usage from PlatformIO output.

- [ ] **Step 6: Commit controller fixed TX helper**

Run:

```sh
git add controller/src/main.c controller/src/app_radio.h controller/src/app_radio.c
git commit -m "Add controller fixed nRF24 transmitter"
```

## Task 2: Receiver Radio Helper

**Files:**
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.h`
- Create: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.c`
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/main.c`

- [ ] **Step 1: Write the receiver radio header**

Create `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.h`:

```c
#ifndef RECEIVER_APP_RADIO_H
#define RECEIVER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u

typedef enum {
    APP_RADIO_RX_NONE = 0,
    APP_RADIO_RX_PACKET,
    APP_RADIO_RX_ERROR
} app_radio_rx_result_t;

stc8h_status_t app_radio_init_rx(void);
app_radio_rx_result_t app_radio_receive_packet(stc8h_u8 *packet, stc8h_u8 len);

#endif
```

- [ ] **Step 2: Implement receiver radio configuration**

Create `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.c`:

```c
#include "app_radio.h"
#include "drv_nrf24l01.h"

#define APP_RADIO_CHANNEL 40u
#define APP_RADIO_ADDR_LEN 5u

static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};

stc8h_status_t app_radio_init_rx(void)
{
    drv_nrf24l01_init_pins();
    drv_nrf24l01_power_down();
    drv_nrf24l01_flush_tx();
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(0x70u);

    if (drv_nrf24l01_check_present() != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_channel(APP_RADIO_CHANNEL) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_address_width(APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rx_address(0u, app_radio_addr, APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_payload_size(0u, APP_RADIO_PACKET_SIZE) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_set_auto_ack(DRV_NRF24L01_PIPE0);
    if (drv_nrf24l01_set_auto_retransmit(3u, 10u) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rate_power(DRV_NRF24L01_RATE_1MBPS, DRV_NRF24L01_POWER_0DBM) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_enter_rx();
    return STC8H_OK;
}
```

- [ ] **Step 3: Implement receiver packet receive**

Append to `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/app_radio.c`:

```c
app_radio_rx_result_t app_radio_receive_packet(stc8h_u8 *packet, stc8h_u8 len)
{
    stc8h_u8 status;

    if ((packet == 0) || (len != APP_RADIO_PACKET_SIZE)) {
        return APP_RADIO_RX_ERROR;
    }

    status = drv_nrf24l01_read_status();
    if ((status & DRV_NRF24L01_STATUS_RX_READY) == 0u) {
        return APP_RADIO_RX_NONE;
    }

    (void)drv_nrf24l01_read_payload(packet, len);
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(status);
    return APP_RADIO_RX_PACKET;
}
```

- [ ] **Step 4: Wire receiver main to receive fixed packets**

Replace `/Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver/src/main.c` with:

```c
#include "app_radio.h"
#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static STC8H_XDATA stc8h_u8 packet[APP_RADIO_PACKET_SIZE];
static stc8h_u8 last_seq;
static stc8h_u8 packet_count;
static stc8h_u8 radio_error;

static void handle_packet(void)
{
    if ((packet[0] == 0xA5u) && (packet[1] == 0x01u)) {
        last_seq = packet[2];
        ++packet_count;
    }
}

void main(void)
{
    stc8h_spi_init();

    if (app_radio_init_rx() != STC8H_OK) {
        radio_error = 1u;
    }

    while (1) {
        if (radio_error == 0u) {
            if (app_radio_receive_packet(packet, APP_RADIO_PACKET_SIZE) == APP_RADIO_RX_PACKET) {
                handle_packet();
            }
        }
    }
}
```

- [ ] **Step 5: Build receiver**

Run:

```sh
pio run
```

Expected: receiver build succeeds. Record flash usage from PlatformIO output.

- [ ] **Step 6: Commit receiver fixed RX helper**

Run:

```sh
git add receiver/src/main.c receiver/src/app_radio.h receiver/src/app_radio.c
git commit -m "Add receiver fixed nRF24 listener"
```

## Task 3: Documentation and Phase 2 Verification

**Files:**
- Modify: `/Users/tyg/dir/codex_dir/Stc8hToyRemote/docs/rewrite-plan.md`

- [ ] **Step 1: Update phase 2 notes**

Add a short status note under phase 2 in `/Users/tyg/dir/codex_dir/Stc8hToyRemote/docs/rewrite-plan.md`:

```markdown
阶段 2 first build:

- 固定地址：`TOYR1`
- 固定频道：40
- 固定 payload：32 bytes
- controller 已提供 PTX 发送 loop。
- receiver 已提供 PRX 接收 loop。
- 硬件待验证：真实双板 `TX_DONE`、`MAX_RETRY`、`RX_READY`、断电恢复和断联功耗。
```

- [ ] **Step 2: Build both projects**

Run:

```sh
(cd controller && pio run)
(cd receiver && pio run)
```

Expected: both builds succeed. Record flash usage.

- [ ] **Step 3: Check git status**

Run:

```sh
git status --short
```

Expected: only `/Users/tyg/dir/codex_dir/Stc8hToyRemote/docs/rewrite-plan.md` is modified.

- [ ] **Step 4: Commit documentation**

Run:

```sh
git add docs/rewrite-plan.md
git commit -m "Document fixed nRF24 link phase"
```

## Self-Review

- Spec coverage: This plan covers phase 2 only: fixed address, fixed channel, fixed 32-byte payload, basic TX/RX status handling, no business IO, no persistence.
- Intentional gaps: It does not implement `proto_rf_link` packets, receiver safe-state logic, business input/output migration, channel scanning, binding, or low-power sleep. Those belong to later phases.
- Placeholder scan: No `TBD` or `TODO` placeholders are used.
- Type consistency: `APP_RADIO_PACKET_SIZE`, `app_radio_init_tx`, `app_radio_init_rx`, `app_radio_send_packet`, and `app_radio_receive_packet` are consistently named across declarations and use sites.
