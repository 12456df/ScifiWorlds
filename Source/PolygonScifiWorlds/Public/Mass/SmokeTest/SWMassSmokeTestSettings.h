#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SWMassSmokeTestSettings.generated.h"

class UMassEntityConfigAsset;

/**
 * M10 技术冒烟的开发配置。
 * EntityConfig 由编辑器资产提供，Subsystem 只在服务器世界读取并使用该引用。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SW Mass Smoke Test"))
class POLYGONSCIFIWORLDS_API USWMassSmokeTestSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** P0 使用的最小 EntityConfig；未配置时拒绝创建。 */
	UPROPERTY(Config, EditAnywhere, Category = "Mass|Smoke Test")
	TSoftObjectPtr<UMassEntityConfigAsset> SmokeTestEntityConfig;

	virtual FName GetCategoryName() const override;
};
