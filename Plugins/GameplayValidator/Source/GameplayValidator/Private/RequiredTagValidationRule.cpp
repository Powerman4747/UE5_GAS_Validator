// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/RequiredTagValidationRule.h"
#include "GasValidator.h"

RequiredTagValidationRule::RequiredTagValidationRule(FName InClassName, FName InTagContainerName, TArray<FName> InTags) :
	IGASValidationRule("RequiredTag")
{
	ClassName = InClassName;
	TagContainerName = InTagContainerName;
	for (const auto& InTag : InTags)
	{
		Tags.Add(FGameplayTag::RequestGameplayTag(InTag));
	}
}

void RequiredTagValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	for ( const auto& TagContainer : Objects.TagContainers)
	{
		if(TagContainer.Class != ClassName || TagContainer.PropertyName != TagContainerName)
		{
			continue;
		}

		for (const auto& Tag : Tags)
		{
			if (!TagContainer.Container.HasTag(FGameplayTag(Tag)))
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::ERROR;
				Result.Message = FString::Printf(
					TEXT("Required tag '%s' does not exist in the required tag container '%s'"), *Tag.ToString(), *TagContainerName.ToString());
				Results.Add(Result);
			}
		}		
	}
}
