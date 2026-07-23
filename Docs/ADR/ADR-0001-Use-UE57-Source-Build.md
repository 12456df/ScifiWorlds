# ADR-0001：使用 UE 5.7.4 源码构建作为项目构建基线

**状态：** Accepted
**日期：** 2026-07-23
**决策者：** `12456df`

## 背景

ScifiWorlds 需要从 M01 开始交付真正的 Dedicated Server Target。预编译 Launcher 引擎不能作为 Server Target 验收环境；项目需要一个可追溯、可复现并支持 Game、Editor 与 Server 三类 Target 的 UE 5.7 基线。

## 决策

项目使用 Epic 官方 `5.7.4-release` 源码构建作为 M01 的构建与 Dedicated Server 验收基线：commit `260bb2e1c5610b31c63a36206eedd289409c5f11`，`Build.version` 为 5.7.4（CompatibleChangelist `47537391`）。项目通过已注册的引擎关联标识解析该引擎。

仓库中的构建脚本不记录机器绝对路径；调用者必须通过 `-EngineRoot` 参数提供本机源码引擎根目录。所有 Server 验收都使用 `TargetType.Server` 产物，不能用 Listen Server 或 Editor 运行结果替代。

## 备选方案

| 方案 | 优点 | 缺点 | 未选择原因 |
|---|---|---|---|
| Launcher 预编译 UE 5.7 | 安装简单 | 不能作为源码 Server Target 的可靠验收基线 | 不满足 M01 Dedicated Server 目标 |
| 引擎源码构建 | 支持完整 Target 构建，版本可追溯 | 首次构建成本较高 | 已选择 |
| 将引擎复制进项目仓库 | 可与项目一同取得 | 仓库体积与许可边界不可接受 | 不满足仓库管理约束 |

## 后果

### 正面

- Game、Editor 与 Dedicated Server 使用同一可验证的引擎基线。
- 构建脚本可在任何具备同版本源码引擎的授权环境中复用。

### 负面与代价

- 每位开发者或 CI 环境都必须准备、注册并编译源码引擎。
- 引擎本地插件和生成物不属于项目 Git 内容，必须与项目仓库隔离。

## 验证与复审

- 验证方式：使用 `Scripts/Build/BuildTargets.ps1` 完成三类 Development Target 构建，并用 `RunUAT` Cook/Stage 后启动 Server 进程。
- 复审条件：升级 UE 主/次版本、改变服务器平台或引擎来源时。
