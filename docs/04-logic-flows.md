# 重要逻辑流程

## controller 启动流程

```text
init SPI
  -> init input and display
  -> init nRF24 PTX on fixed address/channel
  -> loop:
       sample inputs
       pack control payload
       make proto_rf_link DATA packet
       send packet every about 50ms
       read ACK payload status when present
       refresh TM1637 display
```

## controller 正常模式线索

Tx V2.x 记录中正常模式包含方向、速度、刹车、转向、灯光、喇叭、辅助 PWM 和电压显示。新项目保留这些作为需求线索，但最终交互按新需求评估实现：

- 方向是持续状态，不做旧按钮切换兼容。
- 速度是 `0..100` 的持续状态；EC11 增减量先通过 `toy_remote_control_adjust_speed()` 限幅。
- 刹车语义在 controller 侧归约：
  - `TOY_REMOTE_BRAKE_RELEASE`：松开刹车，不改变速度。
  - `TOY_REMOTE_BRAKE_HOLD_SPEED`：刹车但不清速度。
  - `TOY_REMOTE_BRAKE_CLEAR_SPEED`：刹车并把速度清零。
- 转向 ADC 在 controller 侧归约为 `0..180` 舵机角度；反向只改变映射方向，不改变无线字段。
- TM1637 显示正常控制状态；Fn 按下时在本机电压和接收端回传电压之间低频切换。

## controller 配置模式线索

Tx V2.x 记录中配置模式涉及舵机反向、中值、减少角度、方向反转和 EEPROM 保存。新项目暂缓实现配置模式；实施前必须先设计持久化格式、校验、默认值和恢复路径。

## receiver 启动流程

```text
init output safe levels
init SPI
  -> init ADC status
  -> init nRF24 PRX on fixed address/channel
  -> preload ACK status payload
  -> loop:
       receive 32-byte radio packet
       proto_rf_link_poll DATA
       unpack and validate control payload
       apply outputs
       update ACK status payload
       enter safe state on receive timeout
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
set servo to center
continue listening for recovery
```

当前实现选择舵机回中，电机/MOS/灯/蜂鸣器关闭；这是旧 PCB 烧录可用优先的明确安全状态。

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

## 状态回传流程

```text
receiver:
  update link_state
  if controller requests voltage:
       low-frequency sample ADC1/P1.1
  build 4-byte toy status payload
  build 32-byte proto_rf_link STATUS packet
  write nRF24 ACK payload pipe0

controller:
  send DATA packet
  if TX_DONE and RX_READY:
       read ACK dynamic payload
       verify proto_rf_link STATUS header
       copy toy status fields
  Fn held:
       show local battery voltage with colon
       show receiver voltage without colon
```
