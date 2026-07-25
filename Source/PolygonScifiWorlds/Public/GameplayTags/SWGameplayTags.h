// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * ScifiWorlds 原生 Gameplay Tag 的唯一声明处。
 *
 * 契约见 Docs/Systems/M03_GASCoreFramework.md：代码引用项目内固定 Tag 必须使用这里声明的原生
 * Tag，禁止散落的 RequestGameplayTag(TEXT("...")) 字符串。
 *
 * M03 仅注册稳定的根 Tag，不为未来内容预留空分类；具体叶子 Tag 在首次被某个已设计模块消费时
 * 加入，并同步写入该模块设计文档。
 */
namespace SWGameplayTags
{
	// 技能身份、状态与输入 Tag 的根。
	POLYGONSCIFIWORLDS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);

	// 技能冷却阻塞 Tag 的根。
	POLYGONSCIFIWORLDS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown);

	// 角色可被查询的 Gameplay 状态根。
	POLYGONSCIFIWORLDS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State);

	// Gameplay Event 路由根。
	POLYGONSCIFIWORLDS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event);

	// 运行时效果幅值键的根。
	POLYGONSCIFIWORLDS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller);

	// 仅表现用途 Cue 的根。
	POLYGONSCIFIWORLDS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue);
}
