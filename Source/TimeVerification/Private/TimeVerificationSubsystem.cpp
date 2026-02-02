// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeVerificationSubsystem.h"

#include "TimeVerificationConfig.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

void UTimeVerificationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UTimeVerificationConfig* Config = GetDefault<UTimeVerificationConfig>();
	if (!Config || !Config->bEnableTimeVerification)
	{
		return; // Time verification disabled; skip checks.
	}

	// Step 1: Verify no files have modification time in the future (returns true if integrity OK).
	if (!CheckSystemTimeIntegrity())
	{
		ExitGame();
		return;
	}
	// Step 2: Verify current time is before the configured verification date (returns true if allowed to run).
	if (!IsBeforeVerificationDate())
	{
		ExitGame();
		return;
	}
	// All checks passed; no action required.
}

void UTimeVerificationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTimeVerificationSubsystem::ExitGame()
{
	
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this,&UTimeVerificationSubsystem::OnWorldReady);
}

void UTimeVerificationSubsystem::OnWorldReady(UWorld* World,const UWorld::InitializationValues InitValues) const
{
	// Prevent multiple triggers
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	if (!World || World->WorldType != EWorldType::Game)
	{
		return;
	}
	if ( World != nullptr)
	{
		APlayerController* PC = World->GetFirstPlayerController();

		FMessageDialog::Open(EAppMsgType::Ok,FText::FromString(TEXT("TimeVerification is Error, Game will quit."))
);
		// Defer quit to next frame so controller is valid.
		World->GetTimerManager().SetTimerForNextTick([World, PC]()
		{
			UKismetSystemLibrary::QuitGame(
				World,
				PC,
				EQuitPreference::Quit,
				false
			);
		});
	}
}

bool UTimeVerificationSubsystem::IsBeforeVerificationDate()
{
	const UTimeVerificationConfig* Config = GetDefault<UTimeVerificationConfig>();
	if (!Config)
	{
		UE_LOG(LogTemp, Error, TEXT("TimeVerificationConfig not found."));
		return false;
	}
	int32 Year = Config->Year;
	int32 Month = Config->Month;
	int32 Day = Config->Day;

	FDateTime Now = FDateTime::Now();
	bool bValidDate = FDateTime::Validate(Year, Month, Day, 0, 0, 0, 0);
	if (!bValidDate)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid date in TimeVerificationConfig: %d-%d-%d"), Year, Month, Day);
		return false;
	}

	FDateTime VerificationDate = FDateTime(Year, Month, Day);
	// Compare date only so that the verification date itself (same day) is allowed.
	FDateTime NowDateOnly(Now.GetYear(), Now.GetMonth(), Now.GetDay(), 0, 0, 0, 0);
	bool bBeforeOrSameDay = NowDateOnly <= VerificationDate;

	UE_LOG(LogTemp, Error, TEXT("NowTime: %s, VerificationTime: %s, Result: %s"),
		*Now.ToString(), *VerificationDate.ToString(), bBeforeOrSameDay ? TEXT("Verification is OK") : TEXT("Verification is Error"));
	return bBeforeOrSameDay;
}

bool UTimeVerificationSubsystem::CheckSystemTimeIntegrity()
{
	// Resolve scan directory in a cross-platform way:
	// Windows: TEMP/TMP; Linux/macOS: TMPDIR (TEMP often unset); Android/iOS: use ProjectSavedDir when env vars are unavailable.
	FString TempPath = FPlatformMisc::GetEnvironmentVariable(TEXT("TEMP"));
	if (TempPath.IsEmpty())
	{
		TempPath = FPlatformMisc::GetEnvironmentVariable(TEXT("TMPDIR"));
	}
	if (TempPath.IsEmpty())
	{
		TempPath = FPaths::ProjectSavedDir();
	}
	if (TempPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Temp/Scan path is empty, skipping time integrity check."));
		return true; // Pass when check cannot be performed.
	}

	const int32 MaxCheckCount = 300;
	int32 CheckedCount = 0;
	int32 FutureFileCount = 0;
	bool bHasFutureFile = false;

	FDateTime NowDateTime = FDateTime::UtcNow();
	IFileManager& FileManager = IFileManager::Get();

	// Iterate directory contents (non-recursive).
	FileManager.IterateDirectory(*TempPath, [&CheckedCount, MaxCheckCount, &NowDateTime, &bHasFutureFile, &FutureFileCount](const TCHAR* FilenameOrDir, bool bIsDirectory) -> bool
	{
		if (bIsDirectory)
		{
			return true; // Skip directories.
		}

		CheckedCount++;
		if (CheckedCount > MaxCheckCount)
		{
			return false; // Stop iteration.
		}

		FString FilePath(FilenameOrDir);
		FDateTime FileDate = IFileManager::Get().GetTimeStamp(*FilePath);

		if (FileDate > NowDateTime)
		{
			bHasFutureFile = true;
			FutureFileCount++;
		}

		return true; // Continue iteration.
	});

	// Integrity check fails if any file has a modification time in the future (possible time tampering).
	return !bHasFutureFile;
}
