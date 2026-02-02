// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeVerification.h"

#include "TimeVerificationConfig.h"

//only editor include
#if WITH_EDITOR
#include "ISettingsModule.h"
#endif

#define LOCTEXT_NAMESPACE "FTimeVerificationModule"

void FTimeVerificationModule::StartupModule()
{
#if WITH_EDITOR
	// Get Settings Module
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		// Register to Project Settings
		SettingsModule->RegisterSettings(
			"Project",         // ContainerName
			"Plugins",         // CategoryName
			"Time Verification", // DisplayName
			LOCTEXT("TimeVerificationSettingsName", "Time Verification"),
			LOCTEXT("TimeVerificationSettingsDesc", "Configure time verification plugin settings."),
			GetMutableDefault<UTimeVerificationConfig>() // 配置对象
		);
	}
#endif
}

void FTimeVerificationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTimeVerificationModule, TimeVerification)