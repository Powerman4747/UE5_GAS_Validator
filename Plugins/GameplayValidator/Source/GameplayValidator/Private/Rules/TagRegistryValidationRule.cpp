// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/TagRegistryValidationRule.h"
#include "GameplayTagsManager.h"
#include "DataStructures/DiscoveryData.h"

TagRegistryValidationRule::TagRegistryValidationRule() :
	IGASValidationRule("TagRegistration")
{
}

void TagRegistryValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	auto& Manager = UGameplayTagsManager::Get();

	for (auto Pair : Objects.TagContainers)
	{
		const auto& Containers = Pair.Value;
		for (const auto& Container : Containers)
		{
			for (const auto& Tag : Container.Container)
			{
				if (!Manager.IsValidGameplayTagString(Tag.ToString()))
				{
					GASValidationResult Result;
					Result.RuleName = GetRuleName();
					Result.Severity = EGASValidationSeverity::ERROR;
					Result.Message = FString::Printf(
						TEXT("'%s':Tag '%s' in container '%s' isn't in correct format. Please use the correct format 'Parent.Tree.TagName' (no spaces or dots at the end)"), *Pair.Key->GetFName().ToString(), *Tag.ToString(), *Container.PropertyName.ToString());
					Results.Add(Result);
				}
				else if (Manager.RequestGameplayTag(Tag.GetTagName()) == FGameplayTag() && Tag.IsValid())
				{
					GASValidationResult Result;
					Result.RuleName = GetRuleName();
					Result.Severity = EGASValidationSeverity::ERROR;
					Result.Message = FString::Printf(
						TEXT("'%s':Tag '%s' isn't found in '%s'. Please register it in the 'Config.ini' or 'Edit->Project Settings->Project->Gameplay Tags' or use the 'UE_DEFINE_GAMEPLAY_TAG' macro in C++"), *Pair.Key->GetFName().ToString(), *Tag.ToString(), *Container.PropertyName.ToString());
					Results.Add(Result);
				}
				
				if (!Tag.IsValid())
				{
					GASValidationResult Result;
					Result.RuleName = GetRuleName();
					Result.Severity = EGASValidationSeverity::ERROR;
					Result.Message = FString::Printf(
						TEXT("'%s': unregistered tag found in '%s'"), *Pair.Key->GetFName().ToString(), *Container.PropertyName.ToString());
					Results.Add(Result);
				}
			}
		}
	}
}
