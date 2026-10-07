enum class EGASValidationSeverity
{
	INFO,
	WARNING,
	ERROR,
	UNRESOLVED
};

struct GASValidationResult
{
	FString RuleName;
	EGASValidationSeverity Severity;
	FString Message; // The reason of a warning or error
};