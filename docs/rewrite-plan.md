# 重写计划

## 1. 原则

- 旧项目只看需求和场景，不继承技术方案。
- 不兼容旧 API，不保留 `ifnnrf_*`、`TxPacket`、`RxPacket` 等旧命名。
- 业务协议放 `shared/`，基础库只提供硬件能力和通用链路能力。
- 所有无线 payload 手工按字节打包，避免跨编译器结构体布局问题。

## 2. 阶段

### 阶段 1：最小骨架

- `controller` 和 `receiver` 都能用 PlatformIO 编译。
- 两边都能 include `stc8h_config.h`、`drv_nrf24l01.h`、`proto_rf_link.h`。
- `shared/toy_remote_protocol` 能被两个固件共同引用。

### 阶段 2：nRF24L01 双板通信

- 使用固定地址、固定频道、固定 payload。
- 验证 `TX_DONE`、`MAX_RETRY` 和断开恢复。
- 暂不接入业务输入输出。

### 阶段 3：接入 `proto_rf_link`

- 使用 32 字节链路包。
- 先实现 `HELLO`、`DATA`、`STATUS`。
- 明确掉线超时和 receiver 安全状态。

### 阶段 4：迁移业务

- controller 迁移摇杆、EC11、按键、TM1637 显示。
- receiver 迁移电机、舵机、灯、蜂鸣器、电压采样。
- 旧业务字段重设计为新的 `toy_remote_protocol` payload。

## 3. 暂不做

- 不做频道扫描。
- 不做绑定持久化。
- 不做加密、mesh、路由、多节点调度。
- 不做旧项目 API 兼容层。
