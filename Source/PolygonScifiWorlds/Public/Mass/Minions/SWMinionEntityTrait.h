#pragma once

#include "CoreMinimal.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassEntityTraitBase.h"
#include "SWMinionEntityTrait.generated.h"

struct FMassEntityTemplateBuildContext;

/**
 * 正式小兵 EntityConfig 的基础组成。
 * 该 Trait 只定义稳定 ECS 结构与同 Archetype 的不可变参数，绝不决定波次、路线、队伍或具体出生位置。
 */
UCLASS(DisplayName = "SW Minion Entity Trait")
class POLYGONSCIFIWORLDS_API USWMinionEntityTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** 同类小兵的只读参数；Factory 运行时不会改写。 */
	UPROPERTY(EditAnywhere, Category = "Minion")
	FSWMinionArchetypeSharedFragment ArchetypeParameters;

	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
