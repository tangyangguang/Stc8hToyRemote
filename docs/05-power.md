# 节能设计

controller 和 receiver 都使用电池，节能是设计输入，不是后期补丁。

## 通用原则

- 不做无意义忙等。
- 周期任务用 tick 或软定时调度。
- 无线、显示、ADC、PWM 都按需求限频。
- 断联状态降低重试和显示活动频率。
- 低功耗模式必须在硬件唤醒源确认后再实施。

## controller 节能策略

- 无线发送频率按操控体验选择最小可接受值。
- 输入没有变化时仍发送心跳，但频率应低于持续操控状态。
- TM1637 只在内容变化或低频节拍刷新。
- 空闲时可降低显示亮度或关闭显示。
- 电池电压和摇杆 ADC 按限频采样，不在每轮主循环无条件采样。
- 长时间无操作后预留 idle/power-down 策略。

## receiver 节能策略

- WAIT_CONTROLLER 或 SAFE_STATE 中降低状态回传和重试频率。
- 灯、蜂鸣器、aux PWM 在安全状态关闭。
- 电机 PWM 在速度为 0 时关闭或进入硬件确认后的最低耗电状态。
- 舵机 PWM 是否停输出必须实测，避免省电导致机械状态失控。
- 电池电压只在低频节拍或 controller 请求时采样。

## 阶段 2 当前节能状态

当前固定链路阶段为了硬件 bring-up，controller 连续发送测试包，receiver 连续轮询 radio。该行为只用于双板通信验证，不代表最终节能策略。

阶段 3 接入 `proto_rf_link` 后必须引入：

- controller 发送周期。
- controller 断联降频。
- receiver 业务包超时。
- receiver SAFE_STATE 低频监听。

## 低功耗实施前置条件

进入 idle/power-down 前必须确认：

- nRF24L01 CE/CSN/SPI 引脚静态电平。
- nRF24L01 是否保持 RX 或进入 power-down。
- 按键、IRQ 或定时器唤醒源。
- 唤醒后 radio 是否需要重新配置。
- 输出安全状态是否会被低功耗模式破坏。
