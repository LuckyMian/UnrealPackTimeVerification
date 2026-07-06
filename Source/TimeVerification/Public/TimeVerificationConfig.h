// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimeVerificationConfig.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FEncryptedUsers
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Verification")
	FString UsersID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Verification")
	FString CPU;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Verification")
	FString DiskSN;
	
};


UCLASS(config=Game, defaultconfig)
class TIMEVERIFICATION_API UTimeVerificationConfig : public UObject
{
	GENERATED_BODY()
public:
	UTimeVerificationConfig();
	
	

	
	UPROPERTY(EditAnywhere, config, Category="General")
	bool bEnableTimeVerification = false;

	UPROPERTY(EditAnywhere, config, Category="General",meta = (EditCondition = "bEnableTimeVerification"))
	int32 Year;
	UPROPERTY(EditAnywhere, config, Category="General",meta = (EditCondition = "bEnableTimeVerification"))
	int32 Month;
	UPROPERTY(EditAnywhere, config, Category="General",meta = (EditCondition = "bEnableTimeVerification"))
	int32 Day;
	
	UPROPERTY(EditAnywhere, config, Category="General",meta = (EditCondition = "bEnableTimeVerification"))
	bool bIsEnableEncryptedUsers = false;
	
	UPROPERTY(EditAnywhere, config, Category="General",meta = (EditCondition = "bIsEnableEncryptedUsers"))
	TArray<FEncryptedUsers> EncryptedUsers;
};
