# 当前实现总览

本文记录当前最终实现的结构和维护边界。

## 目标

本项目在 STC8H1K08 + nRF24L01 硬件上实现玩具遥控器 controller 和接收机 receiver 固件：

- controller 采集用户输入并周期发送最新控制状态。
- receiver 接收合法控制包后驱动电机、舵机、灯、蜂鸣器和辅助 PWM。
- receiver 通过 ACK payload 回传短状态。
- 双端使用 `../Stc8hBase` 的 HAL、nRF24 驱动和 `proto_rf_link`，应用层只保留 ToyRemote 业务。
- 掉线或射频错误时 receiver 进入明确安全输出。

## 模块关系

```text
controller/src      遥控器输入、显示、配置、无线发送
receiver/src        接收机绑定、安全态、无线接收、输出驱动
shared/             ToyRemote 控制/状态 payload
../Stc8hBase        STC8H HAL、nRF24L01 驱动、RF 链路协议
tests/              host 侧协议和配置测试
tools/              检查、尺寸、上传和诊断辅助脚本
```

## 当前默认通信方案

| 项 | 值 |
| --- | --- |
| 地址 | `TOYR1` |
| 默认频道 | `76` |
| 速率 | 250kbps |
| 发射功率 | 0dBm |
| 控制包 | 32 字节固定 nRF24 payload |
| ACK 状态 | 15 字节动态 ACK payload |
| Auto ACK | 开 |
| SETUP_RETR | ARD 1000us，ARC 15 |

receiver 初始化和恢复时预装 3 个 ACK payload；正常收到控制包后追加下一份 ACK payload，不在每包 RX 后清空 TX FIFO。

## 当前业务能力

controller：

- EC11 调速，正常模式支持快转加速。
- EC11 中键短按清零刹车，长按进入配置，`Lxxx` 下双击扫描。
- 方向、独立刹车、灯、蜂鸣器、Fn 和转向 ADC。
- TM1637 显示连接状态、速度、方向、配置项和电压。
- EEPROM 保存 `tx_id`、频道和校准配置。

receiver：

- 绑定第一个合法非零 `tx_id`，已绑定后只接受匹配 controller。
- AT8236：P3.3/PWM7 前进，P3.4/PWM8 后退。
- 舵机：P1.0/PWM1P，50Hz。
- 灯、蜂鸣器、LED 和辅助 PWM。
- 按请求采样电池电压并回传。
- 掉线安全态关闭电机、灯、蜂鸣器、辅助 PWM，并让舵机回中。

## 配置和扩展边界

- `APP_OUTPUT_MOTOR_MIN_DUTY` 控制电机最低有效 duty，默认 20%。
- `APP_RECEIVER_ENABLE_CHANNEL_BUTTONS` 默认关闭；启用后 P30/P31 只用于维护式频道切换。
- `APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS` 默认开启；用于 P30+P31 上电清除绑定。
- `APP_RADIO_ENABLE_STATS` 可启用 RF 统计，默认关闭以节省 flash。
- `APP_INPUT_DIAG_DISPLAY` 可启用 controller 输入显示诊断，默认关闭。

新增功能前必须先检查 flash 余量、RAM 参数区、无线 payload 字段和 receiver 安全态影响。

## 不支持的内容

- 旧 API 兼容层。
- 旧无线 payload 格式。
- 旧 EEPROM 地址和布局。
- 旧 `my_nRF24L01` 驱动。

诊断固件和验证命令保留在 `docs/06-verification.md`，用于后续硬件问题复现和定位。
