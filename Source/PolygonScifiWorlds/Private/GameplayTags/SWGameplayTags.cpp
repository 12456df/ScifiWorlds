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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Skill1, "Ability.Input.Skill1", "第一个主动技能槽的输入路由。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Skill2, "Ability.Input.Skill2", "第二个主动技能槽的输入路由。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Input_Skill3, "Ability.Input.Skill3", "第三个主动技能槽的输入路由。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_ActiveSkill, "Ability.Type.ActiveSkill", "可由技能点升级的主动技能分类。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fail_NoMana, "Ability.Fail.NoMana", "技能 Mana 不足。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fail_NoStamina, "Ability.Fail.NoStamina", "疾跑所需的 Stamina 不足。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fail_InvalidCostData, "Ability.Fail.InvalidCostData", "主动技能蓝耗为正但未配置 Cost GE。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fail_NoCharges, "Ability.Fail.NoCharges", "技能没有可用充能。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fail_InvalidTarget, "Ability.Fail.InvalidTarget", "技能目标未通过服务器校验。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fail_InvalidLevelData, "Ability.Fail.InvalidLevelData", "技能等级数据无效。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_PortalSphere, "Ability.Skill.PortalSphere", "PortalSphere 主动技能身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_PortalSphere, "Cooldown.Ability.PortalSphere", "PortalSphere 冷却状态。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Shield, "Ability.Skill.Shield", "Shield 主动技能身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_Shield, "Cooldown.Ability.Shield", "Shield 冷却状态。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_AoeBuff, "Ability.Skill.AoeBuff", "以施法者为中心、按敌我施加不同状态效果的主动技能身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_AoeBuff, "Cooldown.Ability.AoeBuff", "AoeBuff 冷却/充能状态。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Fire, "Ability.Weapon.Fire", "开火能力身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Aim, "Ability.Weapon.Aim", "瞄准能力身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Reload, "Ability.Weapon.Reload", "换弹能力身份。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Movement_Sprint, "Ability.Movement.Sprint", "疾跑能力身份。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Firing, "State.Weapon.Firing", "当前正在执行开火循环。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Aiming, "State.Weapon.Aiming", "当前处于瞄准。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Reloading, "State.Weapon.Reloading", "当前正在换弹。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Sprinting, "State.Movement.Sprinting", "当前处于疾跑。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Ability_Targeting, "State.Ability.Targeting", "角色正等待确认或取消目标预览。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Weapon_Fire, "Event.Weapon.Fire", "武器 Montage 发射时刻事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Weapon_ProjectileImpact, "Event.Weapon.ProjectileImpact", "弹丸权威命中事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PortalSphere_Spawn, "Event.Ability.PortalSphere.Spawn", "PortalSphere 施法动作生成弹体事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Shield_Spawn, "Event.Ability.Shield.Spawn", "Shield 施法动作生成屏障事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_AoeBuff_Apply, "Event.Ability.AoeBuff.Apply", "AoeBuff 施法 Montage 到达结算帧事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Fire, "GameplayCue.Weapon.Fire", "开火纯表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Reload, "GameplayCue.Weapon.Reload", "换弹纯表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Projectile_Impact, "GameplayCue.Projectile.Impact", "弹丸命中纯表现 Cue。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Team_None, "State.Team.None", "未分队或中立单位的队伍镜像。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Team_TeamA, "State.Team.TeamA", "Team A 的队伍镜像。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Team_TeamB, "State.Team.TeamB", "Team B 的队伍镜像。");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Physical, "Damage.Type.Physical", "物理伤害通道。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Magical, "Damage.Type.Magical", "魔法伤害通道。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_True, "Damage.Type.True", "真实伤害通道。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage, "Effect.Damage", "造成伤害的 Gameplay Effect 资产 Tag。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage_Raw, "SetByCaller.Damage.Raw", "伤害生产者在服务器计算完成的未减免伤害。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Experience, "SetByCaller.Experience", "一次性经验奖励值。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Healing, "SetByCaller.Healing", "服务器权威的瞬时治疗量。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Ability_Cooldown, "SetByCaller.Ability.Cooldown", "主动技能提交时写入的冷却时长。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Ability_ManaCost, "SetByCaller.Ability.ManaCost", "主动技能提交时写入的 Mana 消耗。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Ability_Duration, "SetByCaller.Ability.Duration", "主动技能提交时写入的持续时间。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Buff_MovementSpeedDelta, "SetByCaller.Buff.MovementSpeedDelta", "加速 Buff 对 MovementSpeedMultiplier 的加法增量。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Buff_HealthPerSecond, "SetByCaller.Buff.HealthPerSecond", "持续回血 Buff 每个周期恢复的生命值。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "单位已死亡。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "单位暂时免疫伤害。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Buff_Speed, "State.Buff.Speed", "单位正受到加速 Buff 影响。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Buff_Heal, "State.Buff.Heal", "单位正受到 Heal Buff 影响。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Debuff_Stunned, "State.Debuff.Stunned", "首版眩晕 Debuff 的表现协议；当前无眩晕动作，因此实际效果为减速，不阻断移动或技能。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Debuff_Poisoned, "State.Debuff.Poisoned", "单位正受到中毒持续伤害影响。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Behavior_SurviveDeath, "Ability.Behavior.SurviveDeath", "死亡时不应被取消的能力。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_DamageResolved, "Event.Combat.DamageResolved", "一次伤害结算完成事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_Death, "Event.Combat.Death", "服务器确认死亡事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_MinionAttack, "Event.Combat.MinionAttack", "服务器小兵攻击目标事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_MinionAttackHit, "Event.Combat.MinionAttack.Hit", "小兵攻击 Montage 的服务器命中时点事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_StructureAttack, "Event.Combat.StructureAttack", "服务器防御结构攻击目标事件。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Hit, "GameplayCue.Combat.Hit", "受击纯表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Death, "GameplayCue.Combat.Death", "死亡纯表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Player_LevelUp, "GameplayCue.Player.LevelUp", "玩家等级提升的一次性世界表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Buff_Speed, "GameplayCue.Buff.Speed", "加速 Buff 的持续表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Buff_Heal, "GameplayCue.Buff.Heal", "Heal Buff 的持续表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Debuff_Stun, "GameplayCue.Debuff.Stun", "眩晕 Debuff 的持续表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Debuff_Poison, "GameplayCue.Debuff.Poison", "中毒 Debuff 的持续表现 Cue。");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Ability_AoeBuff_Cast, "GameplayCue.Ability.AoeBuff.Cast", "AoeBuff 在施法者位置执行的一次性范围施法表现 Cue。");
}
