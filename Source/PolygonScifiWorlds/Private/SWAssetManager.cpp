// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "SWAssetManager.h"

#include "AbilitySystemGlobals.h"

USWAssetManager& USWAssetManager::Get()
{
	check(GEngine);
	USWAssetManager* SWAssetManager = Cast<USWAssetManager>(GEngine->AssetManager);
	return *SWAssetManager;
}

void USWAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// 初始化 GAS 全局数据：注册 TargetData 等结构的网络序列化支持，并使自定义
	// AbilitySystemGlobals（含自定义 EffectContext 分配）真正生效。原生 Gameplay Tag
	// 通过 UE_DEFINE_GAMEPLAY_TAG_COMMENT 自动注册，此处无需额外处理。
	UAbilitySystemGlobals::Get().InitGlobalData();
}
