# Stc8hToyRemote

STC8H 玩具遥控器和接收机重写项目。

本项目从旧 Keil 工程重写，不保留旧 API 兼容层。旧项目已移动到：

```text
legacy/old-prj/
```

`legacy/` 只作为需求线索来源，不在其中修改代码，不参考旧实现方案。

## 基础库

基础库位于相邻目录：

```text
../Stc8hBase
```

新代码优先使用基础库的：

```text
core/
hal/
drivers/
protocols/
```

其中 nRF24L01 通信使用：

```text
drivers/drv_nrf24l01.h
protocols/proto_rf_link.h
```

## 目录

```text
controller/  遥控器固件
receiver/    接收机固件
shared/      玩具遥控业务协议
docs/        需求、架构、协议、流程、节能和验证文档
legacy/      旧项目，只读参考
tests/       本机协议测试
```

## 文档

- `docs/01-requirements.md`：需求说明和 legacy 边界。
- `docs/02-architecture.md`：分层、构建策略和资源原则。
- `docs/03-protocol.md`：链路层和业务 payload。
- `docs/04-logic-flows.md`：controller、receiver、安全状态和无线失败流程。
- `docs/05-power.md`：电池供电节能设计。
- `docs/06-verification.md`：本机、构建和硬件验证计划。
- `docs/07-tx-v2-notes-assessment.md`：Tx V2.x 记录需求评估。
- `docs/rewrite-plan.md`：阶段路线图。

## 当前阶段

当前已完成阶段 2 first build：固定地址、固定频道、固定 32-byte payload 的 nRF24L01 双板通信骨架。

阶段顺序：

1. 验证 controller/receiver 能引用 `Stc8hBase`。
2. 做 nRF24L01 固定 payload 双板通信。
3. 接入 `proto_rf_link`。
4. 迁移摇杆、EC11、按键、显示、电机、舵机、灯、蜂鸣器等业务功能。

## 验证

本机协议测试：

```sh
cc -std=c99 -Wall -Wextra -Ishared -I../Stc8hBase/core tests/toy_remote_protocol_test.c shared/toy_remote_protocol.c -o /tmp/toy_remote_protocol_test
/tmp/toy_remote_protocol_test
```

固件构建：

```sh
(cd controller && pio run)
(cd receiver && pio run)
```

## 构建约束

PlatformIO wrapper 只为当前固件实际使用的 `Stc8hBase` `.c` 文件存在。不要提前编译后续阶段才需要的基础库模块；STC8H1K08 的 flash 和内部 RAM 都很小，未使用函数也会增加 SDCC 链接压力。
