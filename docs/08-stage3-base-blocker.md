# 阶段 3 基础库阻塞

## 背景

阶段 3 计划把 `proto_rf_link` 和 `shared/toy_remote_protocol` 接入 controller/receiver 固件，让 32 字节固定测试包升级为链路层 `HELLO`、`DATA`、`STATUS`。

当前阶段 2 固件只编译实际使用模块：

- `drv_nrf24l01`
- `stc8h_spi`
- `app_radio`

该组合在 STC8H1K08 上可以通过 PlatformIO/SDCC 构建。

## 复现方式

在 controller 和 receiver 的 `src/` 下临时加入 wrapper：

```c
#include "../../../Stc8hBase/protocols/proto_rf_link.c"
```

以及：

```c
#include "../../shared/toy_remote_protocol.c"
```

然后分别运行：

```sh
(cd controller && pio run)
(cd receiver && pio run)
```

## 结果

controller 和 receiver 均在链接阶段失败：

```text
?ASlink-Error-Could not get 49 consecutive bytes in internal RAM for area DSEG.
```

撤掉这两个 wrapper 后，`./tools/check_all.sh` 通过：

```text
controller flash: 1694 / 8192 bytes
receiver flash: 1585 / 8192 bytes
```

## 判断

这是阶段 3 的真实阻塞点。当前应用项目不应为了绕过该问题复制或改写 `proto_rf_link`，否则会破坏“公共硬件和通用链路能力优先使用 `Stc8hBase`”的边界。

问题更可能属于基础库/构建集成层：

- PlatformIO wrapper 会把整个 `.c` 编进固件。
- SDCC 会为被编译进目标的函数分配 DSEG 参数区，即使当前应用未调用所有 API。
- STC8H1K08 内部 RAM 很小，`proto_rf_link` 和业务协议一起进入固件后 DSEG 连续空间不足。

## 给 Stc8hBase 项目的提示词

```text
背景：
我在 Stc8hToyRemote 项目中准备把 `Stc8hBase/protocols/proto_rf_link` 接入 STC8H1K08 controller/receiver 固件。当前阶段 2 固件只编译 `drv_nrf24l01`、`stc8h_spi` 和应用侧 `app_radio`，controller/receiver 都能通过 PlatformIO/SDCC 构建。

目标：
让 `proto_rf_link` 能以适合 8KB flash、1.25KB RAM 的 STC8H1K08 项目方式接入应用固件，同时保持基础库边界：应用项目不复制、不改写通用链路层。

现状问题：
在应用项目的 PlatformIO wrapper 中加入：

`#include "../../../Stc8hBase/protocols/proto_rf_link.c"`

并加入业务协议 wrapper：

`#include "../../shared/toy_remote_protocol.c"`

之后，controller 和 receiver 链接均失败：

`?ASlink-Error-Could not get 49 consecutive bytes in internal RAM for area DSEG.`

撤掉 wrapper 后，`./tools/check_all.sh` 通过，controller flash 为 1694/8192 bytes，receiver flash 为 1585/8192 bytes。

复现方式：
1. 在 Stc8hToyRemote 的 controller/src 和 receiver/src 临时加入 proto_rf_link wrapper 和 toy_remote_protocol wrapper。
2. 分别执行 `(cd controller && pio run)`、`(cd receiver && pio run)`。
3. 观察 DSEG 连续空间不足错误。

影响范围：
这会阻塞所有 STC8H1K08 小内存应用使用 `proto_rf_link`。应用项目如果临时复制/裁剪链路层，会破坏基础库复用边界并产生长期维护负担。

请在 Stc8hBase 项目中评估并完善适合 SDCC/8051 小内存目标的 `proto_rf_link` 接入方式。重点是减少未使用 API 对 DSEG 的影响，或提供更可裁剪的构建/模块组织方式。除非有充分把握，不需要预设具体实现方案；请先复现、分析 map/mem 输出，再决定最佳修改路径。
```
