# 协议设计

## RF 链路

应用固件使用 `proto_rf_link` 的固定 32 字节包：

```text
byte0    magic
byte1    version
byte2    type
byte3    seq
byte4    ack_seq
byte5    flags
byte6    src_id
byte7    dst_id
byte8    len
byte9..  payload
```

controller 使用 src `1`、dst `2` 发送 `DATA`；receiver 使用 src `2`、dst `1` 在 ACK payload 中回传 `STATUS`。链路层只负责包类型、序号、源/目的和 payload 搬运，不理解玩具遥控业务字段。

## nRF24 配置

| 项 | 值 |
| --- | --- |
| 地址 | `TOYR1` |
| 默认频道 | `76` |
| 速率 | 250kbps |
| 发射功率 | 0dBm |
| Auto ACK | 开 |
| ACK payload | 开 |
| 控制包长度 | 32 字节 |
| 状态 ACK 长度 | 15 字节 |
| SETUP_RETR | ARD 1000us，ARC 15 |

250kbps + 15 字节 ACK payload 是距离优先默认配置。不要把 250kbps + 32 字节 ACK payload + 1500us ARD 作为长期默认。

## 控制 payload

ToyRemote 控制 payload 为 11 字节，位于 32 字节 `DATA` 包的 payload 区：

```text
byte0  version
byte1  direction
byte2  speed
byte3  brake
byte4  steering_angle
byte5  light
byte6  buzzer
byte7  aux_pwm
byte8  request_voltage
byte9  tx_id low
byte10 tx_id high
```

字段范围：

- `version`: 固定为 `1`
- `direction`: `0` 前进，`1` 后退
- `speed`: `0..100`
- `brake`: `0..1`
- `steering_angle`: `0..180`
- `light`: `0..1`
- `buzzer`: `0..1`
- `aux_pwm`: `0..100`
- `request_voltage`: `0..1`
- `tx_id`: `1..65535` 有效，低字节在前

业务 payload 必须手工按字节打包，不直接发送 C struct。

## 状态 payload

ToyRemote 状态 payload 为 6 字节，放在 15 字节 ACK payload 的 `STATUS` 包内：

```text
byte0 version
byte1 link_state
byte2 voltage_int
byte3 voltage_dec
byte4 tx_id low
byte5 tx_id high
```

字段范围：

- `version`: 固定为 `1`
- `link_state`: `0` idle, `1` connecting, `2` connected, `3` lost
- `voltage_int`: `0..99`
- `voltage_dec`: `0..99`
- `tx_id`: receiver 已绑定的 controller ID

电压使用百分之一伏表示。例如 7.42V 回传为 `voltage_int=7`、`voltage_dec=42`。

## 绑定规则

- controller 的 `tx_id=0` 无效。
- receiver 未绑定时，收到第一个合法非零 `tx_id` 后保存绑定。
- receiver 已绑定后，只接受匹配 `tx_id` 的控制包。
- P30+P31 上电同时按下可清除 receiver 绑定，频道不随绑定清除改变。

## 控制归约

controller 在发包前完成输入归约：

- EC11 旋转归约为 `speed=0..100`。
- EC11 中键短按：`brake=1` 且 `speed=0`。
- 单独刹车键：`brake=1`，速度保持。
- 转向 ADC 归约为 `0..180`，再应用舵机反向、中位虚位和端点收缩。
- `aux_pwm` 无 controller 输入映射，默认发送 `0`。

receiver 不解释原始按键，只执行归约后的 payload。

## 安全默认控制值

安全控制基线：

```text
direction        forward
speed            0
brake            0
steering_angle   90
light            0
buzzer           0
aux_pwm          0
request_voltage  0
tx_id            0
```

receiver 进入安全状态时，输出层会把 AT8236 duty、灯、蜂鸣器和辅助 PWM 清零，并让舵机回中。
