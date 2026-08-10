// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SWProjectileAbsorptionInterface.generated.h"

/** 声明投射物可被屏障等防御性世界对象吸收。 */
UINTERFACE(BlueprintType)
class POLYGONSCIFIWORLDS_API USWProjectileAbsorptionInterface : public UInterface
{
	GENERATED_BODY()
};

class POLYGONSCIFIWORLDS_API ISWProjectileAbsorptionInterface
{
	GENERATED_BODY()

public:
	/** 仅由服务器调用；返回该投射物当前是否允许被屏障吸收。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	bool CanBeAbsorbedByShield() const;

	/** 仅由服务器调用；实现者必须结束自身的权威投射物生命周期。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	void AbsorbByShieldAuthority(AActor* ShieldActor);
};
