# ADR-0003：服务器 Mass ECS 编排与 Actor/GAS 小兵战斗

**状态：** Accepted
**日期：** 2026-08-15
**决策者：** `12456df`

## 背景

M10/M11 需要实现双方三路周期出兵、小兵沿线推进、索敌、攻击和死亡。项目希望借此学习 Mass Entity/ECS，但现有 M05 已经验证了 `ASWCharacter_Enemy` 自持 ASC、`USWAttributeSet`、队伍关系、伤害、死亡、经验和金币奖励。若 Mass 再维护一套 Health、Team、Damage 或死亡真值，会形成两个权威来源；若完全沿用逐 Actor `AIController + BehaviorTree + Tick`，又无法达到本阶段的 ECS 学习目标。

UE 5.7 的 `MassEntity` 已进入 Engine Runtime；同名旧插件是 deprecated 空壳，不需要启用。`MassGameplay` 与 `MassAI` 仍属于需要谨慎采用的功能，因此必须先通过 Editor/Game/Server 构建与 Dedicated Server 技术冒烟，再扩大实现。

## 决策

M10/M11 采用以下混合架构：

1. **Mass Entity 仅在服务器/Standalone 执行**，负责小兵实体组成、路线推进数据、目标意图、行为状态、批处理和生命周期编排。
2. **每个首版小兵保持一个可复制的 `ASWCharacter_Minion` Actor**。Actor 自持 ASC/AttributeSet，继续作为 TeamId、CombatLevel、Health、死亡、GAS Effect 与奖励的权威玩法真值。
3. Mass 中的 Team/Lane/Level 等字段是出生时写入的不可变查询缓存；Actor 初始化完成后必须校验二者一致。Health、Cooldown、Damage、XP、Gold 不复制进 Mass Fragment。
4. **Mass StateTree 是行为状态的唯一编排者**；StateTree 只切换执行标签/意图，Movement、Targeting、Attack、Death Processor 执行批处理。M11 不为每个小兵创建 AIController、BehaviorTree 或 Tick。
5. 路线首版使用关卡中可编辑的三条 `ASWLaneRoute` Spline。TeamA 从起点到终点，TeamB 反向行进；不在 M10/M11 同时引入 ZoneGraph、NavMesh 寻路、Smart Object 或 MassCrowd。
6. 首版网络表现使用现有 Actor Movement、Actor 属性复制与 AI ASC `Minimal` 复制。**不同时启用 MassReplication 来复制同一小兵状态**，避免双重网络权威。
7. 首版所有小兵保持高精度 Actor 表现，不做 ISM/Actor LOD 切换和对象池。只有性能数据证明需要，且 ASC 生命周期迁移方案完成后，才复审 Mass Representation/Replication/Pooling。

## 备选方案

| 方案 | 优点 | 缺点 | 未选择原因 |
|---|---|---|---|
| 纯 Actor + AIController + BehaviorTree | 与常规 UE AI 接近，初期直观 | 每单位对象与行为开销高，不能形成 Mass ECS 学习闭环 | 不满足本阶段学习目标 |
| 纯 Mass，Health/Combat 也放 Fragment | 数据局部性和规模潜力最高 | 必须重写 M05 的 GAS 命中、死亡和奖励链，并解决玩家 Ability 对 Entity 目标的桥接 | 当前成本与风险过高 |
| Mass + Actor/GAS 混合 | 复用已验证战斗链，同时学习 Entity/Fragment/Processor/StateTree | 每个单位仍保留 Actor/ASC，首版性能收益有限 | 已选择，最符合当前项目阶段 |
| Mass + ZoneGraph + MassCrowd + MassReplication 一次落地 | 可覆盖完整 Mass 技术栈 | 同时引入过多实验边界，故障难定位，独立开发成本过高 | 拆到后续按性能证据引入 |

## 后果

### 正面

- M05 的伤害、队伍、死亡、XP 和 Gold 仍只有一个权威实现。
- M10/M11 可以按 Entity → Fragment → Trait → Processor → StateTree 的顺序学习 ECS。
- 客户端继续依赖已验证的 Actor/GAS 复制，晚加入无需重放历史 RPC。
- M12 的塔和水晶可通过稳定 Target/Combat 接口接入，而不依赖小兵具体类。

### 负面与代价

- 首版不能体现 Mass 的完整表示 LOD 和网络带宽收益；同时存在 Entity 与 Actor 的生命周期桥接成本。
- 访问 Actor/ASC 的 Processor 必须运行在 Game Thread；纯数据 Processor 才允许后续并行化。
- Mass 插件升级可能带来 API 变化，项目代码必须隔离在 `AI/Mass/`、`Lane/` 和 `Minion/` 边界内。
- 如果未来取消每单位 ASC，需要新增 ADR，并重新设计 Damage Target、Buff/Debuff、奖励和复制。

## 验证与复审

- 验证方式：先完成 20 个测试 Entity 的 DS 冒烟；再完成三路双方多波生成、移动、交战、死亡、奖励、晚加入与重复清理测试；Editor/Game/Server 三 Target 全部构建。
- 复审条件：服务器小兵预算无法满足、Actor/ASC 成为主要瓶颈、需要超过当前目标规模，或 Epic 改变 MassGameplay/MassAI 的稳定性与复制模型。
