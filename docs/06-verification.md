# 验证计划

## 本机验证

业务协议测试：

```sh
cc -std=c99 -Wall -Wextra -Ishared -I../Stc8hBase/core tests/toy_remote_protocol_test.c shared/toy_remote_protocol.c -o /tmp/toy_remote_protocol_test
/tmp/toy_remote_protocol_test
```

期望：

- 控制 payload 越界字段被拒绝。
- 安全默认控制值合法且为中性输出。
- 状态 payload 非法小数电压被拒绝。
- 刹车动作归约符合 release、hold speed、clear speed 语义。
- 转向 ADC 归约覆盖 0、中心、满量程、反向和高值限幅。
- 电压 centivolts 归约能拆分整数/小数并对显示范围限幅。

链路集成测试：

```sh
cc -std=c99 -Wall -Wextra -Ishared -I../Stc8hBase/core -I../Stc8hBase/protocols tests/rf_link_integration_test.c shared/toy_remote_protocol.c ../Stc8hBase/protocols/proto_rf_link.c -o /tmp/rf_link_integration_test
/tmp/rf_link_integration_test
```

期望：

- 业务控制 payload 和状态 payload 均能放入 `PROTO_RF_LINK_PAYLOAD_MAX`。
- control payload 能通过 `PROTO_RF_LINK_PACKET_DATA` 往返。
- status payload 能通过 `PROTO_RF_LINK_PACKET_STATUS` 往返。

## 固件构建验证

controller：

```sh
cd controller
pio run
```

receiver：

```sh
cd receiver
pio run
```

期望：

- 两边均编译和链接通过。
- flash 未超过 8KB。
- 链接阶段不出现 DSEG 连续空间不足。

## 阶段 2 硬件验证

固定参数：

- 地址：`TOYR1`
- 频道：40
- payload：32 bytes
- controller：PTX
- receiver：PRX

需要验证：

- controller 在 receiver 上电时出现 `TX_DONE`。
- controller 在 receiver 断电或远离时出现 `MAX_RETRY`。
- receiver 能收到递增 seq。
- receiver 清 `RX_READY` 后可继续收包。
- 断电恢复后双方无需复位即可恢复通信，若不能恢复，需要记录 radio 状态寄存器。
- 连续发送/轮询阶段的电流作为 bring-up 基线，不作为最终功耗目标。

## 阶段 3 验证

- `HELLO` 到 `CONNECTED` 状态转换。
- `DATA` payload 版本、长度、字段范围校验。
- `STATUS` 回传链路状态和电压。
- controller 断联降频。
- receiver 超时进入安全状态。

## 阶段 4 验证

controller：

- EC11 速度增减。
- 方向、刹车、灯、蜂鸣器按键。
- 转向 ADC 映射。
- TM1637 内容变化刷新。

receiver：

- 电机速度和方向。
- 刹车/滑行策略。
- 舵机角度和限幅。
- 灯、蜂鸣器、aux PWM 安全状态。
- 电池电压采样和回传。
