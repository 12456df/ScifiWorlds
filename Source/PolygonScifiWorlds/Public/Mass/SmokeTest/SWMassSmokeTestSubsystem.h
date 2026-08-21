#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "SWMassSmokeTestSubsystem.generated.h"

/**
 * M10 P0 的服务器权威 Mass 冒烟入口。
 * 该类只管理测试 Entity 的创建、统计与销毁，不能作为正式兵线或小兵生命周期入口。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMassSmokeTestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 仅 Dedicated Server、Listen Server 或 Standalone 可以批量创建测试 Entity。 */
	bool SpawnSmokeTestEntitiesAuthority(int32 Count = 20);

	/** 返回当前 World 中仍有效的测试 Entity 数量。 */
	int32 GetActiveSmokeTestEntityCount() const;

	/** 仅权威 World 销毁由本 Subsystem 创建的全部测试 Entity。 */
	void DestroySmokeTestEntitiesAuthority();

	virtual void Deinitialize() override;

private:
	bool IsAuthorityWorld() const;

	/** 仅保存本 World、本 Subsystem 创建的 Handle，绝不跨 World 使用。 */
	TArray<FMassEntityHandle> SmokeTestEntities;
};
