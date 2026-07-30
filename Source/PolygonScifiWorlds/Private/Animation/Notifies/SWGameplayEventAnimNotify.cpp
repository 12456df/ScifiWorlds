// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Animation/Notifies/SWGameplayEventAnimNotify.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void USWGameplayEventAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* AvatarActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!AvatarActor || !EventTag.IsValid() || (bOnlySendOnAuthority && !AvatarActor->HasAuthority()))
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.EventMagnitude = EventMagnitude;
	EventData.Instigator = AvatarActor;
	EventData.Target = AvatarActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, EventTag, EventData);
}
