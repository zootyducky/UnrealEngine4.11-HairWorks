// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class FMediaAssetEditorTexture
	: public FTickableObjectRenderThread
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InSlateTexture The Slate texture to render to.
	 * @param InVideoTrack The video track to fetch.
	 */
	FMediaAssetEditorTexture( FSlateTexture2DRHIRef* InSlateTexture, const IMediaTrackRef& InVideoTrack )
		: FTickableObjectRenderThread(false)
		, LastFrameTime(FTimespan::MinValue())
		, SlateTexture(InSlateTexture)
		, VideoBuffer(MakeShareable(new FMediaSampleBuffer))
		, VideoTrack(InVideoTrack)
	{
		VideoTrack->AddSink(VideoBuffer);
	}

	/** Destructor. */
	~FMediaAssetEditorTexture( )
	{
		VideoTrack->RemoveSink(VideoBuffer);
	}

public:

	// FTickableObjectRenderThread interface

	virtual void Tick( float DeltaTime ) override
	{
		FTimespan CurrentFrameTime = VideoBuffer->GetCurrentSampleTime();

		if (CurrentFrameTime == LastFrameTime)
		{
			return;
		}

		TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> CurrentFrame = VideoBuffer->GetCurrentSample();

		if (!CurrentFrame.IsValid())
		{
			return;
		}

		uint32 Stride = 0;
		uint8* TextureBuffer = (uint8*)RHILockTexture2D(SlateTexture->GetTypedResource(), 0, RLM_WriteOnly, Stride, false);

		FMemory::Memcpy(TextureBuffer, CurrentFrame->GetData(), CurrentFrame->Num());
		RHIUnlockTexture2D(SlateTexture->GetTypedResource(), 0, false);

		LastFrameTime = CurrentFrameTime;
	}
	
	virtual TStatId GetStatId( ) const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMediaAssetEditorTexture, STATGROUP_Tickables);
	}

	virtual bool IsTickable( ) const override
	{
		if (!SlateTexture->IsInitialized())
		{
			SlateTexture->InitResource();

			FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();

 			SetRenderTarget(CommandList, SlateTexture->GetRHIRef(), FTextureRHIRef());
 			CommandList.SetViewport(0, 0, 0.0f, SlateTexture->GetWidth(), SlateTexture->GetHeight(), 1.0f);
 			CommandList.Clear(true, FLinearColor::Black, false, 0.f, false, 0, FIntRect());
		}

		return VideoBuffer->GetCurrentSample().IsValid();
	}

private:

	/** The playback time of the last drawn video frame. */
	FTimespan LastFrameTime;

	/** Pointer to the Slate texture being rendered. */
	FSlateTexture2DRHIRef* SlateTexture;

	/** The video sample buffer. */
	FMediaSampleBufferRef VideoBuffer;

	/** Holds the selected video track. */
	IMediaTrackRef VideoTrack;
};
