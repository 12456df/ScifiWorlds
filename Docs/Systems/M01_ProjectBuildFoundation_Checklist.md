# M01 工程与构建基础工作清单

**文档状态：** Approved
**执行状态：** In Progress
**负责人：** `12456df`
**最后更新：** 2026-07-22
**建议分支：** `feature/m01-build-foundation`
**建议提交：** `build: establish project and dedicated server foundation`

## 1. 目标

为 ScifiWorlds 建立可重复验证的 Unreal Engine 5.7 工程与构建基线，使 `PolygonScifiWorlds` 的 Development Editor、Game、Server Target 均可编译，并让 Dedicated Server 以无渲染进程启动、加载指定地图和监听网络端口。

本清单依据：

- `Docs/07_DevelopmentRoadmap.md`
- `Docs/Engineering/TechnicalStandards.md`
- `.agents/ue-project-context.md`
- Epic UE 5.7 官方源码构建与 Dedicated Server 文档

## 2. 完成门槛

- [x] 技术工程名统一为 `PolygonScifiWorlds`。
- [x] 项目使用可编译 Server Target 的 UE 5.7 源码构建。
- [x] `GameplayAbilities` 与 `EnhancedInput` 插件显式启用。
- [x] `GameplayAbilities`、`GameplayTags`、`GameplayTasks`、`EnhancedInput` 模块依赖已配置。
- [x] Development Editor Target 编译通过。
- [x] Development Game Target 编译通过。
- [x] Development Server Target 编译通过。
- [x] Server内容完成Cook/Stage，DS进程能够启动、加载地图并监听端口。
- [x] 构建和DS启动步骤可以通过脚本或已记录命令重复执行。
- [x] 相关文档与项目上下文已同步。
- [ ] M01提交和里程碑Tag已创建。

任何一项缺少可复核证据时，不得将M01标记为 `Completed`。

## 3. 已知基线与阻塞

执行前重新核验，不得直接把本表当作永久事实。

| 项目 | 当前观测 | 处理要求 |
|---|---|---|
| 产品名 | `ScifiWorlds` | 保持不变 |
| 技术工程/模块名 | `PolygonScifiWorlds` | 核对 `.uproject`、Source目录、Build.cs和Target类一致 |
| 引擎版本 | UE 5.7.4 源码构建（`5.7.4-release`） | 最终以实际源码引擎 `Build.version` 与 Git Tag/commit 为证据 |
| 当前预编译引擎 | `E:\Epic Games\UE_5.7` | 不作为Server Target最终构建环境 |
| 当前5.7注册路径 | Launcher 注册已对齐至 `E:\Epic Games\UE_5.7`；项目已关联源码引擎 | 保持 Launcher 注册正确；项目使用源码引擎注册标识 |
| GAS | 尚未显式配置 | 配置插件和模块依赖，不创建ASC/AttributeSet |
| Enhanced Input |默认输入类已配置，项目依赖尚未显式声明 | 显式配置插件和模块依赖 |
| Target | Game、Editor已存在，Server缺失 | 新增Server Target |
| Git | M00分支与未跟踪学习文档需要先整理 | 不得把无关资料意外混入M01提交 |

## 4. 范围边界

### M01包含

- 引擎来源、版本和工程关联。
- `.uproject`插件声明。
- `Build.cs`模块依赖与IWYU基线。
- Game、Editor、Server Target。
- Server默认地图。
- Rider工程文件生成。
- Editor、Game、Server编译。
- Server Cook/Stage和本地DS启动验证。
- 可重复构建/启动命令与验证记录。

### M01不包含

- GameMode、GameState、PlayerController、PlayerState和队伍系统（M02）。
- ASC、AttributeSet、Gameplay Ability/Effect基类和Native Gameplay Tags（M03）。
- 输入绑定、角色移动、镜头和瞄准（M04）。
- Session、Lobby、Matchmaking、EOS/Steam和托管部署（M15）。
- 正式登录、分队、生成、重生、晚加入和完整断线恢复流程。
- 新的UE C++编译模块拆分；M01继续使用单个Runtime模块。

## 5. 执行清单

### A. Git与文档基线

- [ ] 确认M00已经合并进 `main`，且 `main` 可构建。
- [ ] 单独处理 `Docs/GAS/` 和 `Docs/MultiPlayerNetWorkLearing/` 等未跟踪资料；不要意外混入M01构建提交。
- [ ] 从更新后的 `main` 创建 `feature/m01-build-foundation`。
- [ ] 确认分支开始时没有意外修改、生成文件或未授权资产。
- [ ] 从 `Docs/Systems/SystemDesignTemplate.md` 创建 `Docs/Systems/M01_ProjectBuildFoundation.md`。
- [ ] 在设计文档中填写FR/NFR、边界情况、依赖、实施顺序和需求追踪。
- [ ] 创建或确认源码引擎决策ADR，例如 `Docs/ADR/ADR-0001-Use-UE57-Source-Build.md`。
- [ ] 在进入工程修改前，将M01设计文档状态推进到 `Approved`。

证据：

```text
起始分支/Commit：TBD
M01设计文档：TBD
源码引擎ADR：TBD
```

### B. UE 5.7源码引擎

- [x] 获得与项目基线匹配的Epic官方UE 5.7源码版本。
- [x] 记录准确版本、Patch、Changelist和源码引用（分支或Tag）。
- [x] 执行源码引擎 `Setup.bat`。
- [x] 执行源码引擎 `GenerateProjectFiles.bat`。
- [x] 编译 `UE5 Development Editor Win64`。
- [x] 启动源码构建的Unreal Editor并确认版本。
- [x] 使用UnrealVersionSelector注册该源码引擎。
- [x] 将 `PolygonScifiWorlds.uproject` 关联到源码引擎。
- [x] 确认项目不再解析到不存在的 `D:\UE_5.7`。
- [ ] 不在仓库脚本或文档中硬编码个人机器的引擎绝对路径；使用参数传入。

证据：

```text
Engine Build.version：5.7.4，Changelist 0，CompatibleChangelist 47537391，BranchName `UE5`
源码分支/Tag：`5.7.4-release`，commit `260bb2e1c5610b31c63a36206eedd289409c5f11`
UE5 Editor构建结果：`UnrealEditor Win64 Development` Succeeded（2026-07-23）；`ShaderCompileWorker Win64 Development` Succeeded
项目EngineAssociation：`{F852BF2C-4BD1-6E80-B624-9CABBF35BDEA}` → `E:\UE_Source\UE_5.7.4`
```

阻塞规则：若实际使用的引擎分发不支持 `TargetType.Server`，立即停止Server验收并修复引擎基线，不得用Listen Server冒充Dedicated Server通过。

### C. 技术工程名一致性

- [x] `.uproject`模块名为 `PolygonScifiWorlds`。
- [x] 主Source目录为 `Source/PolygonScifiWorlds/`。
- [x] Build.cs类名为 `PolygonScifiWorlds`。
- [x] Game、Editor、Server Target的类名与文件名符合UBT规则。
- [x] 主模块使用 `IMPLEMENT_PRIMARY_GAME_MODULE`，模块名正确。
- [x] 产品显示名 `ScifiWorlds` 与技术工程名 `PolygonScifiWorlds` 的区别已记录，无需为了显示名重命名C++工程。

证据：

```text
核验结果：2026-07-22 已核验。`PolygonScifiWorlds.uproject`、`Source/PolygonScifiWorlds/`、`PolygonScifiWorlds.Build.cs`、现有 Game/Editor Target 类及 `IMPLEMENT_PRIMARY_GAME_MODULE` 均使用 `PolygonScifiWorlds`；产品显示名保持 `ScifiWorlds`。
是否发生重命名：否（现有技术工程名已一致）。`PolygonScifiWorldsServer.Target.cs` 已于 2026-07-23 新增，类名为 `PolygonScifiWorldsServerTarget`。
```

### D. 插件配置

- [x] 在 `PolygonScifiWorlds.uproject` 中显式启用 `GameplayAbilities`。
- [x] 在 `PolygonScifiWorlds.uproject` 中显式启用 `EnhancedInput`。
- [x] 保留GameplayAbilities插件声明的必要依赖，不手工复制引擎插件到项目。
- [x] 核验 `BpGeneratorUltimate` 只包含Editor模块，不形成Server Runtime依赖。
- [x] 核验 `PythonScriptPlugin` 不形成Server Runtime依赖。
- [x] 打开 `.uproject` 后没有缺失插件或版本不兼容提示。

证据：

```text
.uproject差异：已显式启用 `GameplayAbilities`、`EnhancedInput`；`PythonScriptPlugin` 限制为 `TargetAllowList: [Editor]`。
插件加载验证：`PolygonScifiWorldsEditor Development Win64` 于 2026-07-23 构建成功；项目随后在源码 Editor 中正常打开，未出现 `GameplayAbilities`、`EnhancedInput`、`BpGeneratorUltimate` 或版本不兼容提示。
```

### E. Build.cs配置

- [x] 保留 `PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs`。
- [x] 使用 UE 5.7 的 `IWYUSupport = IWYUSupport.Full`，并修正因此暴露的直接Include问题。
- [x] 保留必要Public依赖：`Core`、`CoreUObject`、`Engine`、`InputCore`。
- [x] 将以下模块加入 `PrivateDependencyModuleNames`：
  - [x] `GameplayAbilities`
  - [x] `GameplayTags`
  - [x] `GameplayTasks`
  - [x] `EnhancedInput`
- [x] 没有把仅在Private代码使用的模块提升为Public依赖。
- [x] 没有提前加入 `OnlineSubsystem`、EOS或Steam依赖。
- [x] Runtime模块没有引用 `UnrealEd` 或其他Editor-only模块。

说明：当M03的Public头文件实际公开GAS类型时，再按真实Include关系评估哪些依赖需要提升为Public。

证据：

```text
Build.cs差异：使用 UE 5.7 的 `IWYUSupport = IWYUSupport.Full`；将 `GameplayAbilities`、`GameplayTags`、`GameplayTasks`、`EnhancedInput` 加入 Private 依赖。
IWYU处理摘要：UE 5.7 已废弃 `bEnforceIWYU`，改用 `IWYUSupport.Full`；当前模块没有上述模块的 Include，待三类 Target 编译时确认无新增 IWYU 错误。
```

### F. Target配置

- [x] `PolygonScifiWorlds.Target.cs` 的类型为 `TargetType.Game`。
- [x] `PolygonScifiWorldsEditor.Target.cs` 的类型为 `TargetType.Editor`。
- [x] 新建 `Source/PolygonScifiWorldsServer.Target.cs`。
- [x] Server Target类名为 `PolygonScifiWorldsServerTarget`。
- [x] Server Target类型为 `TargetType.Server`。
- [x] 三个Target统一使用 `BuildSettingsVersion.V6`。
- [x] 三个Target统一使用 `EngineIncludeOrderVersion.Unreal5_7`。
- [x] 三个Target都只加入 `PolygonScifiWorlds` 主模块。
- [x] M01不新增Client Target，除非路线图通过文档变更明确扩大范围。

证据：

```text
Game Target：`PolygonScifiWorldsTarget`，`TargetType.Game`，V6 / Unreal5_7
Editor Target：`PolygonScifiWorldsEditorTarget`，`TargetType.Editor`，V6 / Unreal5_7
Server Target：`PolygonScifiWorldsServerTarget`，`TargetType.Server`，V6 / Unreal5_7
项目文件生成：2026-07-23 Succeeded；UBT 已成功解析全部 Target
```

### G. 地图与DS基础配置

- [ ] 确认 `/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket` 能在当前授权环境中加载。
- [ ] 在 `DefaultEngine.ini` 的 `GameMapsSettings` 中配置 `ServerDefaultMap`。
- [ ] `ServerDefaultMap` 指向有效地图，不依赖Level Blueprint执行权威规则。
- [ ] 确认服务器启动时不要求LocalPlayer、Viewport、HUD、音频设备或客户端专属对象。
- [ ] 确认配置和日志中没有Token、密钥或其他秘密。

建议配置：

```ini
[/Script/EngineSettings.GameMapsSettings]
ServerDefaultMap=/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket.Demo_BlackMarket
```

证据：

```text
ServerDefaultMap：TBD
地图加载结果：TBD
```

### H. Rider工程生成

- [ ] 关闭Unreal Editor和Live Coding。
- [ ] 使用已经注册的源码引擎重新生成项目文件。
- [ ] 从Rider打开 `.uproject`，能够看到Game、Editor、Server Target。
- [ ] Rider使用的MSBuild、Windows SDK和C++工具链均可用。
- [ ] Rider没有 `No loaded projects`、无效 `.uproject` 或旧引擎关联错误。
- [ ] 不提交 `.idea/`、`.sln`、Rider缓存或其他IDE本地设置。

证据：

```text
项目文件生成结果：TBD
Rider Target列表：TBD
```

### I. 固化构建命令

- [ ] 新建或记录可重复执行的构建入口，例如 `Scripts/Build/BuildTargets.ps1`。
- [ ] 引擎根目录通过参数传入，不硬编码到提交文件。
- [ ] 构建脚本失败时返回非零退出码。
- [ ] 构建脚本按 Editor → Game → Server 顺序执行。
- [ ] 构建日志写入被Git忽略的目录，或只在验证摘要中记录关键结果。

命令骨架：

```powershell
$M01EngineRoot = 'TBD_SOURCE_ENGINE_ROOT'
$M01ProjectFile = 'TBD_PROJECT_FILE'
$M01BuildBat = Join-Path $M01EngineRoot 'Engine\Build\BatchFiles\Build.bat'

& $M01BuildBat PolygonScifiWorldsEditor Win64 Development "-Project=$M01ProjectFile" -WaitMutex -NoHotReloadFromIDE
& $M01BuildBat PolygonScifiWorlds Win64 Development "-Project=$M01ProjectFile" -WaitMutex -NoHotReloadFromIDE
& $M01BuildBat PolygonScifiWorldsServer Win64 Development "-Project=$M01ProjectFile" -WaitMutex -NoHotReloadFromIDE
```

执行者应根据实际源码引擎确认命令参数，不得在命令失败时跳过相应Target。

### J. 三类Target编译

- [ ] Development Editor首次增量构建通过。
- [ ] Development Game首次增量构建通过。
- [ ] Development Server首次增量构建通过。
- [ ] 没有新增编译警告、UHT错误、IWYU错误或链接错误。
- [ ] 关闭Editor和Live Coding后，仅清理项目的 `Binaries/` 与 `Intermediate/`。
- [ ] 不删除 `Content/`、`Config/`、`Source/`、`.git/` 或引擎目录。
- [ ] 重新生成项目文件。
- [ ] Development Editor干净构建通过。
- [ ] Development Game干净构建通过。
- [ ] Development Server干净构建通过。

证据：

| Target | 配置 | 结果 | 日志/摘要 |
|---|---|---|---|
| PolygonScifiWorldsEditor | Development Win64 | TBD | TBD |
| PolygonScifiWorlds | Development Win64 | TBD | TBD |
| PolygonScifiWorldsServer | Development Win64 | TBD | TBD |

### K. Server Cook与Stage

- [ ] 使用源码引擎的RunUAT执行Server Build/Cook/Stage。
- [ ] 明确使用Win64 Server平台和Development配置。
- [ ] 明确指定需要Cook的服务器地图。
- [ ] Cook过程没有缺失资产、未授权插件、Shader或配置错误。
- [ ] Stage产物包含可启动的Server可执行文件和Cooked Content。
- [ ] 产物目录位于仓库之外或已被Git忽略。

命令骨架：

```powershell
$M01RunUat = Join-Path $M01EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$M01ArtifactRoot = 'TBD_ARTIFACT_DIRECTORY'

& $M01RunUat BuildCookRun "-project=$M01ProjectFile" -noP4 -server -noclient -serverplatform=Win64 -serverconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$M01ArtifactRoot" "-map=/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket"
```

证据：

```text
RunUAT命令：TBD
Cook/Stage结果：TBD
Server可执行文件：TBD
```

### L. Dedicated Server启动验证

- [ ] 新建或记录可重复启动入口，例如 `Scripts/Run/StartLocalDS.ps1`。
- [ ] 使用Stage后的Server可执行文件启动，不使用Listen Server冒充。
- [ ] 显式指定地图、`-port=7777`、`-log`、`-unattended` 和 `-NoSound`。
- [ ] Server进程不创建游戏窗口或本地玩家。
- [ ] 日志显示正确地图加载完成。
- [ ] 日志显示网络驱动已监听预期端口。
- [ ] 日志中没有Fatal、Assertion、模块加载失败或Cooked Content缺失。
- [ ] 使用系统网络工具确认Server进程监听7777端口。
- [ ] 正常终止Server进程，并确认没有遗留后台进程。

M01可选烟雾测试，不作为路线图硬门槛：

- [ ] 一个客户端可以执行 `open 127.0.0.1:7777` 并到达服务器地图。
- [ ] 第二个客户端可以连接，且不会导致服务器崩溃。

证据：

```text
启动命令：TBD
Server PID：TBD
地图加载日志：TBD
端口监听证据：TBD
退出码/关闭结果：TBD
可选客户端连接结果：TBD
```

### M. 失败与边界验证

- [ ] 错误引擎关联能够被明确诊断，而不是生成无效Rider项目。
- [ ] 使用不支持Server Target的引擎时，流程明确失败并提示切换源码引擎。
- [ ] 无效地图参数导致清晰错误和非成功验收结果。
- [ ] 端口7777被占用时能够识别绑定失败，不误判DS启动成功。
- [ ] 缺少Cooked Content时能够识别错误，不用Editor进程掩盖问题。
- [ ] Editor-only插件不会进入Server Runtime依赖。
- [ ] 重复执行构建和启动脚本不会依赖上一次残留进程或临时文件。

### N. 文档、差异和提交

- [ ] 更新 `Docs/Systems/M01_ProjectBuildFoundation.md` 的需求追踪与验证结果。
- [ ] 更新 `.agents/ue-project-context.md` 的引擎安装类型、插件、模块依赖和Target信息。
- [ ] 如有新技术债务，登记到TDD或风险文档，不保留永久TODO掩盖问题。
- [ ] 更新 `Docs/07_DevelopmentRoadmap.md`：M01 → `Completed`，当前模块 → M02。
- [ ] 在路线图完成记录中使用 `milestone/m01` 记录完成点。
- [ ] 执行 `git diff --check`。
- [ ] 检查 `git status`，确保未提交Binaries、Intermediate、Saved、DerivedDataCache、IDE设置和构建产物。
- [ ] 检查 `.uasset`、`.umap` 仍由Git LFS管理。
- [ ] 检查没有意外提交商业/Fab原始资产。
- [ ] 创建提交：`build: establish project and dedicated server foundation`。
- [ ] 创建Tag：`milestone/m01`。
- [ ] 推送分支与Tag，并创建Pull Request。

证据：

```text
M01 Commit：TBD
M01 Tag：milestone/m01
Pull Request：TBD
最终git status：TBD
```

## 6. 需求追踪

| ID | 需求 | 对应步骤 | 验收证据 | 状态 |
|---|---|---|---|---|
| FR-01 | 技术工程名应统一为 `PolygonScifiWorlds` | C | 名称核验记录（2026-07-22） | Passed |
| FR-02 | 项目应显式配置GAS和Enhanced Input | D、E | `.uproject`与Build.cs差异、源码 Editor 加载验证（2026-07-23） | Passed |
| FR-03 | 项目应提供Game、Editor、Server Target | F | 三个Target文件与项目文件生成验证（2026-07-23） | Passed |
| FR-04 | 三类Development Target均应编译通过 | J | 三份构建结果 | Not Started |
| FR-05 | DS应能够加载地图并监听端口 | G、K、L | Cook日志、启动日志和端口证据 | Not Started |
| NFR-01 | 构建和启动过程应可重复执行 | I、K、L | 脚本或完整命令记录 | Not Started |
| NFR-02 | Server不得依赖Editor或客户端专属对象 | D、E、G、L | 依赖审计与启动日志 | Not Started |
| NFR-03 | 仓库不得包含生成物、IDE设置或未授权资产 | A、N | Git差异审计 | Not Started |
| EC-01 | 错误引擎关联必须能够被诊断 | B、M | 关联核验记录 | Not Started |
| EC-02 | 不支持Server Target的引擎不得通过验收 | B、M | Server构建结果 | Not Started |
| EC-03 | 地图、Cook或端口错误不得误判启动成功 | K、L、M | 失败测试摘要 | Not Started |

## 7. 最终签署

- [ ] M01设计文档为 `Approved`。
- [ ] FR/NFR/EC均有通过证据。
- [ ] Game、Editor、Server干净构建全部通过。
- [ ] DS为真正的headless Server Target进程。
- [ ] DS正确加载地图并监听端口。
- [ ] 没有新增未登记技术债务、循环依赖或隐藏耦合。
- [ ] 项目SSOT和路线图已同步。
- [ ] M01提交、Tag和PR均已建立。

```text
签署人：TBD
完成日期：TBD
结论：Passed / Failed
未完成项：TBD
```

## Execution Evidence Addendum (2026-07-23)

| Validation item | Result | Evidence |
|---|---|---|
| Development Editor, Game, and Server targets | Passed | All three source-engine `Development Win64` targets returned `Result: Succeeded`. |
| Server default map | Passed | `ServerDefaultMap` resolves to `/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket.Demo_BlackMarket`. |
| Cook / Stage / Archive | Passed | `BuildCookRun` completed with `BUILD SUCCESSFUL` and `AutomationTool exiting with ExitCode=0`. |
| Staged DS startup | Passed | The archived Server process loaded `Demo_BlackMarket` and logged `IpNetDriver listening on port 7777`. |
| OS port verification | Passed | `Get-NetUDPEndpoint -LocalPort 7777` reported the server listener; the port was released after shutdown. |
| Local client smoke test | Passed (optional) | A PIE client logged `Welcomed by server` and `UPendingNetGame::TravelCompleted` for `127.0.0.1:7777`. |
| Port-conflict boundary | Passed | A second `StartLocalDS.ps1` invocation rejected occupied UDP 7777 before creating a server process. |
| Invalid engine-root boundary | Passed | `BuildTargets.ps1` rejected a nonexistent `EngineRoot` before any build began. |
| Invalid-map boundary | Passed after script hardening | UE's raw `BuildCookRun` accepted a nonexistent map and made an incomplete archive; `CookServer.ps1` now requires a `/Game/...` long package name that resolves to an existing project `.umap`, and rejects a missing map before AutomationTool starts. |
| Reproducible entry points | Passed | `BuildTargets.ps1`, `CookServer.ps1`, and `StartLocalDS.ps1` use parameters for engine, project, archive, map, and port; no personal absolute path is embedded. |

Known non-blocking observation: the purchased `BpGeneratorUltimate` plugin emits third-party dependency-declaration warnings during Editor builds. Its modules are Editor-only and did not enter the staged Server runtime.

## 8. 交接给实现工具的最小提示

```text
请严格按照 Docs/Systems/M01_ProjectBuildFoundation_Checklist.md 实现M01。
先读取仓库 AGENTS.md、.agents/ue-project-context.md、Docs/Engineering/TechnicalStandards.md 和 Docs/07_DevelopmentRoadmap.md。
只实现M01范围，不提前实现M02、M03、M04或M15。
逐项更新清单中的状态和证据；遇到源码引擎、Server Target、插件兼容或Cook阻塞时停止并报告，不得用Listen Server或Editor模拟结果冒充通过。
保留用户现有未跟踪文件，不执行破坏性Git操作。
```
