// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"

UAbilityTask_PlayMontageAndWait::UAbilityTask_PlayMontageAndWait(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Rate = 1.f;
}

void UAbilityTask_PlayMontageAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Ability.IsValid() && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			AbilitySystemComponent->ClearAnimatingAbility(Ability.Get());
		}
	}

	if (bInterrupted)
	{
		OnInterrupted.Broadcast();
	}
	else
	{
		OnComplete.Broadcast();
	}

	EndTask();
}

UAbilityTask_PlayMontageAndWait* UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(UObject* WorldContextObject,
	FName TaskInstanceName, UAnimMontage *MontageToPlay, float Rate, FName StartSection)
{
	UAbilityTask_PlayMontageAndWait* MyObj = NewTask<UAbilityTask_PlayMontageAndWait>(WorldContextObject, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;

	return MyObj;
}

void UAbilityTask_PlayMontageAndWait::Activate()
{
	if (AbilitySystemComponent.IsValid() && Ability.IsValid())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			if (AbilitySystemComponent->PlayMontage(Ability.Get(), Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection) > 0.f)
			{
				FOnMontageBlendingOutStarted BlendingOutDelegate;
				BlendingOutDelegate.BindUObject(this, &UAbilityTask_PlayMontageAndWait::OnMontageEnded);
				ActorInfo->AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate);
			}
			else
			{
				ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageAndWait called in Ability %s with null montage; Task Instance Name %s."), *Ability->GetName(), *InstanceName.ToString());
			}
		}
	}
}

void UAbilityTask_PlayMontageAndWait::ExternalCancel()
{
	check(AbilitySystemComponent.IsValid());
	OnCancelled.Broadcast();
	Super::ExternalCancel();
}

void UAbilityTask_PlayMontageAndWait::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	Super::OnDestroy(AbilityEnded);
}

FString UAbilityTask_PlayMontageAndWait::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability.IsValid())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			PlayingMontage = ActorInfo->AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}