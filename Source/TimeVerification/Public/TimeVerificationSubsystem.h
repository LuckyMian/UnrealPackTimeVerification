// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimeVerificationSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TIMEVERIFICATION_API UTimeVerificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	

	void ExitGame();
	
	void OnWorldReady(UWorld* World,const UWorld::InitializationValues InitValues) const;
	
	bool IsBeforeVerificationDate();
	
	bool CheckSystemTimeIntegrity();
};
