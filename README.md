# Stc8hToyRemote

STC8H1K08 玩具遥控器和接收机固件。项目使用 PlatformIO + SDCC 构建，公共芯片能力来自相邻基础库 `../Stc8hBase`，应用层只保留玩具遥控业务协议和板级逻辑。

## 功能

- controller 读取 EC11 速度、EC11 中键、方向、刹车、灯、蜂鸣器、Fn、电位器转向输入。
- controller 通过 TM1637 显示连接状态、方向、速度、配置项和电压。
- receiver 接收控制包，驱动 AT8236 电机、舵机、灯、蜂鸣器和辅助 PWM。
- nRF24L01 使用固定地址 `TOYR1`、默认频道 `76`、250kbps、0dBm、auto ack、15 字节 ACK payload。
- receiver 未绑定时绑定第一台合法 `tx_id` 的 controller；已绑定后拒绝其他 `tx_id`。
- receiver 掉线进入安全状态：电机停止、灯/蜂鸣器/辅助 PWM 关闭、舵机保持最后角度。
- controller 丢链后显示 `Lxxx` 并锁定该频道重试；只有在 `Lxxx` 下双击 EC11 中键才进入扫描。

## 目录

```text
controller/  遥控器固件
receiver/    接收机固件
shared/      玩具遥控业务协议
docs/        最终需求、架构、协议、流程、硬件和验证文档
legacy/      旧项目，只读需求参考
tests/       本机 C 测试
tools/       检查、尺寸和烧录辅助脚本
```

## 基础库边界

新代码引用相邻目录：

```text
../Stc8hBase
```

应用项目使用基础库的 `core/`、`hal/`、`drivers/`、`protocols/`，不复制基础库源码，不复用旧项目 `my_nRF24L01` 实现。nRF24L01 通信通过 `drv_nrf24l01` 和 `proto_rf_link` 接入；玩具遥控业务 payload 只放在 `shared/`。

## 文档

- `遥控器与接收机核心逻辑说明.md`：系统核心逻辑、接收机动作、数据包格式、参数、配置和取值范围。
- `遥控器操作与界面配置说明.md`：遥控器界面含义、日常操作、扫频和配置模式说明。
- `docs/01-requirements.md`：最终需求和非目标。
- `docs/02-architecture.md`：分层、模块、资源和配置策略。
- `docs/03-protocol.md`：RF 链路、业务 payload、绑定和安全字段。
- `docs/04-logic-flows.md`：controller/receiver 生命周期和安全状态。
- `docs/05-power.md`：节能策略和低功耗边界。
- `docs/06-verification.md`：构建、烧录、实机验收和诊断命令。
- `docs/07-ec11-input.md`：EC11 输入采样、加速和中键语义。
- `docs/08-hardware-map.md`：硬件引脚映射。
- `docs/09-upload.md`：STCGAL 烧录配置、命令和排查方法。

## 构建检查

全量检查：

```sh
./tools/check_all.sh
```

单独构建：

```sh
(cd controller && pio run)
(cd receiver && pio run)
```

资源阈值由 `tools/check_firmware_size.sh` 校验。STC8H1K08 只有 8KB flash 和紧张的 internal RAM，新增功能必须先确认 controller 和 receiver 的 flash、stack 可用空间和最大连续 internal RAM 均不过界。

## 烧录

controller 默认上传口：

```sh
cd controller
pio run -t upload --upload-port /dev/cu.usbserial-110
```

receiver 默认上传口：

```sh
cd receiver
pio run -t upload --upload-port /dev/cu.usbserial-120
```

上传脚本使用 `stcgal` 的 `stc8g` 协议，并带自动重试和回退。烧录说明见 `docs/09-upload.md`。

## 关键编译期配置

- `APP_OUTPUT_FAST_PWM_PRESCALER` / `APP_OUTPUT_FAST_PWM_PERIOD`：AT8236 和辅助 PWM 共用的 PWM-B 频率配置，默认 `5u` / `91u`，11.0592MHz 下约 20.03kHz。
- `APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT`：电机最低有效 PWM 占空比百分比，默认 `35u`。
- `APP_RECEIVER_ENABLE_CHANNEL_BUTTONS`：是否启用 receiver P30/P31 本机频道维护键，默认关闭。
- `APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS`：是否启用 receiver P30+P31 上电清绑定，默认开启。
- `APP_RADIO_ENABLE_STATS`：RF 轻量统计，默认关闭。
- `APP_INPUT_DIAG_DISPLAY`：controller 输入显示诊断，默认关闭。

## Legacy 规则

`legacy/old-prj/` 只用于确认硬件接线、业务需求和用户可观察行为；不能修改其中代码，不能把旧代码结构、旧协议流程、旧 EEPROM 布局或旧 nRF24 驱动迁移到新项目。
