#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class AChronosCharacter;

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可交互对象统一接口。
 *
 * 旧项目的教训：武器拾取物同时实现 IInteractable 与 IThrowable 两套接口，
 * 交互判定需要按顺序探测导致过 Bug。这里合并为单一接口，
 * 拾取物的"可投掷"属性由其自身状态决定而非接口类型决定。
 */
class CHRONOS_NEW_API IInteractable
{
	GENERATED_BODY()

public:
	/** 执行交互（拾取等） */
	virtual void OnInteract(AChronosCharacter* Interactor) = 0;

	/** 当前是否可被交互（例如刚扔出的武器在落地前不可拾取） */
	virtual bool CanInteract(const AChronosCharacter* Interactor) const = 0;

	/** 准星提示文本 */
	virtual FText GetInteractionText() const = 0;
};
