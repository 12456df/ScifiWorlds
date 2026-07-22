# SciFi Worlds Repository Guidance

## Project SSOT

- 项目立项、游戏设计、技术设计与生产计划以 `Docs/` 为准。
- UE 工程事实以 `.agents/ue-project-context.md` 为准。
- 跨系统工程约束以 `Docs/Engineering/TechnicalStandards.md` 为准。
- 功能模块的开发顺序、依赖、完成门槛和提交记录以 `Docs/07_DevelopmentRoadmap.md` 为准。
- 尚未决定的项目事实必须标记为 `TBD`，不得推测填充。
- 方向性技术决策记录在 `Docs/ADR/`；具体系统设计记录在 `Docs/Systems/`。

## Implementation Rules

- 本项目由独立开发者维护；优先交付可运行、可测试的功能，同时保持必要技术规范，避免无收益的流程负担。
- 功能顺序按路线图依赖执行，不维护额外的 Must/Should/Could 优先级矩阵。
- 新系统按“问题 → 需求 → 子系统 → 契约 → 数据流 → 实现顺序 → 验证”完成设计后再进入生产实现。
- 项目采用 GAS + 多人联机 + Dedicated Server 目标，并按服务器权威设计。
- C++ 负责稳定框架、GAS/网络契约、权威规则、性能敏感和可测试逻辑；蓝图负责内容配置、组装与表现，具体边界以技术开发规范、TDD 和 ADR 为准。
- 可调数值必须数据驱动；运行时状态必须有唯一数据所有者。
- 系统依赖必须显式且尽量单向；禁止跨系统直接写入他人数据、隐藏全局耦合和循环依赖。
- 保持 Runtime 代码与 Editor-only 依赖隔离。
- 修改实现时同步更新受影响文档。

## Technical Reference Order

1. 当前 ScifiWorlds 源码与仓库 SSOT。
2. Epic 官方 UE 5.7 文档与本机 UE 5.7 引擎源码。
3. `E:/Unreal Projects/Aura/Source/Aura/` 作为实现模式参考。
4. Aura `.docs` 下的 GAS 与多人资料作为补充参考。

- Aura 不是本项目依赖。引用 Aura 前先读其 `AGENTS.md` 与 `.agents/ue-project-context.md`，并按 ScifiWorlds 的所有权、生命周期和网络模型重新验证。
- Aura/第三方资料与 UE 5.7 官方行为冲突时，以 Epic UE 5.7 文档或引擎源码为准。

## Repository Rules

- `main` 必须保持可构建；功能通过短生命周期分支和 Pull Request 合并。
- `.uasset`、`.umap` 必须由 Git LFS 管理。
- 禁止提交 Binaries、Intermediate、Saved、DerivedDataCache 或 IDE 本地设置。
- 商业/Fab/Marketplace 原始资产只能存在于授权的私有协作环境，不得公开分发。
