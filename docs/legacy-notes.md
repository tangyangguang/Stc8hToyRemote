# Legacy 说明

旧项目位置：

```text
legacy/old-prj/
```

包含：

```text
my_stc8h_lib_For_V2.4/
toy_RemoteController_V2_Tx_joystick(v24_2Brake_PinSW2_VoltCheck_Postback)/
toy_RemoteController_V2_Rx_AT8236(V24_Postback_IRQ_P32)/
```

## 使用方式

- 只读查看旧项目，提取需求、硬件接线、业务行为和异常处理经验。
- 不修改 legacy 中任何文件。
- 不复制旧 `my_nRF24L01` 实现。
- 不保留旧全局 `tx_buf/rx_buf/ack_buf` 设计。

## 已知需求样本

- controller 发送方向、速度、刹车、舵机角度、灯、蜂鸣器等控制数据。
- receiver 回传状态，例如电池电压。
- receiver 长时间收不到控制包后必须进入安全状态。
- nRF24L01 通信需要可靠发送、失败恢复和可诊断状态。
