# 架构设计

## 分层

```text
controller/receiver application
  shared/toy_remote_protocol
  proto_rf_link
  drv_nrf24l01
  stc8h_spi/gpio/timer/adc/pwm
```

职责边界：

- `Stc8hBase` 提供芯片 HAL、外设驱动和通用链路协议。
- `shared/` 只定义玩具遥控业务 payload、字段常量、校验和 pack/unpack。
- `controller/` 放遥控器板级引脚、输入采样、显示、无线发送和节能策略。
- `receiver/` 放接收机板级引脚、输出执行、安全状态、无线接收和节能策略。
- `legacy/` 只读，只作为需求线索。

## 构建策略

项目使用 PlatformIO wrapper 引入 `../Stc8hBase` 源文件。wrapper 只为当前固件实际使用的 `.c` 文件存在，不提前编译后续阶段才需要的模块。

原因：

- STC8H1K08 只有 8KB flash 和 1.25KB RAM。
- SDCC 会为被编译进固件的函数分配参数区，即使当前主循环暂时没有调用全部函数。
- 只编译实际使用模块能降低 flash、DSEG 和链接风险，也符合基础库应用集成策略。

当前阶段 2 固件只编译：

- `drv_nrf24l01`
- `stc8h_spi`
- 应用侧 `app_radio`

`proto_rf_link` 和 `toy_remote_protocol` 在阶段 3 接入链路层时再加入固件 wrapper。`shared/toy_remote_protocol` 已通过本机 C 测试验证。

## 当前模块

controller：

- `controller/src/app_radio.h/.c`：配置 nRF24L01 为 PTX，发送固定 32 字节测试包。
- `controller/src/main.c`：初始化 SPI 和 radio，循环发送测试包。

receiver：

- `receiver/src/app_radio.h/.c`：配置 nRF24L01 为 PRX，接收固定 32 字节测试包。
- `receiver/src/main.c`：初始化 SPI 和 radio，循环接收测试包并记录 seq。

shared：

- `shared/toy_remote_protocol.h/.c`：业务协议字段、范围校验、安全默认值、pack/unpack。

## 资源原则

- 新增基础能力前先确认是否属于当前阶段真实需要。
- 不为单次使用创建抽象。
- 不复制基础库源码。
- 遇到基础库能力不足时，不在应用项目临时绕过；整理问题给基础库项目处理。
