# 协议设计

## 链路层

当前固件使用 `Stc8hBase/protocols/proto_rf_link` 的固定 32 字节链路包：

```text
byte0  magic
byte1  version
byte2  type
byte3  seq
byte4  ack_seq
byte5  flags
byte6  src_id
byte7  dst_id
byte8  len
byte9..31 payload
```

链路层只负责序号、包类型和 payload 搬运，不理解玩具遥控业务字段。控制包由 controller 通过 `proto_rf_link_send_data()` 生成，receiver 通过 `proto_rf_link_poll()` 验证并取出业务 payload。

状态回传走 nRF24L01 ACK payload：receiver 预装一个 32 字节 `PROTO_RF_LINK_PACKET_STATUS` 包，controller 在下一次成功发送控制包后读取 ACK payload。STC8H1K08 RAM 很小，controller 的 ACK 状态解析和 receiver 的 ACK 状态包生成按 `proto_rf_link` 线格式做轻量处理；主控制链路仍使用 `proto_rf_link` API。

## 业务控制 payload

业务 payload 位于 `shared/toy_remote_protocol.h/.c`，手工按字节打包，不直接发送 C struct：

```text
byte0 version
byte1 direction
byte2 speed
byte3 brake
byte4 steering_angle
byte5 light
byte6 buzzer
byte7 aux_pwm
byte8 request_voltage
```

字段范围：

- `version`: 当前为 `1`
- `direction`: `0` forward, `1` reverse
- `speed`: `0..100`
- `brake`: `0..1`
- `steering_angle`: `0..180`
- `light`: `0..1`
- `buzzer`: `0..1`
- `aux_pwm`: `0..100`
- `request_voltage`: `0..1`

该格式吸收了 Tx V2.x 记录中的核心控制字段，但不是旧格式兼容层。新格式增加 `version`，并且后续会放入 `proto_rf_link` 的 payload 中传输。

## 业务状态 payload

```text
byte0 version
byte1 link_state
byte2 voltage_int
byte3 voltage_dec
```

字段范围：

- `version`: 当前为 `1`
- `link_state`: `0` idle, `1` connecting, `2` connected, `3` lost
- `voltage_int`: 电压整数部分，`0..99`
- `voltage_dec`: `0..99`

`toy_remote_status_set_voltage_centivolts()` 统一把百分之一伏表示拆成 `voltage_int` 和 `voltage_dec`。例如 `742` 表示 7.42V。超过 99.99V 时夹到 99.99V。

状态 payload 的字段偏移由 `TOY_REMOTE_STATUS_OFFSET_*` 定义，便于 8KB 目标在 ACK payload 热路径中避免额外跨翻译单元参数区。

## 校验规则

- pack 前必须验证结构体字段范围。
- unpack 时必须先验证长度和版本，再写入结构体字段。
- unpack 后必须验证字段范围；非法 payload 返回 `STC8H_ERROR`。
- 不直接发送 C struct。
- ACK payload 只承载短状态回传，不承载控制命令。

## 安全默认控制值

`toy_remote_control_set_safe()` 生成中性控制值：

```text
direction       forward
speed           0
brake           0
steering_angle  90
light           0
buzzer          0
aux_pwm         0
request_voltage 0
```

receiver 进入安全状态时可以以此作为业务输入基线，再结合具体电机驱动策略决定刹车或滑行。

## 刹车动作归约

Tx V2.x 记录里有两类刹车输入。新项目在 `shared/` 中定义为输入归约动作，而不是增加无线字段：

```text
TOY_REMOTE_BRAKE_RELEASE       brake = 0, speed unchanged
TOY_REMOTE_BRAKE_HOLD_SPEED    brake = 1, speed unchanged
TOY_REMOTE_BRAKE_CLEAR_SPEED   brake = 1, speed = 0
```

controller 采样按键后先把输入归约到 `toy_remote_control_t`，receiver 只接收归约后的 `brake` 和 `speed`。这样无线协议保持简单，后续如果按键硬件变化，不影响 receiver 协议解析。

## 转向 ADC 归约

controller 侧使用 `toy_remote_control_set_steering_from_adc()` 把 10-bit ADC 值归约成舵机角度：

```text
ADC 0      -> 0 degree
ADC 512    -> 90 degree
ADC 1023   -> 180 degree
ADC >1023  -> 180 degree
reverse=1  -> 角度反向
```

该函数只做线性映射、限幅和可选反向，不处理舵机中值校准、死区或端点减少角度。校准属于后续配置模式，需要持久化设计后再实现。

## 速度归约

EC11 或其他速度输入先在 controller 侧归约为 `0..100`：

```text
toy_remote_control_adjust_speed(control, delta)
```

该函数要求当前 control 已合法，然后按 signed delta 调整速度并限幅。速度归约不改变刹车状态；如果需要“刹车并清速度”，应调用刹车动作归约。
