// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWShopZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/**
 * 服务器权威的交易区域。
 * 它只维护进入/离开资格，不创建 UI，也不决定价格或交易结果。
 */
UCLASS(Blueprintable)
class POLYGONSCIFIWORLDS_API ASWShopZone : public AActor
{
	GENERATED_BODY()

public:
	ASWShopZone();

	UBoxComponent* GetTradeVolume() const { return TradeVolume; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleTradeVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTradeVolumeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TradeVolume;
};
