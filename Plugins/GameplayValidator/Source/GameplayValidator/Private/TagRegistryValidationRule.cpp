// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/TagRegistryValidationRule.h"
#include "GASValidator.h"
#include "GameplayTagsManager.h"

TagRegistryValidationRule::TagRegistryValidationRule() :
	IGASValidationRule("TagRegistration")
{
}

void TagRegistryValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	auto& Manager = UGameplayTagsManager::Get();

	for (auto Container : Objects.TagContainers)
	{
		for (auto Tag : Container.Container)
		{
			if (!Manager.IsValidGameplayTagString(Tag.ToString()))
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::ERROR;
				Result.Message = FString::Printf(
					TEXT("'%s':Tag '%s' isn't found in '%s'. Please register it in the 'Config.ini' or 'Edit->Project Settings->Project->Gameplay Tags' or use the 'UE_DEFINE_GAMEPLAY_TAG' macro in C++"), *Container.Class.ToString(), *Tag.ToString(), *Container.PropertyName.ToString());
				Results.Add(Result);
			}
		}
	}
}
