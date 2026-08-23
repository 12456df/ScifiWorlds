# UE Project Context

*Last updated: 2026-08-21*

## Engine & Project Overview

**Engine version:** Unreal Engine 5.7.4 源码构建（`5.7.4-release`，commit `260bb2e1c5610b31c63a36206eedd289409c5f11`）
**Product name:** ScifiWorlds
**Technical project/module name:** PolygonScifiWorlds
**Description:** 科幻题材多人联机第三人称技能射击推塔游戏，具有差异化角色射击、专属技能、技能导向装备构筑、三路小兵推进和水晶胜利目标。
**Project type:** Game
**Genre / domain:** Third-person hero/ability shooter + lane-pushing objective game
**Target platforms:** TBD
**Default editor/game map:** `/Game/PolygonSciFiWorlds/Maps/GameMap`

## Module Structure

**Primary game module:** `PolygonScifiWorlds`

| Module | Type | Notes |
|---|---|---|
| PolygonScifiWorlds | Runtime | 当前主游戏模块 |

**Key dependencies:**

- **Public:** Core、CoreUObject、Engine、InputCore、GameplayAbilities、GameplayTags、GameplayTasks、UMG、DeveloperSettings、MassEntity、MassSpawner
- **Private:** EnhancedInput、MassCommon、MassActors
- **Build settings:** BuildSettingsVersion.V6
- **Targets:** `PolygonScifiWorlds`（Game）、`PolygonScifiWorldsEditor`（Editor）、`PolygonScifiWorldsServer`（Dedicated Server）

## Plugin Dependencies

**Enabled project plugins:**

- `PythonScriptPlugin` — 仅限 Editor Target，避免形成 Server Runtime 依赖
- `BpGeneratorUltimate` — Fab 已购插件，已安装到本机源码引擎；仅含 Editor 模块，不进入 Game/Server Runtime，原始内容不纳入项目 Git 仓库
- `GameplayAbilities` — GAS Runtime 插件，已显式启用
- `EnhancedInput` — Enhanced Input Runtime 插件，已显式启用
- `MassGameplay` — M10 使用的 Mass Runtime/Spawner/Actor 桥依赖；小兵 Entity 仅在服务器或 Standalone 执行
- `MassAI` — M11 小兵 StateTree/Mass 行为依赖；仅在服务器或 Standalone 执行
- `StateTree` — M11 小兵行为状态与任务定义的 Runtime 依赖

**Selected but not yet configured:** None.

**Custom project plugins:** None detected.

## Coding Conventions

**Naming prefixes:** Epic 标准 UE 前缀（F/U/A/E/I）；项目类当前使用 `SW` 标识。
**Header style:** `#pragma once`
**Header organization:** 模块使用 Public/Private 目录。
**Current base character:** `ASWCharacter_Base`
**Log categories:** 尚未建立。
**Assertion style:** 尚未建立。
**Development model:** C++ + Blueprint hybrid.
**Quality priorities:** Robustness、extensibility、readability、maintainability；avoid excessive coupling.
**Additional rules:** 参见仓库根目录 `AGENTS.md`、`CONTRIBUTING.md` 与 `Docs/Engineering/TechnicalStandards.md`。

## Subsystems in Use

**Gameplay framework:**

- GameMode: `ASWGameMode`
- GameState: `ASWGameState`
- PlayerController: `ASWPlayerController`
- PlayerState: `ASWPlayerState`
- Pawn / Character: `ASWCharacter_Base`

**Custom subsystems:** `USWAssetManager`：启动时初始化 GAS 全局数据。
**GAS usage:** M03 已完成。

- Player ASC owner: `ASWPlayerState`; current `ASWCharacter_Base` is the Avatar and rebinds after possession/respawn.
- Player replication mode: `Mixed`.
- AI ASC owner: `ASWCharacter_Enemy` 自身。
- AI replication mode: `Minimal`.
- M03 base types: `USWAbilitySystemComponent`、`USWAttributeSet`、`USWGameplayAbility`、`USWGameplayEffect` 与原生 `SWGameplayTags`。
- Player progression owner: `ASWPlayerState` owns replicated `Level`, `Experience`, and `AbilityPoints`; level curves and concrete values remain data-driven `TBD`.

**Planned gameplay domains:**

- Character-specific shooting and abilities
- Equipment-driven ability attribute modification
- Match flow and server-authoritative win condition
- Symmetric three-lane map, periodic minion spawning and lane advancement
- Defensive objectives and team crystals

## Build Configuration

### M12/M13 Validation (2026-08-23)

- 用户已确认完成 M12/M13 的 Dedicated Server 构建与双客户端验证：防御塔/水晶的目标选择、攻击、伤害接收、前置结构解锁、推塔计分与水晶胜负裁决均为服务器权威。
- 比赛结束后服务器和客户端保持 `WaitingPostMatch`；M13 不自动重开或 Travel，回大厅与下一局流程明确留待 M15。
- M12/M13 收尾提交将在 `main` 完成后记录；下一模块为 M14。

### M11 Validation (2026-08-21)

- 用户已确认 Staged Dedicated Server + 两客户端完成 M11 验收：小兵在服务器权威 Mass/StateTree 下完成推进、索敌、攻击、死亡与回收；客户端正确接收移动、攻击、死亡表现，晚加入可恢复当前可见状态。
- 本次验证同时覆盖脱战后的重新索敌、敌我小兵交会的轻量分离与绕行，以及由复制速度驱动的稳定 Idle/Locomotion 动画。
- M11 实现已提交为 `d95e74f`；路线图与完成闸门已同步，下一模块为 M12。

### M10 Validation (2026-08-17)

- MassEntity/MassSpawner/MassActors 已接入 Runtime 模块，MassGameplay 已启用；20 Entity 冒烟以及三路、两队周期波次均已验证。
- 每个首版小兵采用服务器 Mass Entity 与可复制 `ASWCharacter_Minion`/ASC 的桥接；客户端不运行生成或 AI 决策。
- Development Editor、Game、Dedicated Server Target 和 Staged DS + 两客户端的多波生成、Team/Level/ASC 关联与晚加入状态已完成验证。

### M04 Validation (2026-07-30)

- Development Editor、Game 和 Dedicated Server Target 均已构建成功。
- 已完成 Staged Dedicated Server 加两个外部客户端验证：连接、第三人称移动、跳跃、下蹲、疾跑、瞄准、开火、换弹、弹药同步、服务器权威弹丸和最小准星均可运行。
- 同机双客户端验证前，两个客户端均需执行 `t.MaxFPS 60` 与 `t.IdleWhenNotForeground 0`，避免未限帧前台窗口造成 GPU/Draw 争用，使失焦客户端帧时间及本地 Ping/Jitter 诊断失真。
- `WeaponUpperSM` 未按预期使用 `BS1D_PistolUpper_Locomotion` 的问题已登记为 M04 非阻塞后续项。

### M03 Validation (2026-07-25)

- Development Editor、Game 和 Dedicated Server Target 均已构建成功。
- 已完成 Staged Dedicated Server 加两个客户端验证：玩家完成分队后在准备期生成并 Possess Pawn；两个客户端的 `showdebug AbilitySystem` 均确认 ASC、AttributeSet 与 PlayerState Owner / Character Avatar 绑定正确。
- 编辑器 Gameplay Tag 选择器可见 M03 原生根 Tag；具体技能、属性动态写入与进度动态变化将在首次拥有对应生产者的 M05/M07 验证。

### M02 Validation (2026-07-24)

- Development Editor、Game 和 Dedicated Server Target 均已构建验证。
- 已完成 Dedicated Server + 两客户端的加入、TeamA/TeamB 分队、带标签出生、TeamId 复制、队伍人数查询和退出状态更新验证。
- 比赛地图使用 `APlayerStart::PlayerStartTag` 的 `TeamA` / `TeamB` 标签；`ASWGameMode` 是服务器唯一的分队与出生点选择权威。

### M01 Validation (2026-07-23)

- Development Editor, Game, and Dedicated Server targets built successfully from the UE 5.7.4 source build.
- The WindowsServer archive was built, cooked, staged, and archived outside the repository.
- The staged Dedicated Server loaded `/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket`, listened on UDP 7777, and accepted a local client connection.
- `Scripts/Build/BuildTargets.ps1`, `Scripts/Build/CookServer.ps1`, and `Scripts/Run/StartLocalDS.ps1` provide parameterized local build, cook, and DS-start entry points.

**Build targets:** Game、Editor、Dedicated Server；`PolygonScifiWorldsEditor Development Win64` 已通过源码引擎编译（2026-07-23），Server Target 已创建并通过项目文件生成验证
**Custom macros / flags:** None detected.
**Third-party C++ libraries:** None detected.
**Platform-specific code:** None detected.
**Engine modifications:** 官方 `5.7.4-release` 源码基线；本机额外安装 `BpGeneratorUltimate` Editor 插件，未修改引擎源码。
**Networking goal:** Multiplayer, server-authoritative, with a configurable and playable Dedicated Server demo.

## Technical Reference Policy

1. ScifiWorlds source and repository SSOT for project behavior and decisions.
2. Epic UE 5.7 documentation and installed UE 5.7 source for engine/API behavior.
3. `E:/Unreal Projects/Aura/Source/Aura/` for reference implementation patterns only.
4. Aura `.docs` GAS and multiplayer materials for supplementary guidance.

Aura is not a dependency. Read Aura `AGENTS.md` and `.agents/ue-project-context.md` before inspecting its source; resolve engine behavior conflicts in favor of Epic UE 5.7 documentation/source.

## Team Context

**Team:** Solo developer (`12456df`)
**Source control:** GitHub + Git LFS
**Default branch:** `main`
**Repository visibility:** Private
**Binary asset tracking:** `*.uasset`、`*.umap` through Git LFS
**Branching strategy:** `main` 保持可构建；短生命周期 `feature/*` 分支，具体规则见 `CONTRIBUTING.md`。
**Documentation:** 仓库内 Markdown，入口为 `Docs/README.md`。

**Solo development policy:** Prioritize functional delivery and technical standards. Module dependency order replaces a separate Must/Should/Could priority matrix; non-blocking commercial and release decisions are deferred until needed.

## Known Unknowns

- 目标平台与性能预算
- 商业模式及正式发行方式（当前非阻塞）
- 对局人数、角色数量、单局时长与 Demo 内容规模
- Dedicated Server 的托管方式、会话发现与玩家认证方案
- 正式 Gameplay Framework 类与子系统边界
- M03 具体属性上限、等级曲线、战斗公式与 Tag 叶子项
- 插件保留策略、具体断言/日志规则和 CI 方案
