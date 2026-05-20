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

## 固件上传验证

controller：

```sh
cd controller
pio run -t upload --upload-port <serial-port>
```

receiver：

```sh
cd receiver
pio run -t upload --upload-port <serial-port>
```

`<serial-port>` 示例：`/dev/cu.usbserial-110`。当前上传配置仿照 `Stc8hIrLamp` 的已验证流程，使用 PlatformIO 内置 `tool-stcgal`，协议 `stc8g`，默认下载波特率 `38400`，并通过 `custom_stcgal_trim = 11059` 把 IRC 设置到约 11.059MHz。若个别旧板在 `Target frequency` 或 `Finishing write` 阶段掉帧，可临时加 `--project-option custom_stcgal_baud=9600` 复测。

已知不适用路径：

- PlatformIO `intel_mcs51` 默认 uploader 对 `STC8H1K08` 使用 `stc8` 协议，本项目实测会在 `Erasing flash` 阶段出现 `Protocol error: incorrect frame start`。
- 手动调用 `stcgal` 时必须使用 `-P stc8g -t 11059 -a -b 38400` 这一组默认参数；不稳定旧板可临时降到 `-b 9600`。

## 阶段 2 硬件验证

固定参数：

- 地址：`TOYR1`
- controller 默认频道：76，可通过 `APP_DEFAULT_RF_CHANNEL` 编译期宏修改
- receiver 默认频道：76；默认构建关闭 `APP_RECEIVER_ENABLE_CHANNEL_BUTTONS`，运行时固定使用默认频道，不读取 EEPROM 保存频道作为 RF 运行频道
- receiver 保存频道：启用 `APP_RECEIVER_ENABLE_CHANNEL_BUTTONS=1` 后，可用 P30/P31 在预设频道池中调整并保存
- RF：1Mbps、0dBm
- control payload：32 bytes nRF24 packet
- ACK status payload：15 bytes dynamic ACK payload
- controller：PTX
- receiver：PRX

需要验证：

- controller 在 receiver 上电时出现 `TX_DONE`。
- controller 在 receiver 断电或远离时出现 `MAX_RETRY`。
- controller 上电后先显示保存频道 `Cxxx`。
- controller 保存频道连不上时显示 `Lxxx`，持续锁定当前频道重试，不自动扫频。
- controller 显示 `Lxxx` 后，EC11 中键双击才启动扫描；未显示 `Lxxx` 时双击不启动扫描。
- controller 手动扫描时显示 `S000..S125`，持续扫描直到找到匹配 receiver。
- controller 找到频道后显示 `Fxxx` 短暂停留，保存频道，再进入控制界面。
- controller 已连接后连续失败超过阈值时显示 `Lxxx` 并锁定当前频道重试，不自动扫频。
- controller nRF24 初始化失败时显示 `E001` 常驻。
- controller 正常控制界面中，冒号灭表示最近通信正常，冒号快闪表示当前频道短时发送失败或 ACK 缺失；不使用冒号常亮表示故障。
- receiver 能收到递增 seq。
- receiver 清 `RX_READY` 后可继续收包。
- 断电恢复后双方无需复位即可恢复通信，若不能恢复，需要记录 radio 状态寄存器。
- 连续发送/轮询阶段的电流作为 bring-up 基线，不作为最终功耗目标。
- 多接收机测试：把不同 receiver 调到不同频道后，controller 能扫描到目标频道。
- 预设频道池测试：P30/P31 按 `76, 72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16` 循环切换并保存。
- 已连接换频测试：暂缓。`Hxxx` 协商换频需要扩展控制/状态 payload，当前 8KB 版本不实现；若误在已连接状态切换 receiver 频道，应按丢链处理，controller 显示 `Lxxx` 后手动扫描恢复。
- 绑定测试：receiver 未绑定时接受第一个合法 `tx_id`；绑定后丢弃其他 `tx_id`；P30+P31 上电清除绑定。
- receiver LED：上电启动快闪 3 次；nRF24 错误为双闪长停顿；未绑定为单短闪长停顿；已绑定未连接为慢闪；已连接为常亮；清除绑定为快闪 6 次。

## 阶段 3 验证

- `DATA` payload 版本、长度、字段范围校验。
- `STATUS` 经 nRF24 ACK payload 回传链路状态和电压。
- `STATUS` ACK 中的 `tx_id` 必须与 controller 的 `APP_TX_ID` 匹配。
- controller 约 50ms 发送节拍。
- receiver 超时进入安全状态。

## 阶段 4 验证

controller：

- EC11 速度增减。
- 正常模式 EC11 中键短按：刹车并清零速度。
- 正常模式 EC11 中键长按 5 秒：到 5 秒立即进入配置模式，不需要松开。
- 方向、刹车、灯、蜂鸣器按键。
- 转向 ADC 映射。
- TM1637 正常状态和 Fn 电压显示。
- 配置模式显示 `P1:0/1`、`P2:0/1`、`P3:45`、`P4:20` 这类 `P` 前缀格式，冒号固定作为配置项和值的分隔符。
- 配置模式 EC11 中键短按按 `P1 -> P2 -> P3 -> P4 -> P1` 顺序切换配置项。
- 配置模式 EC11 旋转只修改当前配置草稿，且舵机相关配置实时作用。
- 配置模式 EC11 中键长按超过 3 秒后保存 EEPROM 并退出；断电重启放弃未保存草稿。
- 配置模式下发包保持安全：速度 0、刹车、灯/蜂鸣器/aux 关闭。
- 配置保存后重新上电，方向反向、舵机反向、舵机中位和舵机端点收缩均保持。

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

2026-05-16 controller 配置/校准模式补齐后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `8104/8192`，剩余 `88` bytes
- receiver: flash `7089/8192`，剩余 `1103` bytes
- 两端均无 DSEG/OSEG 链接错误。
- 新增保留功能：controller 配置模式、舵机反向、方向反向、舵机中位、端点收缩和 EEPROM 保存。

2026-05-16 controller 无功能损失压缩后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `7878/8192`，剩余 `314` bytes
- receiver: flash `7089/8192`，剩余 `1103` bytes
- 两端均无 DSEG/OSEG 链接错误。
- 压缩方式只消除内部不可达检查、固定参数和重复显示分支，不删除频道扫描、配置模式、ACK、电压显示或掉线安全功能。

2026-05-16 第二轮无功能损失压缩后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `7825/8192`，剩余 `367` bytes
- receiver: flash `6482/8192`，剩余 `1710` bytes
- 两端均无 DSEG/OSEG 链接错误。
- 新增压缩方式：关闭未用 TM1637 clear/encode API、应用侧固定数字编码、receiver 去掉解包后重复校验和固定参数检查。

2026-05-16 接入 Stc8hBase fixed-path 裁剪后再次运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `6663/8192`，剩余 `1529` bytes
- receiver: flash `6019/8192`，剩余 `2173` bytes
- 两端均无 DSEG/OSEG 链接错误。
- 使用基础库 fixed-path 能力：TM1637 `display_raw4`、EC11 small API、proto_rf_link fixed DATA send/poll、nRF24 pipe0 fixed 配置和参数检查裁剪。

2026-05-17 第二轮 base lib 重构（NULL gate + RAM 字段 gate + PWM 影子值 gate）后运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `6484/8192`，剩余 `1708` bytes（较上一次 -27 bytes）；XDATA `115/1024`（较上一次 -4 bytes）
- receiver: flash `5518/8192`，剩余 `2674` bytes（较上一次 -176 bytes）；XDATA `112/1024`（较上一次 -4 bytes）；DATA 栈空间 +6 bytes
- 两端均无 DSEG/OSEG 链接错误。
- 新增基础库默认兼容裁剪宏：
  - `proto_rf_link` 增加 `INCLUDE_TIMEOUT_FIELDS`，关闭后从 `proto_rf_link_t` 中省 `timeout_ms`/`heartbeat_ms` 共 4 字节 RAM；自动 `#error` 排除与 TICK/INIT_TIMEOUT_FIELDS/SEND_HEARTBEAT 冲突的配置。
  - `proto_rf_link_init` 与 `proto_rf_link_set_ids` 的 NULL 检查改为复用 `ENABLE_PACKET_ARG_CHECK` 宏。
  - `drv_ec11` 增加 `ENABLE_NULL_CHECK`，关闭后省 init / scan / get_delta 的 `ec11 == NULL` 检查。
  - `stc8h_pwm` 增加 `TRACK_PERIOD_PRESCALER`，关闭后省 4–8 字节 RAM 影子值，set_period / set_prescaler 的"运行中防改"运行期保护一并移除；编译期 `#error` 强制 `SET_DUTY_CLAMP=0`。
- ToyRemote 接入：
  - controller: `PROTO_RF_LINK_INCLUDE_TIMEOUT_FIELDS=0`、`DRV_EC11_ENABLE_NULL_CHECK=0`。
  - receiver: `PROTO_RF_LINK_INCLUDE_TIMEOUT_FIELDS=0`、`STC8H_PWM_TRACK_PERIOD_PRESCALER=0`。
- 验收要点：
  - controller / receiver 均无对 `link.timeout_ms` / `link.heartbeat_ms` 的读取，仅 controller 在 `handle_ack_status` 中写 `link.ack_pending=0`（仍允许）；安全态、ACK 状态、电压回传、配置/校准、绑定均行为不变。
  - receiver 安全态与控制态的 PWM 占空比、舵机中点、电机方向、灯/蜂鸣器/aux PWM 输出与上一次一致。
  - 业务协议测试和链路集成测试均通过。

2026-05-17 接入最新基础库 fixed-state / SPI 裁剪后运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `6302/8192`，剩余 `1890` bytes（较上一次 -182 bytes）；XDATA `112/1024`（较上一次 -3 bytes）；DATA 栈空间 +1 byte
- receiver: flash `5363/8192`，剩余 `2829` bytes（较上一次 -155 bytes）；XDATA `109/1024`（较上一次 -3 bytes）；DATA 最大连续空闲 +1 byte
- 两端均无 DSEG/OSEG 链接错误。
- 新增接入：
  - controller / receiver: `PROTO_RF_LINK_TRACK_STATE=0`、`PROTO_RF_LINK_TRACK_SEQ_RX=0`、`PROTO_RF_LINK_TRACK_ACK_PENDING=0`、`STC8H_SPI_ENABLE_WRITE=0`。
  - controller: `DRV_NRF24L01_ENABLE_ENTER_RX=0`、`DRV_NRF24L01_ENABLE_ENTER_STANDBY=0`。
  - receiver: `DRV_NRF24L01_ENABLE_ENTER_STANDBY=0`。
- 行为保持：
  - controller 不再写未读取的 `link.seq_rx` / `link.ack_pending` / `link.state`。
  - receiver ACK status 的 `ack_seq` 直接回填当前收到 DATA 包的 `packet[3]`，不再依赖 `proto_rf_link_t.seq_rx`。
  - `READ_STATUS`、`READ_PAYLOAD`、`ACK_PAYLOAD`、`POWER_DOWN` 保留，因为当前收发、ACK payload 和初始化路径仍依赖它们。

2026-05-17 fixed-path 再次压缩后运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `6511/8192`，剩余 `1681` bytes（较上一次 -152 bytes）
- receiver: flash `5694/8192`，剩余 `2498` bytes（较上一次 -325 bytes）
- 两端均无 DSEG/OSEG 链接错误。
- 新增基础库默认兼容裁剪宏：
  - `proto_rf_link` 增加 `ENABLE_SEND_DATA_FIXED_TRACK_ACK` / `ENABLE_POLL_DATA_FIXED_TRACK_LINK`，可关闭 fixed 路径里对 `ack_pending`、`timeout_ms`、`state` 的额外写入。
  - `stc8h_pwm` 增加 `ENABLE_SET_DUTY_CHANNEL_CHECK` / `ENABLE_SET_DUTY_CLAMP`，可在 fixed 通道、已验证范围的应用里跳过运行期通道掩码校验和占空比上限。
  - `drv_nrf24l01_check_present` 内部改为查表循环，pattern 写读各 5 字节共用单一表。
- ToyRemote 接入与重构：
  - controller：TM1637 brightness 固定为 1（关闭 brightness state 变量并去掉 `set_brightness(1u)` 调用）。
  - controller：`display_voltage` 改成 4 次 `divmod10` 循环 + leading-zero 抹零，仍输出 `x.yyy` 与冒号策略。
  - controller：`handle_ack_status` 缓存 `body = ack + PROTO_RF_LINK_HEADER_SIZE`，去掉重复地址计算。
  - receiver：`app_outputs_apply_control` 三段电机分支合并为先算 `fwd/rev` 再统一 `set_duty`。
  - receiver：抽 `app_outputs_write_pwm(servo, aux, fwd, rev)` 共享 4 路 PWM 写入，被 `apply_safe` 和 `apply_control` 共用。
  - receiver：`app_status_update` 改用一次 `/100u + 减乘` 计算 voltage int/dec，去掉重复 `__moduint` 调用。
  - receiver：启用 `STC8H_PWM_ENABLE_SET_DUTY_CHANNEL_CHECK=0` / `STC8H_PWM_ENABLE_SET_DUTY_CLAMP=0` 与 `PROTO_RF_LINK_ENABLE_POLL_DATA_FIXED_TRACK_LINK=0`。
  - controller 启用 `PROTO_RF_LINK_ENABLE_SEND_DATA_FIXED_TRACK_ACK=0`。
- 验收要点：
  - controller TM1637 显示档位、电压、配置档位与冒号位置不变。
  - controller `handle_ack_status` 仍校验 magic/version/type/local_id/peer_id/length/tx_id/voltage int 和 dec 上限。
  - receiver 安全态仍把 PWM6/7/8 拉到 0、舵机回中，并关电机/灯/蜂鸣器/LED。
  - receiver 控制态仍按 brake/speed/direction 三态分配 fwd/rev PWM。
- 业务协议测试和链路集成测试均通过；`./tools/check_all.sh` 全程绿色。

2026-05-19 controller/receiver 生命周期交互补齐后运行：

```sh
./tools/check_all.sh
```

结果：

- controller: flash `8105/8192`，剩余 `87` bytes
- receiver: flash `6904/8192`，剩余 `1288` bytes
- 两端均无 DSEG/OSEG 链接错误。
- 已实现：默认频道 `76`、预设频道池、controller `Cxxx/Lxxx/Sxxx/Fxxx/E001/Pn:value` 显示、`Lxxx` 下双击手动扫描、配置模式 RAM 草稿和长按保存、receiver 生命周期 LED。
- 暂缓：已连接状态协商换频 `Hxxx`。controller 剩余 flash 只有 `87` bytes，本轮不扩展无线 payload。
