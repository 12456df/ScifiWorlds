// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Team/SWTeamTypes.h"
#include "SWShieldBarrier.generated.h"

class APawn;
class UBoxComponent;
class UPrimitiveComponent;

/** 服务器权威的单向投射物屏障；默认尺寸由蓝图资产定义，C++ 仅复制并施加属性倍率。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API ASWShieldBarrier : public AActor
{
	GENERATED_BODY()

public:
	ASWShieldBarrier();

	/** 仅由服务器在 Deferred Spawn 后调用。AreaScale 仅作用于屏障的 Y、Z 尺寸，X 厚度保持蓝图默认值。 */
	bool InitializeShieldAuthority(APawn* InInstigatorPawn, ESWTeamId InTeamId, float InDurationSeconds, float InAreaScale);

	/** 返回蓝图类默认的未缩放吸收盒半尺寸；预览与权威屏障必须共享此基准。 */
	UFUNCTION(BlueprintPure, Category = "SW|Shield")
	FVector GetDefaultAbsorptionBoxExtent() const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SW|Shield")
	TObjectPtr<UBoxComponent> AbsorptionVolume;

	/** 供蓝图同步缩放 Niagara、材质等纯表现组件；碰撞缩放始终由 C++ Actor Scale 驱动。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SW|Shield")
	void BP_OnShieldAreaScaleChanged(float InAreaScale);

	/**
	 * 将服务端权威的持续时间同步给表现蓝图。
	 * 蓝图只据此编排创建/结束 Timeline，不得修改屏障的真实存活时间。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SW|Shield")
	void BP_OnShieldDurationInitialized(float InDurationSeconds, float InEndServerTimeSeconds);

private:
	UFUNCTION()
	void OnAbsorptionVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_AreaScale();

	UFUNCTION()
	void OnRep_EndServerTimeSeconds();

	bool ShouldAbsorbProjectileAuthority(AActor* ProjectileActor) const;
	void ConfigureAuthorityCollision();
	FVector GetAreaScaleVector() const;

	bool bInitialized = false;
	ESWTeamId ShieldTeamId = ESWTeamId::None;

	/** 复制给客户端以保持同一份屏障视觉倍率。 */
	UPROPERTY(ReplicatedUsing = OnRep_AreaScale)
	float AreaScale = 1.f;

	/** 仅用于客户端视觉编排；真实销毁仍由服务端 SetLifeSpan 权威控制。 */
	UPROPERTY(Replicated)
	float DurationSeconds = 0.f;

	/** 服务端世界时间中的到期时刻；客户端据此扣除网络到达延迟后再播放结束表现。 */
	UPROPERTY(ReplicatedUsing = OnRep_EndServerTimeSeconds)
	float EndServerTimeSeconds = 0.f;
};
