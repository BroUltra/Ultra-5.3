// Copyright Epic Games, Inc. All Rights Reserved.

#include "UltraSettingKeyboardInput.h"

#include "../UltraSettingsLocal.h"
#include "Player/UltraLocalPlayer.h"
#include "PlayerMappableInputConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UltraSettingKeyboardInput)

class ULocalPlayer;

#define LOCTEXT_NAMESPACE "UltraSettings"

void FKeyboardOption::ResetToDefault()
{
	if (OwningConfig)
	{
		InputMapping = OwningConfig->GetMappingByName(InputMapping.PlayerMappableOptions.Name);	
	}
	// If we don't have an owning config, then there is no default binding for this and it can simply be removed
	else
	{
		InputMapping = FEnhancedActionKeyMapping();
	}
}

void FKeyboardOption::SetInitialValue(FKey InKey)
{
	InitialMapping = InKey;
}

UUltraSettingKeyboardInput::UUltraSettingKeyboardInput()
{
	bReportAnalytics = false;
}

void UUltraSettingKeyboardInput::OnInitialized()
{
	DynamicDetails = FGetGameSettingsDetails::CreateLambda([this](ULocalPlayer&) {
		return FText::Format(LOCTEXT("DynamicDetails_KeyboardInputAction", "Bindings for {0}"), FirstMappableOption.InputMapping.PlayerMappableOptions.DisplayName);
	});

	Super::OnInitialized();
}

void UUltraSettingKeyboardInput::SetInputData(FEnhancedActionKeyMapping& BaseMapping, const UPlayerMappableInputConfig* InOwningConfig, int32 InKeyBindSlot)
{
	if (InKeyBindSlot == 0)
	{
		FirstMappableOption.InputMapping = BaseMapping;
		FirstMappableOption.OwningConfig = InOwningConfig;
		FirstMappableOption.SetInitialValue(BaseMapping.Key);
	}
	else if (InKeyBindSlot == 1)
	{
		SecondaryMappableOption.InputMapping = BaseMapping;
		SecondaryMappableOption.OwningConfig = InOwningConfig;
		SecondaryMappableOption.SetInitialValue(BaseMapping.Key);
	}
	else
	{
		ensureMsgf(false, TEXT("Invalid key bind slot provided!"));
	}

	ensure(FirstMappableOption.InputMapping.PlayerMappableOptions.Name != NAME_None && !FirstMappableOption.InputMapping.PlayerMappableOptions.DisplayName.IsEmpty());

	const FString NameString = TEXT("KBM_Input_") + FirstMappableOption.InputMapping.PlayerMappableOptions.Name.ToString();
	SetDevName(*NameString);
	SetDisplayName(FirstMappableOption.InputMapping.PlayerMappableOptions.DisplayName);
}

FText UUltraSettingKeyboardInput::GetPrimaryKeyText() const
{
	return FirstMappableOption.InputMapping.Key.GetDisplayName();
}

FText UUltraSettingKeyboardInput::GetSecondaryKeyText() const
{
	return SecondaryMappableOption.InputMapping.Key.GetDisplayName();
}

void UUltraSettingKeyboardInput::ResetToDefault()
{
	// Find the UPlayerMappableInputConfig that this came from and reset it to the value in there
	FirstMappableOption.ResetToDefault();
	SecondaryMappableOption.ResetToDefault();
}

void UUltraSettingKeyboardInput::StoreInitial()
{
	FirstMappableOption.SetInitialValue(FirstMappableOption.InputMapping.Key);
	SecondaryMappableOption.SetInitialValue(SecondaryMappableOption.InputMapping.Key);
}

void UUltraSettingKeyboardInput::RestoreToInitial()
{	
	ChangeBinding(0, FirstMappableOption.GetInitialStoredValue());
	ChangeBinding(1, SecondaryMappableOption.GetInitialStoredValue());
}

bool UUltraSettingKeyboardInput::ChangeBinding(int32 InKeyBindSlot, FKey NewKey)
{
	// Early out if they hit the same button that is already bound. This allows for them to exit binding if they made a mistake.
	if ((InKeyBindSlot == 0 && FirstMappableOption.InputMapping.Key == NewKey) || (InKeyBindSlot == 1 && SecondaryMappableOption.InputMapping.Key == NewKey))
	{
		return false;
	}

	if (!NewKey.IsGamepadKey())
	{
		UUltraLocalPlayer* UltraLocalPlayer = CastChecked<UUltraLocalPlayer>(LocalPlayer);
		UUltraSettingsLocal* LocalSettings = UltraLocalPlayer->GetLocalSettings();
		if (InKeyBindSlot == 0)
		{
			LocalSettings->AddOrUpdateCustomKeyboardBindings(FirstMappableOption.InputMapping.PlayerMappableOptions.Name, NewKey, UltraLocalPlayer);
			FirstMappableOption.InputMapping.Key = NewKey;
		}
		else if (InKeyBindSlot == 1)
		{
			// If there is no default secondary binding then we can create one based off of data from the primary binding
			if (SecondaryMappableOption.InputMapping.PlayerMappableOptions.Name == NAME_None)
			{
				SecondaryMappableOption = FKeyboardOption(FirstMappableOption);
			}
			
			LocalSettings->AddOrUpdateCustomKeyboardBindings(SecondaryMappableOption.InputMapping.PlayerMappableOptions.Name, NewKey, UltraLocalPlayer);
			SecondaryMappableOption.InputMapping.Key = NewKey;
		}
		else
		{
			ensureMsgf(false, TEXT("Invalid key bind slot provided!"));
		}

		// keybindings are never reset to default or initial
		NotifySettingChanged(EGameSettingChangeReason::Change);

		return true;
	}

	return false;
}

void UUltraSettingKeyboardInput::GetAllMappedActionsFromKey(int32 InKeyBindSlot, FKey Key, TArray<FName>& OutActionNames) const
{
	if (InKeyBindSlot == 1)
	{
		if (SecondaryMappableOption.InputMapping.Key == Key)
		{
			return;
		}
	}
	else
	{
		if (FirstMappableOption.InputMapping.Key == Key)
		{
			return;
		}
	}

	if (const UUltraLocalPlayer* UltraLocalPlayer = CastChecked<UUltraLocalPlayer>(LocalPlayer))
	{
		UUltraSettingsLocal* LocalSettings = UltraLocalPlayer->GetLocalSettings();
		LocalSettings->GetAllMappingNamesFromKey(Key, OutActionNames);
	}
}

#undef LOCTEXT_NAMESPACE

