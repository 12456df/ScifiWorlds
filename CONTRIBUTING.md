# Contributing

## 分支

- `main`：可构建的主线。
- `feature/<short-name>`：功能开发。
- `fix/<short-name>`：缺陷修复。
- `docs/<short-name>`：纯文档变更。

分支应保持短生命周期；重要改动通过 Pull Request 合并。

## 提交信息

采用简洁的 Conventional Commits 风格：

- `feat: add ...`
- `fix: correct ...`
- `docs: define ...`
- `refactor: restructure ...`
- `test: add ...`
- `build: update ...`

## Unreal 资产

- `.uasset` 和 `.umap` 必须由 Git LFS 管理。
- 编辑共享地图或核心蓝图前，先确认没有其他人同时修改。
- 不提交 DerivedDataCache、Intermediate、Saved、Binaries 或 IDE 本地设置。
- 第三方资产必须登记来源和许可证；商业原始资产不得放入公开仓库。

## Pull Request 检查

- [ ] 变更目标和范围清晰
- [ ] Development Editor 编译通过
- [ ] 受影响功能经过验证
- [ ] 没有意外提交生成文件或未授权资产
- [ ] 文档、测试和变更记录已同步更新
