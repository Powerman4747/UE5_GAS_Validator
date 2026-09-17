// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayValidator.h"

#include "GASValidator.h"
#include "GASValidatorLog.h"

#define LOCTEXT_NAMESPACE "FGameplayValidatorModule"

void FGameplayValidatorModule::StartupModule()
{
	UE_LOG(LogGASValidator, Log, TEXT("GameplayValidator module loaded"));

	// Register a function to be called when menu system is initialized
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
		this, &FGameplayValidatorModule::AddValidationButton));}

void FGameplayValidatorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
 	UToolMenus::UnregisterOwner(this);
}

/*
 * minifloppy.it/posts/2024/adding-custom-buttons-unreal-editor-toolbars-menus/
 */
void FGameplayValidatorModule::AddValidationButton()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(
		"LevelEditor.MainMenu.Tools");
	FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("DataValidation");
 
	ToolbarSection.AddMenuEntry(
	"GameplayValidatorGAS",
	INVTEXT("Validate GAS"),
	INVTEXT("Validate the Gameplay Ability System (GAS)"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
			FUIAction(FExecuteAction::CreateStatic(&UGASValidator::RunValidator
	)));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameplayValidatorModule, GameplayValidator)