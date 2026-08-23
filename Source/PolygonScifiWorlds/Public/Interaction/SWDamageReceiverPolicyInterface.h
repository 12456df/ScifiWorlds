// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "SWDamageReceiverPolicyInterface.generated.h"

class AActor;

/** 特殊伤害接收者拒绝伤害时的服务器诊断原因；不复制，也不作为 UI 文案。 */
enum class ESWDamageReceptionRejectionReason : uint8
{
	None,
	TargetUnavailable,
	InvalidSource,
	InvalidTeam,
	UnsupportedDamageType,
	SourceOutOfRange,
};

/** ExecCalc 交给特殊接收者的只读服务器伤害上下文。 */
struct FSWDamageReceptionQuery
{
	const AActor* SourceAvatar = nullptr;
	const AActor* TargetActor = nullptr;
	FGameplayTag DamageType;
	FVector ServerSourceLocation = FVector::ZeroVector;
};

/** 特殊接收者返回的纯决策；ExecCalc 保持唯一的 IncomingDamage 写入者。 */
struct FSWDamageReceptionResult
{
	bool bAccepted = true;
	float PostMitigationMultiplier = 1.f;
	ESWDamageReceptionRejectionReason RejectionReason = ESWDamageReceptionRejectionReason::None;

	static FSWDamageReceptionResult Accept(const float Multiplier = 1.f)
	{
		FSWDamageReceptionResult Result;
		Result.bAccepted = true;
		Result.PostMitigationMultiplier = Multiplier;
		return Result;
	}

	static FSWDamageReceptionResult Reject(const ESWDamageReceptionRejectionReason Reason)
	{
		FSWDamageReceptionResult Result;
		Result.bAccepted = false;
		Result.PostMitigationMultiplier = 0.f;
		Result.RejectionReason = Reason;
		return Result;
	}
};

/**
 * 可选的服务器伤害接收策略。
 * 普通角色与小兵不实现该接口，继续沿用现有 ExecCalc；结构将在后续 M12 步骤实现该接口以附加专属受击门槛。
 */
UINTERFACE(NotBlueprintable)
class POLYGONSCIFIWORLDS_API USWDamageReceiverPolicyInterface : public UInterface
{
	GENERATED_BODY()
};

class POLYGONSCIFIWORLDS_API ISWDamageReceiverPolicyInterface
{
	GENERATED_BODY()

public:
	/** 仅由服务器 ExecCalc 调用；只返回决策，严禁直接写 Attribute、死亡或表现。 */
	virtual FSWDamageReceptionResult EvaluateDamageReceptionAuthority(const FSWDamageReceptionQuery& Query) const = 0;
};
