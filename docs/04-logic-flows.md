# 重要逻辑流程

## controller 启动流程

```text
init SPI
  -> init input and display
  -> load controller config: tx_id, last_channel, calibration
  -> show saved channel as Cxxx
  -> init nRF24 PTX on saved last_channel
      failure -> show E001 and stop in radio-error state
  -> try saved channel while showing Cxxx
  -> if this boot has not connected yet:
       scan channels 0..125 while showing Sxxx until a matching ACK status is found
  -> show found channel as Fxxx for a short confirmation window
  -> loop:
       sample inputs
       pack control payload
       make proto_rf_link DATA packet
       send packet every about 50ms
       read ACK payload status when present
       refresh TM1637 display
       on repeated radio failures after a successful connection:
            show Lxxx and keep retrying the current channel, do not scan
```

频道扫描只属于“本次上电尚未连上”的启动发现阶段。只要本次上电已经成功连接过，后续丢包、距离过远、遮挡或短时干扰都必须视为当前频道链路问题；controller 锁定当前频道重试，显示 `Lxxx`，不能自动扫频。重新扫频只能由重启、无有效保存频道或后续明确设计的用户手动搜索动作触发。

controller 连接阶段 TM1637 目标显示：

| 显示 | 含义 |
| --- | --- |
| `C040` | 正在尝试保存频道 40 |
| `S040` | 启动发现阶段正在扫描频道 40 |
| `F040` | 已找到频道 40，短暂停留后进入控制界面 |
| `L040` | 已连接过但当前丢链，保持频道 40 重试 |
| `E001` | controller nRF24 初始化失败，检查射频模块、供电和接线 |

## controller 正常模式线索

Tx V2.x 记录中正常模式包含方向、速度、刹车、转向、灯光、喇叭、辅助 PWM 和电压显示。新项目保留这些作为需求线索，但最终交互按新需求评估实现：

- 方向是持续状态，不做旧按钮切换兼容。
- 速度是 `0..100` 的持续状态；EC11 增减量在 controller 输入层限幅。
- 刹车语义在 controller 侧归约：
  - `TOY_REMOTE_BRAKE_RELEASE`：松开刹车，不改变速度。
  - `TOY_REMOTE_BRAKE_HOLD_SPEED`：刹车但不清速度。
  - `TOY_REMOTE_BRAKE_CLEAR_SPEED`：刹车并把速度清零。
- 转向 ADC 在 controller 侧归约为 `0..180` 舵机角度；发包前应用舵机反向、中位和端点收缩校准。
- TM1637 显示正常控制状态；Fn 按下时在本机电压和接收端回传电压之间低频切换。

正常控制界面目标语义：

| 显示元素 | 含义 |
| --- | --- |
| 第 1 位 `^` / `v` | 前进 / 后退 |
| 第 1 位 `-` | 刹车或安全控制状态 |
| 第 3-4 位 `00..99`、`A0` | 速度 0..100 |
| 冒号灭 | 最近通信正常 |
| 冒号快闪 | 当前频道出现发送失败或 ACK 缺失，仍未判定丢链 |
| `Lxxx` | 连续失败超过阈值，锁定当前频道重连 |

冒号不使用常亮表示故障。常亮不够醒目，且容易被理解为普通分隔符；链路异常必须用快闪或 `Lxxx` 这种明确状态提示。

## controller 配置模式

Tx V2.x 记录中配置模式涉及舵机反向、中值、减少角度、方向反转和 EEPROM 保存。新实现保留这组真实需求，但不复用旧 EEPROM 布局。

controller 使用 fixed-block EEPROM 保存 `tx_id`、`last_channel`、`flags`、`steering_reduce` 和 `steering_middle`。无有效配置时写入默认值；扫描找到 receiver 后保存频道；配置模式修改后立即保存。

```text
normal mode:
  EC11 SW + brake held about 3s -> enter config mode

config mode:
  receiver-facing control is forced safe: speed 0, brake 1, light/buzzer/aux off
  direction switch state -> direction reverse flag
  brake edge -> toggle steering reverse
  EC11 SW edge -> switch edited item between reduce and middle
  EC11 rotation -> adjust selected item
  buzzer edge -> exit config mode
  display shows reduce and middle; colon shows reverse/current item
```

## receiver 启动流程

```text
init output safe levels
init SPI
  -> init ADC status
  -> load receiver config: bound_tx_id, rf_channel, servo_reverse
  -> if P30 and P31 are held at boot, clear bound_tx_id
  -> init nRF24 PRX on saved rf_channel
      failure -> LED double-flash pause loop, keep outputs safe
  -> preload ACK status payload
  -> loop:
       handle P30/P31 channel add/minus
       receive 32-byte radio packet
       proto_rf_link_poll DATA
       unpack and validate control payload
       bind first valid tx_id or reject mismatched tx_id
       apply outputs
       update ACK status payload
       enter safe state on receive timeout
```

绑定和频道流程：

```text
receiver unbound:
  valid control tx_id != 0 -> save bound_tx_id

receiver bound:
  matching tx_id -> apply outputs
  different tx_id -> drop packet

receiver P30 rising edge -> channel + 1, wrap 125 to 0, save config
receiver P31 rising edge -> channel - 1, wrap 0 to 125, save config
receiver boot with P30+P31 held -> clear binding, keep channel
```

receiver LED 目标语义：

| 阶段 | LED 表现 | 含义 |
| --- | --- | --- |
| Boot | 快闪 3 次 | MCU 程序已启动 |
| Radio init error | 双闪后长停顿，循环 | 接收端 nRF24 初始化失败 |
| Waiting unbound | 单短闪后长停顿，循环 | 未绑定，等待第一台有效遥控器 |
| Waiting bound / link lost | 慢闪，约 500ms 翻转 | 已绑定但当前未收到有效业务包 |
| Connected | 常亮 | 正在收到有效控制包 |
| Binding cleared | 快闪 6 次后进入 Waiting unbound | P30+P31 上电清除绑定成功 |

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
  -> repeated failure: rescan channels
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
  build 6-byte toy status payload
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
