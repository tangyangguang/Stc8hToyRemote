# Tx V2.x 记录取舍

`legacy/old-prj/Tx遥控器端V2.x功能说明.txt` 只作为旧遥控端需求线索，不作为实现方案。当前项目按以下结论保留、修正或暂不启用相关功能。

## 保留为核心需求

- 方向：前进/后退。
- 速度：`0..100`。
- 刹车：支持清零速度刹车和保持速度刹车两类用户语义。
- 转向：电位器归约为舵机角度 `0..180`，默认中位约 `90`。
- 灯光：开关量。
- 蜂鸣器：开关量。
- Fn：用于电压显示或请求接收端回传电压。
- 通信状态：controller 必须显示连接、丢链、扫描、找到和射频初始化失败状态。
- receiver 掉线必须进入安全输出。

## 已重新设计

| 旧记录能力 | 当前设计 |
| --- | --- |
| 业务数据直接描述 nRF24 payload | 业务 payload 放入 `proto_rf_link` DATA/STATUS 包 |
| 无版本字段 | 控制和状态 payload 都包含 `version` |
| 旧 EEPROM 地址和布局 | 使用本项目 fixed-block 配置格式，不兼容旧布局 |
| EC11 中键刹车 | 短按清零速度并刹车 |
| 独立刹车键 | 刹车但保持速度，松开后恢复速度输出 |
| 舵机配置 | 支持方向反向、舵机反向、中位和端点收缩 |
| 通信失败显示 | 使用 `Lxxx`、`Sxxx`、`Fxxx`、`E001` 等明确状态 |

## 辅助 PWM

协议保留 `aux_pwm` 字段，receiver 保留 `P5.4/PWM6` 辅助 PWM 输出，并在安全态关闭。

controller 当前没有确认的第二电位器或独立辅助 PWM 输入映射，因此默认发送 `aux_pwm=0`。后续若硬件确认有辅助输入，应先补充引脚、ADC 通道、显示和安全策略，再启用 controller 端映射。

## 当前协议覆盖

控制 payload：

```text
version
direction
speed
brake
steering_angle
light
buzzer
aux_pwm
request_voltage
tx_id
```

状态 payload：

```text
version
link_state
receiver_voltage
bound_tx_id
```

字段定义和范围以 `shared/toy_remote_protocol.h` 为准。

## 不迁移的旧行为

- 不复用旧 `my_nRF24L01` 驱动。
- 不复用旧全局 `tx_buf/rx_buf/ack_buf` 设计。
- 不兼容旧无线 payload 格式。
- 不兼容旧 EEPROM 布局。
- 不把频道切换做成已连接状态下的隐式协商；误切频道按丢链处理。
