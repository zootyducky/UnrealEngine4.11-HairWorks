// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MediaAssetFactory.generated.h"


/**
 * Implements a factory for UMediaAsset objects.
 */
UCLASS(hidecategories=Object)
class UMediaAssetFactory
	: public UFactory
{
	GENERATED_UCLASS_BODY()


public:

	// UFactory Interface

	virtual UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn ) override;

protected:

	/** Reloads the collection of supported media formats. */
	void ReloadMediaFormats( );

private:

	// Callback for handling the adding of media player factories.
	void HandleMediaPlayerFactoryAdded( );

	// Callback for handling the adding of media player factories.
	void HandleMediaPlayerFactoryRemoved( );
};
