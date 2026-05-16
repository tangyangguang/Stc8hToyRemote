# 重要逻辑流程

## controller 启动流程

```text
BOOT
  -> init board pins
  -> init SPI
  -> check nRF24L01
  -> configure fixed radio params
  -> CONNECTING
```

阶段 2 当前流程：

```text
init SPI
  -> app_radio_init_tx()
  -> loop:
       make fixed packet
       send packet
       record TX result
```

阶段 3 目标流程：

```text
CONNECTING
  -> send HELLO periodically
  -> receive status or ACK evidence
  -> CONNECTED
  -> sample inputs by schedule
  -> pack control payload
  -> send DATA with latest state
  -> LINK_LOST on repeated send failure or timeout
```

## controller 正常模式线索

Tx V2.x 记录中正常模式包含方向、速度、刹车、转向、灯光、喇叭、辅助 PWM 和电压显示。新项目保留这些作为需求线索，但最终交互按新需求评估实现：

- 方向是持续状态，不做旧按钮切换兼容。
- 速度是 `0..100` 的持续状态。
- 刹车语义需要区分“清速度”和“不清速度”两类输入。
- 显示只要求状态可观察，不复刻旧段码编码。

## controller 配置模式线索

Tx V2.x 记录中配置模式涉及舵机反向、中值、减少角度、方向反转和 EEPROM 保存。新项目暂缓实现配置模式；实施前必须先设计持久化格式、校验、默认值和恢复路径。

## receiver 启动流程

```text
BOOT
  -> init board pins
  -> init output safe levels
  -> init SPI
  -> check nRF24L01
  -> configure fixed radio params
  -> WAIT_CONTROLLER
```

阶段 2 当前流程：

```text
init SPI
  -> app_radio_init_rx()
  -> loop:
       poll radio status
       read fixed packet when RX_READY
       record seq/count
```

阶段 3 目标流程：

```text
WAIT_CONTROLLER
  -> receive HELLO
  -> send STATUS or HELLO_ACK
  -> CONNECTED
  -> receive DATA
  -> validate payload
  -> apply outputs
  -> refresh last_packet_ms
  -> SAFE_STATE on timeout
```

## receiver 安全状态流程

触发条件：

- 业务包超时。
- payload 版本或字段非法并持续无法恢复。
- radio 明确失败且超过恢复窗口。

动作：

```text
set motor speed 0
turn light off
turn buzzer off
turn aux PWM off
keep servo at last valid angle by default
lower retry/status activity
continue low-frequency listening for recovery
```

舵机是否回中位需要硬件确认；默认保持最近有效角度，避免断联瞬间突然打角造成机械风险。

## 无线失败处理流程

controller：

```text
send packet
  -> TX_DONE: clear IRQ, remain connected
  -> MAX_RETRY: flush TX, clear IRQ, count failure
  -> repeated failure: LINK_LOST, reduce retry rate
```

receiver：

```text
RX_READY
  -> read payload
  -> flush RX
  -> clear IRQ
  -> validate and apply
```

不在中断里做 SPI 读写。IRQ 只适合作为“有事件”的提示，实际处理放在主循环。
