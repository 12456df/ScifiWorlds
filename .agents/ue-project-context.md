# UE Project Context

*Last updated: 2026-07-24*

## Engine & Project Overview

**Engine version:** Unreal Engine 5.7.4 源码构建（`5.7.4-release`，commit `260bb2e1c5610b31c63a36206eedd289409c5f11`）
**Product name:** ScifiWorlds
**Technical project/module name:** PolygonScifiWorlds
**Description:** 科幻题材多人联机第三人称技能射击推塔游戏，具有差异化角色射击、专属技能、技能导向装备构筑、三路小兵推进和水晶胜利目标。
**Project type:** Game
**Genre / domain:** Third-person hero/ability shooter + lane-pushing objective game
**Target platforms:** TBD
**Default editor/game map:** `/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket`

## Module Structure

**Primary game module:** `PolygonScifiWorlds`

| Module | Type | Notes |
|---|---|---|
| PolygonScifiWorlds | Runtime | 当前主游戏模块 |

**Key dependencies:**

- **Public:** Core、CoreUObject、Engine、InputCore
- **Private:** GameplayAbilities、GameplayTags、GameplayTasks、EnhancedInput
- **Build settings:** BuildSettingsVersion.V6
- **Targets:** `PolygonScifiWorlds`（Game）、`PolygonScifiWorldsEditor`（Editor）、`PolygonScifiWorldsServer`（Dedicated Server）

## Plugin Dependencies

**Enabled project plugins:**

- `PythonScriptPlugin` — 仅限 Editor Target，避免形成 Server Runtime 依赖
- `BpGeneratorUltimate` — Fab 已购插件，已安装到本机源码引擎；仅含 Editor 模块，不进入 Game/Server Runtime，原始内容不纳入项目 Git 仓库
- `GameplayAbilities` — GAS Runtime 插件，已显式启用
- `EnhancedInput` — Enhanced Input Runtime 插件，已显式启用

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

**Custom subsystems:** None implemented/detected.
**GAS usage:** Selected as the core gameplay ability framework; implementation not configured yet.

- Player ASC owner: TBD; evaluate PlayerState ownership for respawn persistence
- AI ASC owner: TBD
- Player replication mode: TBD; evaluate Mixed
- AI replication mode: TBD; evaluate Minimal
- Ability/AttributeSet/Tag base classes: TBD

**Planned gameplay domains:**

- Character-specific shooting and abilities
- Equipment-driven ability attribute modification
- Match flow and server-authoritative win condition
- Symmetric three-lane map, periodic minion spawning and lane advancement
- Defensive objectives and team crystals

## Build Configuration

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
- ASC 宿主、复制模式、AttributeSet/Ability 基类与 Gameplay Tag 规范
- 插件保留策略、具体断言/日志规则和 CI 方案
