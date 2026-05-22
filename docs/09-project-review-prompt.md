# 项目整体评审提示词

下面提示词用于让另一个高能力模型检查和评估本项目的代码、文档、硬件方案和验证方案。

```text
你是资深 STC8H/8051 嵌入式工程师和固件架构评审专家。请评审本地仓库：

/Users/tyg/dir/codex_dir/Stc8hToyRemote

项目背景：
- 硬件是 STC8H1K08 + nRF24L01 玩具遥控器和接收机。
- 相邻基础库路径是 /Users/tyg/dir/codex_dir/Stc8hBase。
- 应用项目必须引用 Stc8hBase，不复制基础库源码，不复用 legacy 旧驱动。
- legacy/ 只读，只能作为硬件接线、需求和可观察行为参考。
- 当前目标是评估最终方案质量，以最终版文档和代码为准。

请先阅读：
1. AGENTS.md
2. README.md
3. docs/01-requirements.md
4. docs/02-architecture.md
5. docs/03-protocol.md
6. docs/04-logic-flows.md
7. docs/05-power.md
8. docs/06-verification.md
9. docs/07-tx-v2-notes-assessment.md
10. docs/08-hardware-map.md
11. EC11_NOTES.md
12. STCGAL_UPLOAD_NOTES.md

重点评审范围：
- controller 和 receiver 的 nRF24L01 初始化、PTX/PRX 切换、ACK payload 策略、MAX_RT 恢复和频道/绑定逻辑。
- ToyRemote 业务协议是否边界清楚，payload 是否手工按字节打包，版本、长度、字段范围和 tx_id 校验是否完整。
- receiver 掉线安全态是否覆盖电机、舵机、灯、蜂鸣器和辅助 PWM。
- AT8236 输出策略是否合理：P3.3/PWM7、P3.4/PWM8、刹车、安全态、最低有效 duty 和速度曲线。
- controller 输入链路是否可靠：EC11 Timer0 ISR、按键语义、转向 ADC、配置模式和显示生命周期。
- EEPROM fixed-block 配置是否有版本、默认值、校验、旧配置失效策略和误操作保护。
- STC8H1K08 8KB flash、1.25KB RAM、SDCC 参数区和 PlatformIO 裁剪宏是否合理。
- 诊断固件、测试脚本和 docs/06-verification.md 是否足以定位 RF、PWM、电机、输入和烧录问题。
- 文档是否只描述最终版本，是否还有过期配置或与代码不一致的说明。

请执行：
1. 使用 rg/rg --files 快速定位相关代码和文档。
2. 只读评审，不要先改代码。
3. 运行 ./tools/check_all.sh，并记录关键结果。
4. 如发现问题，按严重程度排序，给出文件和行号。
5. 对每个问题说明：现象/风险、依据、建议修复方向、是否必须立即修。
6. 明确区分代码 bug、设计风险、文档不一致、测试缺口和可后续优化。
7. 如果没有发现阻塞问题，也要说明剩余风险和建议的实机验证项目。

输出格式：
- 先列 Findings，按 P0/P1/P2/P3 排序。
- 然后列 Open Questions。
- 然后列 Verification，包含实际运行命令和结果。
- 最后给出 Overall Assessment，判断是否适合继续实机使用、是否需要先修复阻塞问题。

限制：
- 不要修改 /Users/tyg/dir/codex_dir/Stc8hBase。
- 不要修改 legacy/。
- 不要因为旧项目做法而要求兼容旧 API、旧 EEPROM 或旧无线格式。
- 不要提出会显著增加 flash 的方案，除非同时给出裁剪替代方案。
```
