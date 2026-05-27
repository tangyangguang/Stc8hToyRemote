# 验证方法

## 全量检查

```sh
./tools/check_all.sh
```

该脚本覆盖：

- host C 单元测试。
- `proto_rf_link` 集成测试。
- controller 和 receiver 正常固件构建。
- 固件尺寸和 internal RAM 余量阈值检查。
- controller/receiver 诊断固件构建。
- 关键 ISR 和 nRF24 pin codegen 检查。

固件资源基线：

| 固件 | Flash | Flash 阈值 | Stack 可用 | 最大连续 internal RAM |
| --- | --- | --- | --- | --- |
| controller | 7788/8192 | <= 7950 | >= 135 bytes | >= 0 bytes |
| receiver | 6902/8192 | <= 6904 | >= 145 bytes | >= 2 bytes |

## 单独构建

controller：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/controller
pio run
```

receiver：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run
```

期望：

- 编译和链接通过。
- flash、stack 可用空间和最大连续 internal RAM 不低于阈值。

## 烧录

controller：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/controller
pio run -t upload --upload-port /dev/cu.usbserial-110
```

receiver：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run -t upload --upload-port /dev/cu.usbserial-120
```

项目使用 `stcgal` 的 `stc8g` 协议和自动重试上传脚本。烧录失败时先确认串口号、上电时机、协议族和 USB-UART 连接，再调整下载波特率或握手参数。

## 正常链路验收

1. 先打开 receiver，再打开 controller。
2. controller 显示 `C076` 后进入正常控制界面。
3. receiver LED 常亮。
4. 关闭 receiver 后，controller 显示 `L076`。
5. 再打开 receiver 后，controller 自动恢复正常控制界面。
6. 只有在 `L076` 下双击 EC11 中键才进入扫描。

失败判定：

- controller 长时间停在 `Cxxx` 或 `Lxxx`：检查频道、绑定、receiver 上电和 nRF24。
- controller 显示 `E001`：检查 controller nRF24 初始化、供电和接线。
- receiver LED 不按生命周期闪烁：检查 receiver 启动、nRF24 初始化和配置。

## 控制验收

controller：

- EC11 旋转改变速度，显示 `00..99/A0`。
- EC11 中键短按刹车并清零速度。
- 单独刹车键只刹车，不清速度。
- 方向键改变前进/后退。
- 灯光和蜂鸣器按键能控制 receiver 输出。
- Fn 显示电压。
- 长按 EC11 中键 5 秒进入配置模式。

receiver：

- 速度小于 5 时电机不转。
- 速度等于 5 时电机 duty 为 `APP_OUTPUT_MOTOR_MIN_DUTY_PERCENT` 对应的最低占空比，默认 35%。
- 速度 100 时 duty 为 100%。
- 前进时 P3.3/PWM7 输出，P3.4/PWM8 为 0。
- 后退时 P3.4/PWM8 输出，P3.3/PWM7 为 0。
- 刹车时 P3.3/PWM7 和 P3.4/PWM8 同时 100%。
- 掉线后 AT8236 duty、灯、蜂鸣器和辅助 PWM 均关闭，舵机回中。
- AT8236 PWM-B 频率默认约 20.03kHz；若修改 `APP_OUTPUT_FAST_PWM_PRESCALER` / `APP_OUTPUT_FAST_PWM_PERIOD`，需重新核对低速起步、噪声和温升。

## 诊断固件

接收机电机 PWM 扫描：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run -c platformio_diag.ini -e STC8H1K08_motor_diag -t upload --upload-port /dev/cu.usbserial-120
```

接收机 RF 控制到 PWM 输出路径：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run -c platformio_diag.ini -e STC8H1K08_control_diag -t upload --upload-port /dev/cu.usbserial-120
```

接收机 RF 控制到 P3.3/P3.4 直控路径：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run -c platformio_diag.ini -e STC8H1K08_control_gpio_diag -t upload --upload-port /dev/cu.usbserial-120
```

controller radio 诊断：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/controller
pio run -c platformio_diag.ini -e STC8H1K08_radio_diag -t upload --upload-port /dev/cu.usbserial-110
```

诊断使用原则：

- `control_gpio_diag` 只验证 RF 控制包和 P3.3/P3.4 硬件链路，不验证调速曲线。
- `control_diag` 验证 RF 控制包、解包和 PWM 输出路径。
- `motor_diag` 不依赖 RF，用于独立确认 PWM 和电机驱动。

## 配置验收

- 正常模式 EC11 中键长按 5 秒进入配置模式。
- 配置模式显示 `P1/P2/P3/P4` 和配置值。
- 配置模式短按切换配置项。
- 配置模式旋转只改选中的配置项。
- 舵机相关配置实时作用。
- 配置模式长按 3 秒保存并退出。
- 断电重启不会保存未确认的 RAM 草稿。

## 多接收机和频道验收

默认固件关闭 receiver P30/P31 频道维护键。若启用 `APP_RECEIVER_ENABLE_CHANNEL_BUTTONS=1`：

- P30/P31 在预设频道池中切换并保存。
- 切换后 receiver 输出保持安全。
- controller 需要在 `Lxxx` 下双击手动扫描找到新频道。
- 已连接状态下不做协商换频；误切频道按丢链处理。

默认固件保留 P30+P31 上电清绑定。验收时同时按住 P30/P31 再给 receiver 上电，LED 应快闪 6 次并进入未绑定等待，频道保持默认策略不变。
