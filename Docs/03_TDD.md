# 技术设计文档（TDD）

**状态：** Draft
**最后更新：** 2026-07-20

## 技术目标

- 引擎：Unreal Engine 5.7
- 项目模块：`PolygonScifiWorlds`（Runtime）
- 构建目标：Game、Editor
- 核心玩法框架：Gameplay Ability System（GAS）
- 开发模式：C++ 与蓝图混合开发
- 目标平台：TBD
- 性能目标：TBD

## 技术原则

- 契约优先：公开接口与数据结构先于实现。
- 单一职责：每个系统拥有明确且唯一的职责。
- 显式依赖：禁止通过隐式全局状态建立系统耦合。
- 数据驱动：可调参数存放在 Data Asset、Data Table 或配置中。
- 单一数据所有者：任何运行时状态只能由一个系统负责写入。
- 工程质量：以健壮性、可扩展性、可读性和可维护性为验收属性，避免过度耦合。
- 完整规范：[ScifiWorlds 技术开发规范](Engineering/TechnicalStandards.md)

## C++ 与蓝图边界

- C++：基础框架、稳定契约、性能敏感逻辑和可测试核心逻辑。
- 蓝图：内容组装、表现层逻辑、快速迭代与关卡脚本。
- Data Asset/Data Table：设计师可调配置与内容定义。
- 权威游戏规则、GAS 初始化和网络复制必须由 C++ 契约控制；蓝图不得绕过契约直接修改权威状态。
- 详细边界以技术开发规范为准；具体系统的特殊边界通过 ADR 批准。

## GAS 架构

- 状态：已选定，尚未完成项目插件、模块依赖和基础类配置。
- 范围：角色技能、属性、消耗、冷却、Buff/Debuff、技能状态和装备对技能参数的修正。
- 玩家 ASC 宿主：TBD；多人重生需求下优先评估 PlayerState 持有方案。
- AI ASC 宿主：TBD。
- 玩家复制模式：TBD；优先评估 Mixed。
- AI 复制模式：TBD；优先评估 Minimal。
- 预测策略：玩家主动技能按需使用 GAS 预测；最终结果由服务器确认。
- Gameplay Tags、AttributeSet、Gameplay Effect 和 Ability 基类命名将在首个 GAS 系统设计中确定。

## 模块与子系统

| 名称 | 类型 | 职责 | 依赖 | 状态 |
|---|---|---|---|---|
| PolygonScifiWorlds | Runtime | 当前主游戏模块 | Core、CoreUObject、Engine、InputCore | Existing |

## Gameplay Framework

| 类型 | 当前类 | 计划职责 |
|---|---|---|
| GameMode | TBD | TBD |
| GameState | TBD | TBD |
| PlayerController | TBD | TBD |
| PlayerState | TBD | TBD |
| Character | `ASWCharacter_Base` | TBD |
| GameInstance | TBD | TBD |

## 系统架构

具体系统使用 [系统设计模板](Systems/SystemDesignTemplate.md)，并在本节维护索引。

| 系统 | 文档 | 状态 | 数据所有者 |
|---|---|---|---|
| TBD | TBD | Proposed | TBD |

## 数据与资源策略

- 资源命名规范：TBD
- Content 目录规范：TBD
- 软引用/硬引用规则：TBD
- Asset Manager 策略：TBD
- 存档兼容策略：TBD

## 联网模型

- 单机/多人：多人联机
- 权威模型：服务器权威
- 预测需求：角色移动使用 Character Movement 原生预测；适用技能使用 GAS 预测；其他状态默认不做权威预测
- Dedicated Server：正式技术目标；交付可配置并可完成完整对局的 DS Demo

## 性能预算

| 指标 | 目标 | 测量场景 |
|---|---:|---|
| 帧率 | TBD | TBD |
| Game Thread | TBD ms | TBD |
| GPU Frame | TBD ms | TBD |
| 峰值内存 | TBD | TBD |
| 包体大小 | TBD | TBD |

## 构建、测试与发布

- Development Editor 构建：所有代码变更的最低验证要求
- 自动化测试：核心数值、技能修正、装备叠加和胜负规则必须具备可重复测试
- 网络验证：所有权威玩法变更必须完成 DS + 至少两个客户端测试
- Cook/Package 验证：里程碑前执行客户端与 Dedicated Server 构建验证
- CI：TBD
- 崩溃与日志收集：TBD

## 技术参考来源

1. 本项目源码与 SSOT 文档
2. Epic 官方 UE 5.7 文档及本机 UE 5.7 引擎源码
3. `E:/Unreal Projects/Aura/Source/Aura/` 参考实现
4. Aura `.docs` 中的 GAS 与多人资料

Aura 仅作为实现模式参考，不构成本项目依赖；所有引用必须按本项目的所有权、生命周期与网络模型重新验证。

## 技术债务

| ID | 描述 | 影响 | 处理计划 |
|---|---|---|---|
| TD-01 | TBD | TBD | TBD |
