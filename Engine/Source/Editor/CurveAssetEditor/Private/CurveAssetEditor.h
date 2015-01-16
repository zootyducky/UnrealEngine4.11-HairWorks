// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorManager.h"

class FCurveAssetEditor :  public ICurveAssetEditor
{
public:

	FCurveAssetEditor()
		: TrackWidget( NULL )
		, ViewMinInput( 0.f )
		, ViewMaxInput( 0.f )
	{}

	virtual ~FCurveAssetEditor() {}

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/**
	 * Edits the specified table
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	CurveToEdit				The curve to edit
	 */
	void InitCurveAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveBase* CurveToEdit );

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	TSharedPtr<FExtender> GetToolbarExtender();

private:

	float GetViewMinInput() const { return ViewMinInput; }
	float GetViewMaxInput() const { return ViewMaxInput; }
	/** Sets InViewMinInput and InViewMaxInput */
	void SetInputViewRange(float InViewMinInput, float InViewMaxInput);
	/** Return length of timeline */
	float GetTimelineLength() const;
	/**	Spawns the tab with the curve asset inside */
	TSharedRef<SDockTab> SpawnTab_CurveAsset( const FSpawnTabArgs& Args );

	/** Get the snapping value for the input domain. */
	float GetInputSnap() const;
	/** Gets the snapping value for the input domain as text. */
	FText GetInputSnapText() const;
	/** Get set the snapping value for the input domain. */
	void SetInputSnap(float value);
	/** Callback for setting the snapping value for the input domain from a text value. */
	void InputSnapTextComitted(const FText& InNewText, ETextCommit::Type InTextCommit);
	/** Builds a menu widget with options for snapping in the input domain. */
	TSharedRef<SWidget> BuildInputSnapMenu();

	/** Get the snapping value for the output domain. */
	float GetOutputSnap() const;
	/** Gets the snapping value for the output domain as text. */
	FText GetOutputSnapText() const;
	/** Get set the snapping value for the output domain. */
	void SetOutputSnap(float value);
	/** Callback for setting the snapping value for the output domain from a text value. */
	void OutputSnapTextComitted(const FText& InNewText, ETextCommit::Type InTextCommit);
	/** Builds a menu widget with options for snapping in the output domain. */
	TSharedRef<SWidget> BuildOutputSnapMenu();

	/** Get the current snap label visibility for a given orientation. */
	EVisibility GetSnapLabelVisibilty(EOrientation LabelOrientation) const;

	TSharedPtr<class SCurveEditor> TrackWidget;

	/**	The tab id for the curve asset tab */
	static const FName CurveTabId;

	float ViewMinInput;
	float ViewMaxInput;

	/** The snapping value for the input domain. */
	float InputSnap;
	/** The snapping value for the output domain. */
	float OutputSnap;
};
