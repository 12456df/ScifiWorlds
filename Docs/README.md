# SciFi Worlds 项目文档

本目录是项目设计、技术与生产管理文档的单一事实来源（SSOT）。当实现与文档不一致时，应在同一次变更中更新对应文档，或记录一条架构决策（ADR）。

## 建议补充顺序

1. [项目立项章程](00_ProjectCharter.md)
2. [游戏愿景](01_GameVision.md)
3. [游戏设计文档](02_GDD.md)
4. [技术设计文档](03_TDD.md)
5. [技术开发规范](Engineering/TechnicalStandards.md)
6. [美术规范](04_ArtBible.md)
7. [生产计划](05_ProductionPlan.md)
8. [模块开发路线图](07_DevelopmentRoadmap.md)
9. [风险登记册](06_RiskRegister.md)
10. [架构决策记录](ADR/README.md)
11. [具体系统设计](Systems/README.md)

## 文档状态

每份文档顶部使用以下状态之一：

- `Draft`：正在讨论，不作为实现依据。
- `In Review`：等待评审，原则上不扩大实现范围。
- `Approved`：已批准，是当前实现依据。
- `Superseded`：已被其他文档或决策替代。

## 维护规则

- 所有需求应具有唯一编号和可验证的验收标准。
- 未确定的信息写为 `TBD`，不得通过猜测补全。
- 系统设计必须先明确数据所有权、依赖关系和公开契约，再进入实现。
- 范围、平台、联网模型或核心技术路线的改变必须记录 ADR。
- 文档与对应实现通过同一个 Pull Request 提交。
