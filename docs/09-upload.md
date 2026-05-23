# STCGAL 烧录说明

本项目使用 `stcgal` 通过 STC8H UART ISP 烧录 controller 和 receiver。烧录脚本封装在各自项目的 `upload_stcgal.py` 中，实际重试逻辑位于 `tools/upload_stcgal_runner.py`。

## 默认配置

| 固件 | 默认串口 | 默认下载波特率 | 协议 |
| --- | --- | --- | --- |
| controller | `/dev/cu.usbserial-110` | `38400` | `stc8g` |
| receiver | `/dev/cu.usbserial-120` | `19200` | `stc8g` |

配置位置：

- `controller/platformio.ini`
- `receiver/platformio.ini`

默认不启用 RC trim，不强制指定握手波特率。

## 常用命令

烧录 controller：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/controller
pio run -t upload --upload-port /dev/cu.usbserial-110
```

烧录 receiver：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run -t upload --upload-port /dev/cu.usbserial-120
```

构建后不烧录：

```sh
cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/controller
pio run

cd /Users/tyg/dir/codex_dir/Stc8hToyRemote/receiver
pio run
```

## 上传器回退策略

上传器按以下顺序尝试：

1. 配置波特率，重复 `custom_stcgal_attempts` 次。
2. 配置波特率 + `-l 1200`。
3. `9600`。
4. `9600 + -l 1200`。
5. `4800 + -l 1200`。

每次尝试都会完整执行 `stcgal -P stc8g -p <port> -a -b <baud> <hex>`。只有本次尝试失败后才进入下一个回退参数。

## 失败排查

常见失败信息包括：

- `Protocol error: incorrect frame start`
- `Target frequency: Disconnected!`
- `Finishing write: Disconnected!`

排查顺序：

1. 确认串口号：controller 用 `/dev/cu.usbserial-110`，receiver 用 `/dev/cu.usbserial-120`。
2. 确认协议仍为 `stc8g`。
3. 确认板子进入 ISP 时机正确，下载前后电源循环干净。
4. 保持默认不启用 RC trim。
5. 先使用项目内置上传器，不直接绕过重试脚本。
6. 仍不稳定时，再按需调整 `custom_stcgal_baud` 或增加握手波特率回退。

如果出现偶发成功、频繁失败，优先检查 USB-UART、供电、RX/TX 接线和上电时机，再判断固件或应用逻辑问题。
