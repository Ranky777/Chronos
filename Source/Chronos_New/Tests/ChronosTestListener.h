#pragma once

#include "CoreMinimal.h"
#include "ChronosTestListener.generated.h"

/**
 * 自动化测试专用委托监听器。
 * dynamic 委托（AddDynamic）要求绑定目标是 UObject + UFUNCTION 成员，
 * 而 UCLASS 必须位于头文件中由 UHT 处理（GENERATED_BODY 依赖 .generated.h），
 * 故从 Tests/ChronosGameFlowTests.cpp 中抽出（简报原文将其写在 .cpp 内，无法编译）。
 */
UCLASS()
class CHRONOS_NEW_API UChronosTestListener : public UObject
{
	GENERATED_BODY()
public:
	int32 ClearedCount = 0;
	bool bFinalFlag = false;
	int32 CountCalls = 0;

	UFUNCTION()
	void HandleCleared(bool bFinal) { ++ClearedCount; bFinalFlag = bFinal; }
	UFUNCTION()
	void HandleCount(int32 RemainingEnemies) { ++CountCalls; }
};
