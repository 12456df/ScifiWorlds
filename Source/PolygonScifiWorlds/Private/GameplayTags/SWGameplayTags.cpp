// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "GameplayTags/SWGameplayTags.h"

// 使用 UE_DEFINE_GAMEPLAY_TAG_COMMENT 定义的原生 Tag 会在静态初始化阶段自动向
// GameplayTagsManager 注册，无需额外的手动初始化调用。
namespace SWGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability, "Ability", "技能身份、状态与输入 Tag 的根。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown, "Cooldown", "技能冷却阻塞 Tag 的根。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "State", "角色可被查询的 Gameplay 状态根。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event, "Event", "Gameplay Event 路由根。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller, "SetByCaller", "运行时效果幅值键的根。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue, "GameplayCue", "仅表现用途 Cue 的根。");
}
