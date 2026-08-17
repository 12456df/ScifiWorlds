#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "SWMassSmokeTestTrait.generated.h"

struct FMassEntityTemplateBuildContext;

/**
 * 将 M10 冒烟测试所需的最小 Fragment/Tag 加入 EntityConfig。
 * 资产只决定 Entity 的组成，不负责任何生成时机或网络权威。
 */
UCLASS(DisplayName = "SW Mass Smoke Test Trait")
class POLYGONSCIFIWORLDS_API USWMassSmokeTestTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
