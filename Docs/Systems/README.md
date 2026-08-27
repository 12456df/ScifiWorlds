# 系统设计文档

每个进入生产的游戏系统都必须从 [系统设计模板](SystemDesignTemplate.md) 创建独立文档。

## 设计顺序

1. 问题定义
2. 可测试需求与明确边界
3. 子系统拆分与依赖 DAG
4. 接口、数据契约和事件
5. 运行时数据流
6. 按依赖排序的实现计划
7. 需求追踪与验证

## 系统索引

| 系统 | 文档 | 负责人 | 状态 |
|---|---|---|---|
| M01 工程与构建基础工作清单 | [M01_ProjectBuildFoundation_Checklist.md](M01_ProjectBuildFoundation_Checklist.md) | `12456df` | Approved |
| M02 多人 Gameplay Framework | [M02_MultiplayerGameplayFramework.md](M02_MultiplayerGameplayFramework.md) | `12456df` | Approved |
| M03 GAS 核心框架 | [M03_GASCoreFramework.md](M03_GASCoreFramework.md) | `12456df` | Approved |
| M04 输入、移动、相机与固定武器基础 | [M04_InputMovementCameraWeapon.md](M04_InputMovementCameraWeapon.md) | `12456df` | Approved |
| M05 战斗生命循环 | [M05_CombatLifecycle.md](M05_CombatLifecycle.md) | `12456df` | Completed |
| M06 射击命中结算与扩展 | [M06_AuthoritativeShootingResolution.md](M06_AuthoritativeShootingResolution.md) | `12456df` | Completed |
| M07 主动技能框架 | [M07_GameplayAbilityPipeline.md](M07_GameplayAbilityPipeline.md) | `12456df` | Completed |
| M08 装备与属性修正 | [M08_EquipmentAttributeModifiers.md](M08_EquipmentAttributeModifiers.md) | `feature/m08-m09-equipment-economy-shop` | Completed |
| M09 经济、六槽装备栏与商店 | [M09_EconomyInventoryShop.md](M09_EconomyInventoryShop.md) | `feature/m08-m09-equipment-economy-shop` | Completed |
| M10 路线、波次与 Mass 生成基础 | [M10_LaneWaveMassFoundation.md](M10_LaneWaveMassFoundation.md) | `12456df` | Approved |
| M11 Mass 小兵 AI 与战斗 | [M11_MassMinionAICombat.md](M11_MassMinionAICombat.md) | `12456df` | Approved |
| M12 防御塔与水晶 | [M12_DefenseStructures.md](M12_DefenseStructures.md) | `12456df` | Completed |
| M13 完整比赛流程 | [M13_AuthoritativeMatchFlow.md](M13_AuthoritativeMatchFlow.md) | `12456df` | Completed |
| M14 对局 HUD 与游戏润色 | [M14_GameplayHUDAndPolish.md](M14_GameplayHUDAndPolish.md) | `12456df` | Draft |
