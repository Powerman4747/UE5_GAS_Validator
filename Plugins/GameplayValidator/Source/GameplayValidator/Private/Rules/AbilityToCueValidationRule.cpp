// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/AbilityToCueValidationRule.h"

#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "GameplayEffect.h"
#include "DataStructures/DiscoveryData.h"
#include "Engine/Engine.h"

AbilityToCueValidationRule::AbilityToCueValidationRule() :
	IGASValidationRule("AbilityToCuePipeline")
{
}

void AbilityToCueValidationRule::Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results)
{
	UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();	
	UGameplayCueSet* CueSet = CueManager->GetEditorCueSet();	
	
	for (const auto& Pair : Objects.Abilities)
	{
		const auto& Ability = Pair.Value;

		for (const auto& Effect : Ability.Effects)
		{
			if (!Objects.Effects.Contains(Effect.EffectClass))
			{
				// Ability doesn't do anyhting. no Effect is triggerred
				continue;
			}

			if (const TArray<FDiscoveredTagContainer>* TagContainers = Objects.TagContainers.Find(Effect.EffectClass))
			{
				for (const FDiscoveredTagContainer& TagContainer : *TagContainers)
				{
					if (TagContainer.PropertyName != "GameplayCues")
					{
						continue;
					}
					
					if (!CueSet)
					{
						// Cue set unavailable, can't check
						continue;
					}
					
					for (const auto& Tag : TagContainer.Container)
					{
						int32* idx = CueSet->GameplayCueDataMap.Find(Tag);
						if (idx == nullptr)
						{
							// Tag is do not reference to a Cue
						}
					}
					
					break;
				}
			}
		}
	}
}