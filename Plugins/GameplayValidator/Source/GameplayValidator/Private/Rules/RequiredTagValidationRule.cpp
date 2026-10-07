// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/RequiredTagValidationRule.h"
#include "DataStructures/DiscoveryData.h"

FRequiredTagValidationRule::FRequiredTagValidationRule(FName InClassName, FName InTagContainerName, TArray<FName> InTags) :
	IGASValidationRule("RequiredTag")
{
	ClassName = InClassName;
	TagContainerName = InTagContainerName;
	for (const auto& InTag : InTags)
	{
		Tags.Add(FGameplayTag::RequestGameplayTag(InTag));
	}
}

void FRequiredTagValidationRule::Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results)
{
	for ( const auto& Pair : Objects.TagContainers)
	{
		const auto& TagContainers = Pair.Value;
		
		for ( const auto& TagContainer : TagContainers)
		{
			if(Pair.Key->GetName() != ClassName || TagContainer.PropertyName != TagContainerName)
			{
				continue;
			}

			for (const auto& Tag : Tags)
			{
				if (!TagContainer.Container.HasTag(FGameplayTag(Tag)))
				{
					FGASValidationResult Result;
					Result.RuleName = GetRuleName();
					Result.Severity = EGASValidationSeverity::ERROR;
					Result.Message = FString::Printf(
						TEXT("Required tag '%s' does not exist in the required tag container '%s'"), *Tag.ToString(), *TagContainerName.ToString());
					Results.Add(Result);
				}
			}
		}
	}
}
