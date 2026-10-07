#pragma once
#include "CoreMinimal.h"

enum class EGASValidationSeverity
{
	INFO,
	WARNING,
	ERROR,
	UNRESOLVED
};

struct FGASValidationResult
{
	FString RuleName;
	EGASValidationSeverity Severity;
	FString Message; // The reason of a warning or error
};