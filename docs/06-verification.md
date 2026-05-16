# 验证计划

## 本机验证

全量检查入口：

```sh
./tools/check_all.sh
```

该脚本会运行业务协议测试、链路集成测试、controller 构建和 receiver 构建。下面的命令用于单独定位问题。

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
- controller 默认频道：40，可通过 `APP_DEFAULT_RF_CHANNEL` 编译期宏修改
- receiver 保存频道：默认 40，可用 P30/P31 调整并保存
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
- 多接收机测试：把不同 receiver 调到不同频道后，controller 能扫描到目标频道。
- 绑定测试：receiver 未绑定时接受第一个合法 `tx_id`；绑定后丢弃其他 `tx_id`；P30+P31 上电清除绑定。

## 阶段 3 验证

- `DATA` payload 版本、长度、字段范围校验。
- `STATUS` 经 nRF24 ACK payload 回传链路状态和电压。
- `STATUS` ACK 中的 `tx_id` 必须与 controller 的 `APP_TX_ID` 匹配。
- controller 约 50ms 发送节拍。
- receiver 超时进入安全状态。

## 阶段 4 验证

controller：

- EC11 速度增减。
- 方向、刹车、灯、蜂鸣器按键。
- 转向 ADC 映射。
- TM1637 正常状态和 Fn 电压显示。

receiver：

- 电机速度和方向。
- 刹车/滑行策略。
- 舵机角度和限幅。
- 灯、蜂鸣器、aux PWM 安全状态。
- 电池电压采样和回传。

## 当前构建结果

2026-05-16 本机运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `7226/8192`
- receiver: flash `6644/8192`
- 两端均无 DSEG/OSEG 链接错误。

2026-05-16 加入简单频道扫描和 receiver 绑定后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `7622/8192`
- receiver: flash `8005/8192`
- 两端均无 DSEG/OSEG 链接错误。

2026-05-16 切换基础库 fixed-block EEPROM，并恢复 controller 保存 `tx_id/last_channel` 后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `8164/8192`
- receiver: flash `7791/8192`
- 两端均无 DSEG/OSEG 链接错误。

2026-05-16 固件尺寸优化后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `7539/8192`，剩余 `653` bytes
- receiver: flash `7089/8192`，剩余 `1103` bytes
- 两端均无 DSEG/OSEG 链接错误。
- 保留功能：控制输入、nRF24 通信、ACK 状态回传、电压显示/回传、频道扫描、receiver 绑定、EEPROM 配置、receiver 掉线安全态、电机/舵机/灯/蜂鸣器/aux PWM 输出。
