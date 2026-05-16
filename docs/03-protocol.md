# 协议设计

## 链路层

阶段 3 使用 `Stc8hBase/protocols/proto_rf_link`，固定 32 字节链路包：

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

链路层只负责连接、状态、序号、超时和 payload 搬运，不理解玩具遥控业务字段。

## 业务控制 payload

业务 payload 位于 `shared/toy_remote_protocol.h/.c`，手工按字节打包：

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
- `voltage_int`: 电压整数部分
- `voltage_dec`: `0..99`

## 校验规则

- pack 前必须验证结构体字段范围。
- unpack 时必须先验证长度和版本，再写入结构体字段。
- unpack 后必须验证字段范围；非法 payload 返回 `STC8H_ERROR`。
- 不直接发送 C struct。
- ACK payload 只能作为短状态优化，不作为唯一双向业务协议。

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
