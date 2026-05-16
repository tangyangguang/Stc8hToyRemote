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

- 只读查看旧项目，只提取需求线索、硬件接线、用户可观察行为和异常场景。
- 旧项目不是实现参考；不能参考旧代码结构、协议流程、函数划分、时序写法或持久化布局。
- 从旧项目提取到的每条需求都要重新评估：合理的保留，不合理的修正，不清楚的补充定义。
- 不修改 legacy 中任何文件。
- 不复制旧 `my_nRF24L01` 实现。
- 不保留旧全局 `tx_buf/rx_buf/ack_buf` 设计。

## 已知需求样本

- controller 发送方向、速度、刹车、舵机角度、灯、蜂鸣器等控制数据。
- receiver 回传状态，例如电池电压。
- receiver 长时间收不到控制包后必须进入安全状态。
- nRF24L01 通信需要可靠发送、失败恢复和可诊断状态。

## 需求评估规则

- 只把 legacy 当作“可能需要支持什么”的线索，不把“它如何实现”带入新项目。
- 业务需求要落到新协议、新状态机和新硬件抽象里。
- 与安全、节能、可验证性冲突的旧行为必须重新设计。
- 涉及 EEPROM/IAP 等持久化配置时，必须先设计版本、校验、默认值、备份恢复或升级路径，再实施。
