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

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Fire, "Ability.Input.Fire", "开火输入。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Aim, "Ability.Input.Aim", "瞄准输入。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Reload, "Ability.Input.Reload", "换弹输入。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Sprint, "Ability.Input.Sprint", "疾跑输入。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Fire, "Ability.Weapon.Fire", "开火能力身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Aim, "Ability.Weapon.Aim", "瞄准能力身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Reload, "Ability.Weapon.Reload", "换弹能力身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Movement_Sprint, "Ability.Movement.Sprint", "疾跑能力身份。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Firing, "State.Weapon.Firing", "当前正在执行开火循环。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Aiming, "State.Weapon.Aiming", "当前处于瞄准。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Reloading, "State.Weapon.Reloading", "当前正在换弹。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Sprinting, "State.Movement.Sprinting", "当前处于疾跑。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Weapon_Fire, "Event.Weapon.Fire", "武器 Montage 发射时刻事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Weapon_ProjectileImpact, "Event.Weapon.ProjectileImpact", "弹丸权威命中事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Fire, "GameplayCue.Weapon.Fire", "开火纯表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Reload, "GameplayCue.Weapon.Reload", "换弹纯表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Projectile_Impact, "GameplayCue.Projectile.Impact", "弹丸命中纯表现 Cue。");
}
