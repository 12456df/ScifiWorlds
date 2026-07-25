# ADR-0002：玩家 ASC 归属 PlayerState

**状态：** Accepted
**日期：** 2026-07-25
**决策者：** `12456df`

## 背景

ScifiWorlds 是服务器权威的多人游戏，角色 Pawn 会在死亡和重生时被替换。技能、属性、等级、经验和技能点不能因 Pawn 生命周期结束而丢失；M03 还需要保证 ASC 在服务器与拥有者客户端都能正确重绑。

## 决策

玩家的 `USWAbilitySystemComponent` 和 `USWAttributeSet` 由 `ASWPlayerState` 持有，PlayerState 是 ASC 的 Owner；当前 `ASWCharacter_Base` 是 Avatar。玩家 ASC 使用 `Mixed` Gameplay Effect 复制模式。服务器在 Possess 后、拥有者客户端在 `OnRep_PlayerState` 后调用幂等的 `InitAbilityActorInfo(PlayerState, Character)`；每次 Pawn 更换均重绑 Avatar。

`ASWPlayerState` 同时持有服务器权威且可复制的 `Level`、`Experience` 和 `AbilityPoints`。AI 不复用此方案：未来 AI 直接由 AI Character 持有 ASC，并采用 `Minimal` 复制模式。

## 备选方案

| 方案 | 优点 | 缺点 | 未选择原因 |
|---|---|---|---|
| Character 持有玩家 ASC | 初始实现较直接 | Pawn 重生会丢失或迁移状态，生命周期耦合更强 | 不满足跨重生持久性目标 |
| PlayerState 持有玩家 ASC | 跨 Pawn 生命周期稳定，符合多人重生模型 | 需要双端初始化与 Avatar 重绑 | 已选择 |
| GameState/全局管理器持有 ASC | 可集中管理 | 没有清晰的玩家所有权，易形成全局耦合 | 不符合数据所有权原则 |

## 后果

### 正面

- 玩家 GAS 状态与等级进度跨重生保持。
- 能力、属性和复制策略的所有者清晰；Character 只承担 Avatar 生命周期。
- 后续装备可通过 Gameplay Effect 修改同一 ASC 上的属性，供技能统一读取。

### 成本

- 必须严格实现服务器与客户端两条 `InitAbilityActorInfo` 路径，并防止重复初始化。
- 所有依赖 Avatar 的能力都必须处理 Avatar 失效、结束和重新绑定。

## 验证与复审

- 验证：Dedicated Server + 两客户端完成属性复制、Pawn 替换、晚加入与进度保持检查。
- 复审：若引入不依赖 PlayerState 的可玩单位、持久化跨会话档案，或更改玩家重生模型时复审。
