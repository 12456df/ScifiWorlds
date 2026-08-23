// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Targeting/SWCombatTargetRegistrySubsystem.h"
#include "SWStructureTargetingComponent.generated.h"

class AActor;
class ASWDefenseStructure;
class UPrimitiveComponent;

/** 当前权威目标改变时广播；仅服务器 AIController 消费，Actor 不被强引用。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FSWOnStructureTargetChanged, TWeakObjectPtr<AActor> /* NewTarget */);

/**
 * 静态防御结构的服务器端范围候选与目标选择组件。
 * Overlap 只维护候选加速表；每次选择均重新验证 Combat、Team、死亡、Targetable 与当前范围。
 */
UCLASS(ClassGroup = (Structure), meta = (BlueprintSpawnableComponent))
class POLYGONSCIFIWORLDS_API USWStructureTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USWStructureTargetingComponent();

	/** 由拥有者在服务器完成 CombatRange 配置后调用；可重复调用且只会绑定一次。 */
	void InitializeAuthority();

	/** 服务器当前目标的只读查询；如目标已失效会先同步重选。 */
	AActor* GetCurrentTargetAuthority();

	/** 服务器目标改变事件；AIController 在后续步骤订阅并写入 Blackboard。 */
	FSWOnStructureTargetChanged& GetOnTargetChangedDelegate() { return OnTargetChanged; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class ETargetInvalidReason : uint8
	{
		General,
		Dead,
	};

	struct FCandidateRecord
	{
		TWeakObjectPtr<AActor> Actor;
		uint32 TargetId = 0;
		ESWMinionTargetCategory Category = ESWMinionTargetCategory::None;
		uint64 EntrySequence = 0;
		FDelegateHandle DeathDelegateHandle;
	};

	UFUNCTION()
	void HandleCombatRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleCombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	void HandleCandidateDeath(const struct FSWDeathContext& DeathContext, TWeakObjectPtr<AActor> DeadActor);
	void AddCandidateAuthority(AActor& CandidateActor);
	void RemoveCandidateAuthority(const AActor& CandidateActor, ETargetInvalidReason InvalidReason);
	void RefreshTargetAuthority(ETargetInvalidReason InvalidReason);
	bool IsCandidateLegalAuthority(const FCandidateRecord& Candidate, ETargetInvalidReason& OutInvalidReason) const;
	void ClearCandidatesAuthority();
	void BroadcastTargetIfChanged(TWeakObjectPtr<AActor> NewTarget);
	void AdvanceEntrySequence();

	TWeakObjectPtr<ASWDefenseStructure> StructureOwner;
	TMap<TObjectKey<const AActor>, FCandidateRecord> Candidates;
	TWeakObjectPtr<AActor> CurrentTarget;
	uint64 NextEntrySequence = 1;
	bool bInitializedAuthority = false;
	FSWOnStructureTargetChanged OnTargetChanged;
};
