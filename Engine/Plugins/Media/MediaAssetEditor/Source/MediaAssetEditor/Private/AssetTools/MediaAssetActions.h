// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "AssetTypeActions_Base.h"


/**
 * Implements an action for media assets.
 */
class FMediaAssetActions
	: public FAssetTypeActions_Base
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InStyle The style set to use for asset editor toolkits.
	 */
	FMediaAssetActions( const TSharedRef<ISlateStyle>& InStyle );

public:

	// FAssetTypeActions_Base overrides

	virtual bool CanFilter( ) override;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual uint32 GetCategories( ) override;
	virtual FText GetName( ) const override;
	virtual UClass* GetSupportedClass( ) const override;
	virtual FColor GetTypeColor( ) const override;
	virtual bool HasActions( const TArray<UObject*>& InObjects ) const override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;

private:

	// Callback for selecting CreateMediaSoundWave.
	void ExecuteCreateMediaSoundWave( TArray<TWeakObjectPtr<UMediaAsset>> Objects );

	// Callback for selecting CreateMediaTexture.
	void ExecuteCreateMediaTexture( TArray<TWeakObjectPtr<UMediaAsset>> Objects );	

	// Callback for executing the 'Play Movie' action.
	void HandlePauseMovieActionExecute( TArray<TWeakObjectPtr<UMediaAsset>> Objects );

	// Callback for executing the 'Play Movie' action.
	void HandlePlayMovieActionExecute( TArray<TWeakObjectPtr<UMediaAsset>> Objects );

	// Callback for executing the 'Play Movie' action.
	void HandleStopMovieActionExecute( TArray<TWeakObjectPtr<UMediaAsset>> Objects );

private:

	// Pointer to the style set to use for toolkits.
	TSharedRef<ISlateStyle> Style;
};
