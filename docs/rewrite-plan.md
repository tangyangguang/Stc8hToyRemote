# 需求和重写设计

## 1. 项目目标

本项目重写 STC8H 玩具遥控器 controller 和接收机 receiver 固件。重写目标不是迁移旧实现，而是从旧项目提取真实业务需求，评估合理性，然后基于当前 monorepo 和 `../Stc8hBase` 独立设计清晰、可维护、可验证的新方案。

成功标准：

- controller 和 receiver 都能独立编译、烧录和诊断。
- 双板 nRF24L01 通信稳定，断开、恢复和失败状态可观察。
- receiver 掉线后进入明确安全状态：电机停止，灯和蜂鸣器关闭或进入预定义安全状态。
- 两端都按电池供电项目设计，默认降低无效轮询、无线占空比、显示占空比和外设耗电。
- 业务协议集中在 `shared/toy_remote_protocol.h/.c`，无线 payload 全部手工按字节打包。

## 2. Legacy 使用边界

`legacy/old-prj/` 只作为需求线索来源：

- 可以提取：硬件接线线索、控制输入、执行输出、用户可观察行为、异常场景、需要诊断的状态。
- 必须评估：这些需求是否仍合理，是否需要补充安全、节能、版本、校验和验证标准。
- 禁止使用：旧代码结构、旧函数、旧协议流程、旧全局缓冲区、旧 nRF24L01 实现、旧 EEPROM 布局和旧 API 兼容层。

旧实现中不合理的做法不迁移。新项目按当前目标和最佳实践重新设计。

## 3. 需求定义

### 3.1 controller 需求

- 读取用户输入：方向、速度、刹车、转向角度、灯、蜂鸣器、辅助 PWM、状态/电压请求。
- 周期发送最新控制状态。遥控类业务以“最新状态”为准，不排队重发过期控制包。
- 显示必要运行状态，例如速度、方向、连接状态、电池电压或错误状态。
- 通信失败时给出可观察状态，避免用户误以为接收机仍受控。
- 电池供电下应限制显示刷新、无线发送频率和阻塞等待时间；空闲或长时间无操作时预留低功耗策略。

### 3.2 receiver 需求

- 接收并验证 controller 的控制 payload。
- 输出执行器状态：电机、舵机、灯、蜂鸣器和辅助 PWM。
- 定期或按请求回传状态，例如链路状态和电池电压。
- 超过业务包超时时间后立即进入安全状态：
  - 电机速度为 0，驱动进入停止或明确刹车/滑行策略。
  - 灯关闭。
  - 蜂鸣器关闭。
  - 辅助 PWM 关闭。
  - 舵机策略需要硬件确认：默认保持最近有效角度，若实测存在安全风险再改为中位。
- 电池供电下应减少无意义 PWM 输出、ADC 采样和状态回传；断联等待时降低无线和外设活动频率。

### 3.3 通信需求

- 使用 `Stc8hBase/drivers/drv_nrf24l01` 访问 nRF24L01，不复用旧实现。
- 使用 `Stc8hBase/protocols/proto_rf_link` 作为 32 字节链路层。
- 链路包只负责连接、状态、序号、超时和 payload 搬运；玩具遥控业务语义只放在 `shared/`。
- ACK payload 只能作为短状态优化，不作为唯一双向业务协议。
- 第一阶段固定地址和固定频道；频道扫描、绑定和持久化配置后续单独设计。

## 4. 独立设计

### 4.1 分层

```text
controller/receiver application
  shared/toy_remote_protocol
  proto_rf_link
  drv_nrf24l01
  stc8h_spi/gpio/timer/adc/pwm
```

边界：

- `Stc8hBase` 提供芯片 HAL、外设驱动和通用链路协议。
- `shared/` 只定义玩具遥控业务 payload 和 pack/unpack。
- `controller/` 和 `receiver/` 只放各自板级引脚、状态机、输入输出和节能策略。

### 4.2 业务 payload

控制 payload 手工按字节打包：

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

状态 payload 手工按字节打包：

```text
byte0 version
byte1 link_state
byte2 voltage_int
byte3 voltage_dec
```

所有字段必须在解包时做版本和长度检查。范围检查由业务层处理，例如 speed 限制到 0..100，steering_angle 限制到 0..180。

### 4.3 状态机

controller：

```text
BOOT -> RADIO_CHECK -> CONNECTING -> CONNECTED -> LINK_LOST
```

receiver：

```text
BOOT -> RADIO_CHECK -> WAIT_CONTROLLER -> CONNECTED -> SAFE_STATE
```

receiver 的 `SAFE_STATE` 是业务安全状态，不等同于芯片休眠。进入安全状态后仍可低频监听恢复连接。

### 4.4 节能要求

两个固件都必须把节能作为设计输入，而不是后期补丁：

- 无线：控制包发送频率按操控体验选择最小可接受值；断联重试降频；避免忙等 `STATUS`。
- 显示：TM1637 只在内容变化或低频节拍刷新；空闲可降低亮度或关闭显示。
- ADC：电池电压和摇杆采样按需求限频；电压无请求时不高频采样。
- PWM：安全状态关闭不必要 PWM；舵机和电机输出按实际硬件确认是否可以停 PWM。
- 主循环：用 1ms tick 或软定时调度，不在主循环做长时间阻塞延时。
- 低功耗：后续硬件验证后再引入 idle/power-down；引入前必须确认 nRF24L01、按键和必要唤醒源。

## 5. 实施阶段

### 阶段 1：最小骨架

- `controller` 和 `receiver` 都能用 PlatformIO 编译。
- 两边都能 include `stc8h_config.h`、`drv_nrf24l01.h`、`proto_rf_link.h`。
- `shared/toy_remote_protocol` 能被两个固件共同引用。
- 不接真实业务输入输出，只验证分层和构建。

### 阶段 2：nRF24L01 双板通信

- 使用固定地址、固定频道、固定 payload。
- 验证 `TX_DONE`、`MAX_RETRY` 和断开恢复。
- 暂不接入业务输入输出。
- 记录无线发送频率、失败重试策略和断联降频策略。

### 阶段 3：接入 `proto_rf_link`

- 使用 32 字节链路包。
- 先实现 `HELLO`、`DATA`、`STATUS`。
- 明确掉线超时和 receiver 安全状态。
- controller 高频发送最新控制状态，receiver 低频或按需回传状态。

### 阶段 4：迁移业务

- controller 迁移摇杆、EC11、按键、TM1637 显示。
- receiver 迁移电机、舵机、灯、蜂鸣器、电压采样。
- 旧业务字段重设计为新的 `toy_remote_protocol` payload。
- 每迁移一个硬件能力，都要记录节能策略和安全状态。

## 6. 暂不做

- 不做频道扫描。
- 不做绑定持久化。
- 不做加密、mesh、路由、多节点调度。
- 不做旧项目 API 兼容层。

如果后续需要持久化频道、校准值或用户配置，必须先补充独立设计：数据版本、长度、CRC、默认值、备份恢复和升级路径。

## 7. 验证方式

- 构建验证：controller 和 receiver 分别 PlatformIO 编译通过。
- 协议验证：`toy_remote_protocol` pack/unpack 对版本、长度和字段边界有明确行为。
- 无线验证：记录连接、断开、恢复、`MAX_RETRY`、`RX_READY` 和状态回传。
- 安全验证：receiver 在超时后电机、灯、蜂鸣器和 aux PWM 符合安全状态。
- 节能验证：记录关键循环频率、发送周期、显示刷新周期、ADC 采样周期和断联重试周期。
