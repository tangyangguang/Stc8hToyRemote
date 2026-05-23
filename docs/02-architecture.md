# 架构设计

## 分层

```text
controller/receiver application
  shared/toy_remote_protocol
  Stc8hBase/protocols/proto_rf_link
  Stc8hBase/drivers/drv_nrf24l01
  Stc8hBase/hal + core
```

职责边界：

- `Stc8hBase` 提供芯片 SFR、HAL、通用驱动和通用 RF 链路协议。
- `shared/` 定义 ToyRemote 业务 payload、字段范围、pack/unpack 和安全默认值。
- `controller/` 负责遥控器输入、显示、配置保存、PTX 发送和链路状态机。
- `receiver/` 负责接收、绑定、安全输出、PRX ACK 状态回传和执行器驱动。
- `legacy/` 只读，只作为需求和硬件线索来源。

## 构建策略

项目使用 PlatformIO wrapper 引入 `../Stc8hBase` 的必要 `.c` 文件。STC8H1K08 只有 8KB flash 和 1.25KB RAM；SDCC 会为被编译进固件的函数分配参数区，因此只编译当前固件实际使用的基础库模块。

构建裁剪通过 `platformio.ini` 的宏完成：

- `PROTO_RF_LINK_ENABLE_*`：只启用固定 DATA 发送或固定 DATA 轮询所需路径。
- `TOY_REMOTE_ENABLE_*`：STC 固件关闭未用的通用协议 API；host 测试保留完整 API。
- `DRV_NRF24L01_ENABLE_*`：按 PTX/PRX 和诊断需要裁剪 nRF24 API。
- `STC8H_PWM_*`：receiver 使用固定 PWM 通道，关闭运行期通道检查和 duty clamp。

## Controller 模块

- `app_input`：EC11、按键和 ADC 采样；Timer0 ISR 解码 EC11，主循环消费累计 delta。
- `app_display`：TM1637 段码和状态显示。
- `app_button`：EC11 中键短按、双击、长按事件。
- `app_config`：EEPROM fixed-block 配置，保存 `tx_id`、频道和舵机/方向校准。
- `app_radio`：nRF24 PTX 初始化、发送、ACK payload 分类和 MAX_RT 恢复。
- `main`：启动流程、配置模式、频道扫描、控制包打包和显示状态机。

## Receiver 模块

- `app_outputs`：PWM 和 GPIO 输出，包含舵机、电机、灯、蜂鸣器和辅助 PWM。
- `app_outputs_calc`：输出曲线宏；PWM-B 默认约 20.03kHz，电机最低有效 duty 由 `APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT` 配置，默认 20%。
- `app_indicator`：receiver LED 生命周期状态。
- `app_status`：状态回传数据，按请求低频采样电池电压。
- `app_config`：EEPROM fixed-block 配置，保存绑定 `tx_id` 和频道配置；配置版本 2 的 byte7 为保留字节，保存时写 0。
- `app_radio`：nRF24 PRX 初始化、收包、ACK payload 预装和频道设置。
- `main`：绑定、收包、输出应用、安全态和 ACK 状态更新。

## RF 配置

默认应用配置：

| 项 | 值 |
| --- | --- |
| 地址 | `TOYR1` |
| 默认频道 | `76` |
| 速率 | 250kbps |
| 发射功率 | 0dBm |
| 控制包 | 32 字节 nRF24 payload，内含 11 字节 ToyRemote control |
| ACK payload | 15 字节，9 字节 `proto_rf_link` 头 + 6 字节 ToyRemote status |
| Auto ACK | 开 |
| Dynamic payload | 用于 ACK payload |
| SETUP_RETR | ARD 1000us，ARC 15 |

receiver 启动和恢复时预装 3 个 ACK payload 槽；首次绑定后替换 ACK FIFO，确保回传新的 `tx_id`；正常收到控制包后追加下一份 ACK payload，避免每包 flush TX 造成空 ACK。

## 输出策略

- AT8236 电机只由 `P3.3/PWM7` 和 `P3.4/PWM8` 控制。
- `P5.4/PWM6` 是保留辅助 PWM，来自 `aux_pwm` 字段；与 AT8236 共用 PWM-B 频率，controller 当前始终发送 `0`，所以默认关闭。
- 舵机使用 `P1.0/PWM1P`，50Hz，安全态回中。
- 灯光和蜂鸣器低电平有效，安全态关闭。
- receiver LED 高电平亮，用于生命周期状态提示。

## 资源原则

- 不复制基础库代码，不在应用层绕开已确认属于基础库的问题。
- 不为旧 API 或旧 EEPROM 布局保留兼容负担。
- 新增功能必须同时评估 controller 和 receiver flash 余量。
- 无线 payload 必须按字节手工打包，不直接发送 C struct。
