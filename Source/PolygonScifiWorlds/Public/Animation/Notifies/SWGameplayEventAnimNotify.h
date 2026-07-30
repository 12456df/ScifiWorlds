// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "SWGameplayEventAnimNotify.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * 在动画的指定时刻向拥有该 Skeletal Mesh 的 Actor 分发 Gameplay Event。
 * 默认仅服务器分发，确保弹丸、伤害和法术等权威副作用不会由客户端 Notify 决定。
 */
UCLASS(meta = (DisplayName = "SW 发送 Gameplay Event"))
class POLYGONSCIFIWORLDS_API USWGameplayEventAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	/** 要分发的 Gameplay Event Tag；应使用项目已声明的 Event.* Native Tag。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event", meta = (Categories = "Event"))
	FGameplayTag EventTag;

	/** 写入 Payload 的通用数值；当前武器开火默认使用 1。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	float EventMagnitude = 1.f;

	/** 为 false 时可在客户端也分发事件，且调用方仍必须自行保证无权威写入。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	bool bOnlySendOnAuthority = true;
};
