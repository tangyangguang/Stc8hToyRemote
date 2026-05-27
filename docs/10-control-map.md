# 控制功能核对表

本文按当前正式固件实现整理 controller 输入、无线控制包和 receiver 本地输出动作，用于和实际硬件逐项核对。

## Controller 输入

当前参与控制的输入可以分为 6 个数字按键/开关、1 个 EC11 旋转输入和 1 路转向 ADC。

| 输入 | 引脚/来源 | 当前功能 | 发送值 |
| --- | --- | --- | --- |
| EC11 旋转 | P1.1 / P1.0 | 调速度 | `speed=0..100`；慢转每步 1，快转每步 5 |
| EC11 中键 | P5.4 | 普通模式按下刹车并清速度；长按 5 秒进设置；丢联时双击触发扫频 | `brake=1` 且 `speed=0`；长按/双击是本机 UI 行为 |
| 方向键/方向开关 | P3.7 | 前进/后退 | `direction=0` 前进，`direction=1` 后退；可被设置项反转 |
| 灯键 | P3.6 | 灯开关 | `light=0..1` |
| 蜂鸣器键 | P3.5 | 蜂鸣器开关 | `buzzer=0..1` |
| 独立刹车键 | P3.0 | 刹车 | `brake=0..1`；不清 `speed` |
| Fn 键 | P3.4 | 请求/显示电压 | `request_voltage=0..1` |
| 转向电位器 | ADC11 / P3.3 | 舵机角度 | 原始 `0..1023` 映射到 `steering_angle=0..180`；可反向、调中点、限制端点 |
| AUX PWM | 无当前输入 | 预留输出通道 | `aux_pwm=0..100`；当前 controller 始终发送 `0` |

如果只数按钮/开关，当前是 6 个：EC11 中键、方向、灯、蜂鸣器、独立刹车、Fn。

## 控制包结构

nRF24 实际发送固定 32 字节包。前 9 字节是 `proto_rf_link` 头，后 11 字节是 ToyRemote 控制 payload，剩余字节补 0。

| 字节 | 字段 | 值/范围 |
| ---: | --- | --- |
| 0 | magic | `0xA5` |
| 1 | link version | `0x01` |
| 2 | packet type | `3`，DATA |
| 3 | seq_tx | 发送序号 |
| 4 | seq_rx | 最近接收序号；当前固定发送路径可为 0 |
| 5 | flags | `1`，要求 ACK |
| 6 | local_id | controller 为 `1` |
| 7 | peer_id | receiver 为 `2` |
| 8 | payload_len | `11` |
| 9 | protocol version | `1` |
| 10 | direction | `0..1` |
| 11 | speed | `0..100` |
| 12 | brake | `0..1` |
| 13 | steering_angle | `0..180` |
| 14 | light | `0..1` |
| 15 | buzzer | `0..1` |
| 16 | aux_pwm | `0..100`；当前正常为 `0` |
| 17 | request_voltage | `0..1` |
| 18 | tx_id low | 默认 `0x21`，来自默认 `tx_id=0x4A21` |
| 19 | tx_id high | 默认 `0x4A` |
| 20..31 | padding | `0` |

业务 payload 必须手工按字节打包，不直接发送 C struct。

## Receiver 校验和绑定

receiver 收到包后先做以下校验：

- `proto_rf_link` 包头、包类型、源/目的 ID 和 payload 长度必须正确。
- `version` 必须为 `1`。
- `direction/speed/brake/steering_angle/light/buzzer/aux_pwm/request_voltage` 必须在合法范围内。
- `tx_id` 不能为 `0`。
- 未绑定时，保存第一个合法非零 `tx_id`；已绑定后，只接受匹配 `tx_id` 的控制包。

## Receiver 本地动作

| 接收字段 | 本地动作 |
| --- | --- |
| `direction + speed + brake` | 控制 AT8236 两路 PWM。不刹车时 `direction=0` 输出前进 PWM，`direction=1` 输出后退 PWM；`speed<5` 输出 0；`speed=5..100` 映射到 20%..100% 占空比；`brake=1` 时前进/后退两路都输出 100% duty |
| `steering_angle` | 控制 P1.0 / PWM1P 舵机，`0..180` 映射到舵机 PWM，90 为中位 |
| `light` | 控制 P3.5 灯输出，1 开，0 关 |
| `buzzer` | 控制 P3.6 蜂鸣器输出，1 开，0 关 |
| `aux_pwm` | 控制 P5.4 / PWM6_2 辅助 PWM，`0..100`；当前 controller 没有输入源，正常一直为 0 |
| `request_voltage` | 为 1 时采样 receiver 电池电压，并通过 ACK payload 回传 |
| `tx_id` | 用于首次绑定和后续包过滤 |

## 状态 ACK

receiver 通过 15 字节 ACK payload 回传状态。结构为 9 字节 `proto_rf_link` 头加 6 字节状态 payload。

| payload 字节 | 字段 | 值/范围 |
| ---: | --- | --- |
| 0 | version | `1` |
| 1 | link_state | `0` idle，`1` connecting，`2` connected，`3` lost |
| 2 | voltage_int | `0..99` |
| 3 | voltage_dec | `0..99` |
| 4 | tx_id low | receiver 已绑定的 controller ID 低字节 |
| 5 | tx_id high | receiver 已绑定的 controller ID 高字节 |

电压按百分之一伏显示。例如 7.42V 回传为 `voltage_int=7`、`voltage_dec=42`。

## 安全态

receiver 在掉线、射频错误或长时间收不到合法控制包时进入安全态：

```text
AT8236 前进 PWM = 0
AT8236 后退 PWM = 0
辅助 PWM = 0
灯关闭
蜂鸣器关闭
舵机回中
```

刹车命令和安全态不同：刹车时 AT8236 前进/后退两路同时输出 100% duty；安全态下两路均为 0。

## 硬件核对重点

- `aux_pwm` 在 receiver 有实际输出，但 controller 端当前没有任何输入控制它。
- EC11 中键和独立刹车键都会产生刹车，但 EC11 中键会清速度，独立刹车键不清速度。
- Fn 键当前不是普通输出开关，只用于请求/显示电压。
- 方向、灯、蜂鸣器、独立刹车、Fn、EC11 中键都是低电平有效输入。
