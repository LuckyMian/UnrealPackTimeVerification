// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimeVerificationConfig.generated.h"

/**
 * 
 */
UCLASS(config=Game, defaultconfig)
class TIMEVERIFICATION_API UTimeVerificationConfig : public UObject
{
	GENERATED_BODY()
public:
	UTimeVerificationConfig();
	
	

	
	UPROPERTY(EditAnywhere, config, Category="General")
	bool bEnableTimeVerification = false;

	UPROPERTY(EditAnywhere, config, Category="General")
	int32 Year;
	UPROPERTY(EditAnywhere, config, Category="General")
	int32 Month;
	UPROPERTY(EditAnywhere, config, Category="General")
	int32 Day;
};
