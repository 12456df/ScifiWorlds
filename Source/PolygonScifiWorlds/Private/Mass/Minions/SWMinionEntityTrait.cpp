#include "Mass/Minions/SWMinionEntityTrait.h"

#include "MassCommonFragments.h"
#include "MassActorSubsystem.h"
#include "MassEntityManager.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionEntityTrait)

void USWMinionEntityTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	// 所有正式小兵均拥有 Transform；M10 Factory 写入出生 Transform，M11 移动 Processor 更新它。
	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment<FSWMinionIdentityFragment>();
	BuildContext.AddFragment<FSWMinionTeamFragment>();
	BuildContext.AddFragment<FSWMinionLaneFragment>();
	BuildContext.AddFragment<FSWMinionSpatialFragment>();
	// StateTree 与后续 Processor 通过该 Fragment 共享行为输出，不在 Actor 上保存平行状态机。
	BuildContext.AddFragment<FSWMinionIntentFragment>();
	BuildContext.AddFragment<FSWMinionTargetFragment>();
	BuildContext.AddFragment<FSWMinionCombatFragment>();
	BuildContext.AddFragment<FSWMinionLeashFragment>();
	BuildContext.AddFragment<FSWMinionTimingFragment>();
	// M10-5：保存服务器权威 Character Actor，并维护 Actor 到 Entity 的反向查询。
	BuildContext.AddFragment<FMassActorFragment>();

	const FConstSharedStruct Parameters = EntityManager.GetOrCreateConstSharedFragment(ArchetypeParameters);
	BuildContext.AddConstSharedFragment(Parameters);
}
