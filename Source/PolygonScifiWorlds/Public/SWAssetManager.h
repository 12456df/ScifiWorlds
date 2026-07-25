// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SWAssetManager.generated.h"

/**
 * ScifiWorlds 的 AssetManager。
 *
 * 作为引擎启动早期的统一初始化入口，负责初始化 GAS 全局数据。需在 DefaultEngine.ini 中通过
 * AssetManagerClassName 注册为项目的 AssetManager。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static USWAssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};
