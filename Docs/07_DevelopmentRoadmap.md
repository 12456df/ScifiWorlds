# ScifiWorlds 模块开发路线图

**状态：** Approved（Baseline v1）
**最后更新：** 2026-08-21
**当前模块：** M12 — 防御塔与水晶（Not Started）

## 目的

本路线图定义从工程基线到可玩 Dedicated Server 垂直切片的推荐开发顺序。顺序按依赖关系排列：下游模块只能依赖已经完成并通过验收的上游模块。

这里的“模块”表示可独立设计、开发和验收的功能系统，不等同于 Unreal Engine 的 C++ 编译模块。项目早期继续使用单个 `PolygonScifiWorlds` Runtime 模块；只有在职责、依赖和独立构建需求明确后，才通过 ADR 决定是否拆分编译模块。

本项目为独立开发项目。路线图优先保证功能开发、技术正确性和可落地验证；不额外维护多人团队资源排序或 Must/Should/Could 优先级矩阵。

## 开发顺序

| ID | 模块 | 主要依赖 | 完成门槛 | 状态 | 建议 Commit |
|---|---|---|---|---|---|
| M00 | 文档与规范基线 | 无 | Charter、TDD、工程规范、开发路线图和 UE 项目上下文完成一致性检查 | Completed | `docs: establish project planning and technical standards` |
| M01 | 工程与构建基础 | M00 | 确定技术工程名；配置 GAS、Enhanced Input；Game、Editor、Server Target 均可编译；DS 可启动 | Completed | `build: establish project and dedicated server foundation` |
| M02 | 多人 Gameplay Framework | M01 | GameMode、GameState、PlayerController、PlayerState、Character 和队伍基础完成；客户端可加入 DS、分队、生成和退出 | Completed | `feat: establish multiplayer gameplay framework` |
| M03 | GAS 核心框架 | M02 | ASC、AttributeSet、Ability/Effect 基类、Native Gameplay Tags 与双端初始化完成；复制和重生后重新绑定正确 | Completed | `feat: establish multiplayer GAS foundation` |
| M04 | 输入、移动、相机与固定武器基础 | M02、M03 | Enhanced Input、第三人称移动、动画模板、镜头、固定武器、弹匣/换弹、瞄准、服务器权威弹丸和最小准星完成；DS 下移动、武器和远端表现正确 | Completed | `feat: add networked character controls and fixed weapon` |
| M05 | 战斗生命循环 | M03、M04 | 队伍关系、等级属性初始化、伤害、生命、击杀经验、死亡、按等级重生和临时无敌由服务器权威运行并正确复制；最小战斗 HUD 与伤害数字接入只读数据源 | Completed | `feat: add combat lifecycle` |
| M06 | 射击命中结算与扩展 | M04、M05 | 在 M04 固定武器闭环上完成命中扫描/投射物统一契约、伤害接入、角色射击差异化和服务器命中验证；同时收尾 M05 物理吸血并完成 SCI-6/SCI-7 | Completed | `feat: add authoritative shooting resolution` |
| M07 | 主动技能框架 | M03、M04、M05 | 扩展 M04 的 Input Tag → GAS Ability 通道；完成三个可扩展技能槽，其中配置两个首发样例技能；完成独立技能等级表、AbilityPoints 升级、AttributeSet 修正、目标、消耗、冷却/充能、取消、预测、Gameplay Cue 与最小冷却 UI；至少一个样例主动技能通过网络测试。Skill3 留作后续内容扩展，不是 M07 完成前置 | Completed | `feat: add gameplay ability pipeline` |
| M08 | 装备与技能修正 | M03、M07 | 数据驱动装备定义和 Infinite GE 修正完成；按槽应用/移除、叠加、重生保留和失败回滚正确；武器与技能通过 AttributeSet 聚合值受到修正 | Completed | `feat: complete M08 equipment modifiers and M09 economy shop` |
| M09 | 经济、背包与商店 | M02、M08 | PlayerState 金币、等级击杀赏金、被动收入、六槽装备栏、商店区域、Tab 浏览、购买/出售原子事务、OwnerOnly 复制和重生保留完成 | Completed | `feat: complete M08 equipment modifiers and M09 economy shop` |
| M10 | 路线与兵线生成 | M02 | 三条路线、出生点、波次和双方周期生成均由数据驱动，并由服务器控制 | Completed | `feat: add lane and wave system` |
| M11 | 小兵 AI 与战斗 | M05、M10 | 小兵沿线移动、选择目标、攻击和死亡完整运行；AI 由服务器执行，客户端只接收必要状态 | Completed | `feat: add networked minion AI` |
| M12 | 防御塔与水晶 | M05、M10、M11 | 队伍归属、攻击、受击、路线推进约束和水晶状态完成并由服务器权威同步 | Not Started | `feat: add towers and team crystals` |
| M13 | 完整比赛流程 | M02、M12 | 准备、开始、进行、结束和重置状态完整；摧毁敌方水晶后服务器唯一判定并同步结果 | Not Started | `feat: complete authoritative match flow` |
| M14 | HUD 与游戏 UI | M05、M08、M09、M13 | 在 M05 Combat Overlay 基础上完成技能、冷却、装备、商店、目标、比赛状态、计分板和结算界面，并统一完整 HUD 视觉与导航 | Not Started | `feat: add gameplay HUD and shop UI` |
| M15 | 会话与 DS 部署 | M01、M02、M13 | DS 参数、会话发现、加入、断线处理、服务器构建与部署说明完成 | Not Started | `feat: add dedicated server session flow` |
| M16 | 垂直切片与加固 | M01–M15 | 至少一个完整角色和三路对局可在 DS 上从加入玩到结算；通过晚加入、重生、断线、丢包、性能和打包测试 | Not Started | `feat: deliver playable multiplayer vertical slice` |

## 每个模块的标准工作流

1. 从 `Docs/Systems/SystemDesignTemplate.md` 创建模块设计文档。
2. 完成问题定义、FR/NFR、边界情况和明确不做的范围。
3. 定义子系统职责、数据所有权、依赖 DAG、公开契约和网络权威。
4. 按依赖顺序实现 C++、蓝图和数据资产。
5. 通过模块设计文档中的需求追踪测试。
6. 编译受影响的 Development Editor、Game 和 Server Target。
7. 从 M02 开始，使用 Dedicated Server + 至少两个客户端完成相关流程测试。
8. 检查 Git 差异，更新 TDD、UE 项目上下文、风险和变更记录。
9. 使用表格中的 Commit 信息提交；提交后将本文件状态移动到下一模块。

## 模块完成规则

模块只有同时满足以下条件才可标记为 `Completed`：

- [ ] 设计文档达到 `Approved`
- [ ] 所有验收标准通过
- [ ] 编译和适用的 DS 多客户端测试通过
- [ ] 没有新增未登记的技术债务、循环依赖或隐藏耦合
- [ ] 相关文档和 SSOT 已同步
- [ ] Git 工作区的提交范围经过检查
- [ ] 模块 Commit 已创建并记录哈希

### M00 特例

M00 的目标是建立足以安全开始开发的功能方向与技术规范，不要求提前填满商业、发行、日期、美术产量等非阻塞产品选项。允许 Charter/TDD 保留已明确标注的非阻塞 `TBD`，但以下内容必须确定：

- 核心玩法和目标 Demo
- 技术栈、网络权威与 Dedicated Server 方向
- C++/蓝图边界和工程质量要求
- 模块开发顺序与逐模块提交规则
- 独立开发工作方式
- 仓库及第三方资产安全策略

## 完成记录

| 模块 | Commit | 完成日期 | 验证摘要 |
|---|---|---|---|
| M00 | `milestone/m00` | 2026-07-20 | 功能方向、GAS/多人/DS 技术规范、独立开发流程、路线图、SSOT 与私有仓库安全策略完成审计 |
| M01 | `milestone/m01` | 2026-07-23 | UE 5.7.4 源码引擎、GAS/Enhanced Input、Game/Editor/Server Target、WindowsServer Cook/Stage、DS UDP 7777 与本地客户端连通性均已验证；参数化脚本覆盖构建、Cook 与本地 DS 启动。 |
| M03 | `cded842` | 2026-07-25 | 玩家 ASC/AttributeSet 归属 PlayerState，服务器与拥有者客户端完成 Owner/Avatar 绑定；原生 Tag、Ability/Effect 基类、进度字段和 AI 自持 ASC 基础完成。Editor/Game/Server Development Target 构建成功，Staged DS 加两客户端完成 GAS 调试验证。 |
| M04 | `feat: add networked character controls and fixed weapon` | 2026-07-30 | 输入、第三人称移动、相机、动画模板、固定武器、服务器权威弹丸、瞄准/开火/换弹与最小 UI 完成。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成联机功能验证。`WeaponUpperSM` BlendSpace 问题已登记为非阻塞后续项。 |
| M05 | `feat: add combat lifecycle` | 2026-08-05 | 队伍关系、等级属性初始化、伤害、击杀经验、死亡、重生、临时无敌、战斗 HUD 与伤害数字完成。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成验证。后续动画、命中表现与资源恢复事项已登记至 Linear Backlog。 |
| M06 | `feat: add authoritative shooting resolution` | 2026-08-07 | 物理吸血、投射物与 Hitscan 统一服务器权威伤害结算、射击 Montage/Section/Notify 契约完成；开火与瞄准仅影响手臂，换弹保持上半身分层，枪口与瞄准方向表现已验证。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成命中、伤害、吸血、弹药与动画验证。 |
| M07 | `feat: add gameplay ability pipeline` | 2026-08-10 | 三固定可扩展技能槽、两个首发样例技能、独立等级、AbilityPoints 升级、资源消耗、目标验证、冷却/充能、确认式施法、技能栏与升级加号完成。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成技能施放、效果、消耗、冷却/充能、升级及死亡重生后的状态验证。Skill3 按收缩范围保留为空槽。 |
| M08 | `feat: complete M08 equipment modifiers and M09 economy shop` | 2026-08-15 | 数据驱动装备定义、Infinite GE 槽位应用/移除、装备重生保留与最大资源按比例换算完成；首批八件装备覆盖主要 AttributeSet 修正并通过验证。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成装备属性、重生保留和 UI 验证。 |
| M09 | `feat: complete M08 equipment modifiers and M09 economy shop` | 2026-08-15 | PlayerState 金币、等级击杀赏金、被动收入、六槽装备栏、交易区域、Tab 商店界面与服务器权威购买/出售原子事务完成。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成金币、交易、装备生效/出售、死亡重生与 UI 验证。 |
| M10 | `feat: add lane and wave system` | 2026-08-17 | Mass 20 Entity 冒烟、三条路线、双方三路周期波次、Entity/Actor/ASC Ready Bridge 与原子回滚完成。Development Editor/Game/Server Target 构建成功；Staged DS 加双客户端完成多波生成、Team/Level/ASC 关联和晚加入状态验证。 |
| M11 | `d95e74f` | 2026-08-21 | 服务器权威 Mass/StateTree 小兵完成沿线推进、索敌、攻击、死亡与回收；脱战重新索敌、同队轻量分离、交会绕行与攻击转向完成。用户已在 Staged DS 加双客户端验证移动、战斗、死亡、晚加入与比赛结束表现。 |

## 路线图变更规则

- 模块可以拆分，但不得在没有说明依赖影响的情况下合并或跳过。
- 改变 GAS 所有权、服务器拓扑、模块边界或核心数据所有权时，必须新增 ADR。
- 调整顺序时，应同步更新依赖、完成门槛和生产计划。
- 若原型验证否定核心假设，应回到 Project Charter/GDD 重新评审，而不是继续堆叠下游实现。
