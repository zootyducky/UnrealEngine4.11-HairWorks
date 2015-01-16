// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/NiagaraSimulation.h"
#include "Engine/NiagaraEffectRenderer.h"
#include "VectorVM.h"


FNiagaraSimulation::FNiagaraSimulation(FNiagaraEmitterProperties *InProps) 
: Age(0.0f)
, bIsEnabled(true)
, SpawnRemainder(0.0f)
, CachedBounds(ForceInit)
, EffectRenderer(nullptr)
{
	Props = InProps;

	Init();
}

FNiagaraSimulation::FNiagaraSimulation(FNiagaraEmitterProperties *InProps, ERHIFeatureLevel::Type InFeatureLevel)
	: Age(0.0f)
	, bIsEnabled(true)
	, SpawnRemainder(0.0f)
	, CachedBounds(ForceInit)
	, EffectRenderer(nullptr)
{
	Props = InProps;

	Init();
	SetRenderModuleType(Props->RenderModuleType, InFeatureLevel);
}

void FNiagaraSimulation::Init()
{
	if (Props && Props->UpdateScript && Props->SpawnScript)
	{
		Data.SetAttributes(Props->UpdateScript->Attributes);
		Constants.Empty();
		Constants.Merge(Props->ExternalConstants);

		bool bDisable = false;
		//Temporarily enforcing that spawn scripts have exactly the same attriubtes as update scripts.
		//To fix this we need to refactor the scripts into source and runtime objects.
		//The runtime will be a script autogenerated to be a safe combination of all scripts attributes. 
		//With an options somewhere for handling of attributes not set in a particular script. 
		//eg. Zeroing for spawn of attributes in update script that are not set explicitly in the spawn script. And pass-through copy for the reverse.
		if (Props->SpawnScript->Attributes.Num() == Props->UpdateScript->Attributes.Num())
		{
			for (int32 i = 0; i < Props->SpawnScript->Attributes.Num(); ++i)
			{
				if (Props->SpawnScript->Attributes[i] != Props->UpdateScript->Attributes[i])
				{
					bDisable = true;
					break;
				}
			}
		}
		else
		{
			bDisable = true;
		}

		if (bDisable)
		{
			Data.Reset();//Clear the attributes to mark the sim as disabled independatly of the user set Enabled flag.
			bIsEnabled = false;//Also force the user flag to give an indication to the user.
			UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's spawn script and update script don't have matching attributes."));
		}
	}
	else
	{
		Data.Reset();//Clear the attributes to mark the sim as disabled independatly of the user set Enabled flag.
		bIsEnabled = false;//Also force the user flag to give an indication to the user.
		//TODO - Arbitrary named scripts. Would need some base functionality for Spawn/Udpate to be called that can be overriden in BPs for emitters with custom scripts.
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's doesn't have both an update and spawn script."));
	}
}

void FNiagaraSimulation::SetProperties(FNiagaraEmitterProperties *InProps)	
{ 
	Props = InProps; 
	Init();
}

void FNiagaraSimulation::SetSpawnScript(UNiagaraScript* Script)
{
	Props->SpawnScript = Script;
	Init();
}

void FNiagaraSimulation::SetUpdateScript(UNiagaraScript* Script)
{
	Props->UpdateScript = Script;
	Init();
}

void FNiagaraSimulation::SetEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;
	Init();
}


float FNiagaraSimulation::GetTotalCPUTime()
{
	float Total = CPUTimeMS;
	if (EffectRenderer)
	{
		Total += EffectRenderer->GetCPUTimeMS();
	}

	return Total;
}

int FNiagaraSimulation::GetTotalBytesUsed()
{
	return Data.GetBytesUsed();
}


void FNiagaraSimulation::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);

	SimpleTimer TickTime;

	if (Data.GetNumAttributes() == 0)
		return;

	// Cache the ComponentToWorld transform.
//	CachedComponentToWorld = Component.GetComponentToWorld();

	Data.SwapBuffers();
	// Remember the stride of the original data.
	int32 PrevNumVectorsPerAttribute = Data.GetParticleAllocation();
	int32 MaxNewParticles = Data.GetNumParticles();
	int32 NumToSpawn = 0;

	// Figure out how many we will spawn.
	if (Props->SpawnScript)
	{
		NumToSpawn = CalcNumToSpawn(DeltaSeconds);
		MaxNewParticles = Data.GetNumParticles() + NumToSpawn;
		Data.Allocate(MaxNewParticles);
	}

	if (Props->UpdateScript)
	{
		// Simulate particles forward by DeltaSeconds.
		UpdateParticles(
			DeltaSeconds,
			Data.GetPreviousBuffer(),
			PrevNumVectorsPerAttribute,
			Data.GetCurrentBuffer(),
			MaxNewParticles,
			Data.GetNumParticles()
			);
	}

	if (Props->SpawnScript)
	{
		SpawnAndKillParticles(NumToSpawn);
	}

	CPUTimeMS = TickTime.GetElapsedMilliseconds();

	DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"), STAT_NiagaraNumParticles, STATGROUP_Niagara);
	INC_DWORD_STAT_BY(STAT_NiagaraNumParticles, Data.GetNumParticles());

}


void FNiagaraSimulation::UpdateParticles(
	float DeltaSeconds,
	FVector4* PrevParticles,
	int32 PrevNumVectorsPerAttribute,
	FVector4* Particles,
	int32 NumVectorsPerAttribute,
	int32 NumParticles
	)
{
	if (Data.GetNumAttributes() == 0)
		return;

	SCOPE_CYCLE_COUNTER(STAT_NiagaraSimulate);
	Age += DeltaSeconds;
	Constants.SetOrAdd(TEXT("Emitter Age"), FVector4(Age, Age, Age, Age));

	VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
	VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
	const int32 NumAttr = Props->UpdateScript->Attributes.Num();

	check(NumAttr < VectorVM::MaxInputRegisters);
	check(NumAttr < VectorVM::MaxOutputRegisters);

	// Setup input and output registers.
	for (int32 AttrIndex = 0; AttrIndex < NumAttr; ++AttrIndex)
	{
		InputRegisters[AttrIndex] = (VectorRegister*)(PrevParticles + AttrIndex * PrevNumVectorsPerAttribute);
		OutputRegisters[AttrIndex] = (VectorRegister*)(Particles + AttrIndex * NumVectorsPerAttribute);
	}

	//Fill constant table with required emitter constants and internal script constants.
	TArray<FVector4> ConstantTable;
	TArray<FNiagaraDataObject *>DataObjTable;
	Props->UpdateScript->ConstantData.FillConstantTable(Constants, ConstantTable, DataObjTable);

	VectorVM::Exec(
		Props->UpdateScript->ByteCode.GetData(),
		InputRegisters,
		NumAttr,
		OutputRegisters,
		NumAttr,
		ConstantTable.GetData(),
		DataObjTable.GetData(),
		NumParticles
		);
}





int32 FNiagaraSimulation::SpawnAndKillParticles(int32 NumToSpawn)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawnAndKill);
	int32 OrigNumParticles = Data.GetNumParticles();
	int32 CurNumParticles = OrigNumParticles + NumToSpawn;

	// run the spawn graph over all new particles
	if (Props->SpawnScript && Props->SpawnScript->ByteCode.Num())
	{
		VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
		VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
		const int32 NumAttr = Props->SpawnScript->Attributes.Num();
		const int32 NumVectors = NumToSpawn;

		check(NumAttr < VectorVM::MaxInputRegisters);
		check(NumAttr < VectorVM::MaxOutputRegisters);

		FVector4 *NewParticlesStart = Data.GetCurrentBuffer() + OrigNumParticles;

		// Setup input and output registers.
		for (int32 AttrIndex = 0; AttrIndex < NumAttr; ++AttrIndex)
		{
			InputRegisters[AttrIndex] = (VectorRegister*)(NewParticlesStart + AttrIndex * Data.GetParticleAllocation());
			OutputRegisters[AttrIndex] = (VectorRegister*)(NewParticlesStart + AttrIndex * Data.GetParticleAllocation());
		}

		//Fill constant table with required emitter constants and internal script constants.
		TArray<FVector4> ConstantTable;
		TArray<FNiagaraDataObject *>DataObjTable;
		Props->SpawnScript->ConstantData.FillConstantTable(Constants, ConstantTable, DataObjTable);

		VectorVM::Exec(
			Props->SpawnScript->ByteCode.GetData(),
			InputRegisters,
			NumAttr,
			OutputRegisters,
			NumAttr,
			ConstantTable.GetData(),
			DataObjTable.GetData(),
			NumVectors
			);
	}

	//TODO: NEED TO HANDLE PARTICLE DEATH WITHOUT RELYING ON AGE ATTRIBUTE. UNLESS WE FORCE IT.
	//MAYBE WITH BRANCHING IN VM AND A KILL() NODE.

	// Iterate over looking for dead particles and move from the end of the list to the dead location, compacting in the process
	int32 ParticleIndex = 0;
	const FVector4* ParticleRelativeTimes = Data.GetAttributeData(FNiagaraVariableInfo(FName(TEXT("Age")), ENiagaraDataType::Vector));
	if (ParticleRelativeTimes)
	{
		while (ParticleIndex < OrigNumParticles)
		{
			if (ParticleRelativeTimes[ParticleIndex].X > 1.0f)
			{
				// Particle is dead, move one from the end here.
				MoveParticleToIndex(--CurNumParticles, ParticleIndex);
			}
			ParticleIndex++;
		}
	}

	Data.SetNumParticles(CurNumParticles);
	return CurNumParticles;
}

bool FNiagaraSimulation::CheckAttriubtesForRenderer()
{
	bool bOk = true;
	if (EffectRenderer)
	{
		const TArray<FNiagaraVariableInfo>& RequiredAttrs = EffectRenderer->GetRequiredAttributes();

		for (auto& Attr : RequiredAttrs)
		{
			if (!Data.HasAttriubte(Attr))
			{
				bOk = false;
				UE_LOG(LogNiagara, Error, TEXT("Cannot render %s because it does not define attriubte %s."), *Props->Name, *Attr.Name.ToString());
			}
		}
	}
	return bOk;
}

/** Replace the current effect renderer with a new one of Type, transferring the existing material over
 *	to the new renderer. Don't forget to call RenderModuleUpdate on the SceneProxy after calling this! 
 */
void FNiagaraSimulation::SetRenderModuleType(EEmitterRenderModuleType Type, ERHIFeatureLevel::Type FeatureLevel)
{
	if (Type != Props->RenderModuleType || EffectRenderer==nullptr)
	{
		UMaterial *Material = UMaterial::GetDefaultMaterial(MD_Surface);

		if (EffectRenderer)
		{
			Material = EffectRenderer->GetMaterial();
			delete EffectRenderer;
		}
		else
		{
			if (Props->Material)
			{
				Material = Props->Material;
			}
		}

		Props->RenderModuleType = Type;
		switch (Type)
		{
		case RMT_Sprites: EffectRenderer = new NiagaraEffectRendererSprites(FeatureLevel, Props->RendererProperties);
			break;
		case RMT_Ribbon: EffectRenderer = new NiagaraEffectRendererRibbon(FeatureLevel, Props->RendererProperties);
			break;
		default:	EffectRenderer = new NiagaraEffectRendererSprites(FeatureLevel, Props->RendererProperties);
					Props->RenderModuleType = RMT_Sprites;
					break;
		}

		EffectRenderer->SetMaterial(Material, FeatureLevel);
		CheckAttriubtesForRenderer();
	}
}
