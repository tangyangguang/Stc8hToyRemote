# Stc8hToyRemote

STC8H 玩具遥控器和接收机重写项目。

本项目从旧 Keil 工程重写，不保留旧 API 兼容层。旧项目已移动到：

```text
legacy/old-prj/
```

`legacy/` 只作为需求和行为参考，不在其中修改代码。

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
docs/        重写计划和 legacy 记录
legacy/      旧项目，只读参考
```

## 当前阶段

当前只建立最小可编译骨架。后续顺序：

1. 验证 controller/receiver 能引用 `Stc8hBase`。
2. 做 nRF24L01 固定 payload 双板通信。
3. 接入 `proto_rf_link`。
4. 迁移摇杆、EC11、按键、显示、电机、舵机、灯、蜂鸣器等业务功能。
