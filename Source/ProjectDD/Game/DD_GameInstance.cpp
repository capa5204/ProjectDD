﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "DD_GameInstance.h"

#include "DD_BasicUtilLibrary.h"
#include "DD_SceneManager.h"
#include "Manager/DD_SingletonManager.h"

UDD_GameInstance::UDD_GameInstance()
{
	FWorldDelegates::OnStartGameInstance.AddUObject(this, &UDD_GameInstance::OnStartGameInstance);

	TickDelegateHandle = nullptr;
	ProcessType = EDD_LaunchProcessType::None;
}

UDD_GameInstance::~UDD_GameInstance()
{
	FWorldDelegates::OnStartGameInstance.RemoveAll(this);
}

void UDD_GameInstance::Init()
{
	Super::Init();

	ProcessInitialize();
	DD_CHECK(ProcessType == EDD_LaunchProcessType::ProcessFinished);
}

void UDD_GameInstance::Shutdown()
{
	ProcessFinalize();
	DD_CHECK(ProcessType == EDD_LaunchProcessType::End)

	ProcessType = EDD_LaunchProcessType::None;
	
	Super::Shutdown();
}

void UDD_GameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	Super::OnWorldChanged(OldWorld, NewWorld);
}

void UDD_GameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);

	gSceneMng.SceneLoadComplete(LoadTime, MapName);
}

bool UDD_GameInstance::Tick(float DeltaSeconds)
{
	if (UDD_SingletonManager::GetInstance())
	{
		UDD_SingletonManager::GetInstance()->TickSingletons(DeltaSeconds);
	}

	if(ProcessType == EDD_LaunchProcessType::ProcessFinished)
	{
		gSceneMng.ExecuteLoadLevelDelegate();
		ProcessType = EDD_LaunchProcessType::End;
	}
	
	return true;
}

void UDD_GameInstance::OnStartGameInstance(UGameInstance* GameInstance)
{
	if (UDD_SingletonManager* SingletonManager = UDD_SingletonManager::GetInstance())
	{
		SingletonManager->InitializeSingletons();
	}
}

void UDD_GameInstance::ProcessInitialize()
{
	ProcessType = EDD_LaunchProcessType::CreateBasicUtility;
	if (CreateBasicUtility() == false)
	{
		return;
	}
	
	ProcessType = EDD_LaunchProcessType::CreateManager;
	if (CreateManagers() == false)
	{
		return;
	}
	
	ProcessType = EDD_LaunchProcessType::RegistTick;
	if (RegisterTick() == false)
	{
		return;
	}
	
	ProcessType = EDD_LaunchProcessType::RegistState;
	if (RegisterState() == false)
	{
		return;
	}
	
	ProcessType = EDD_LaunchProcessType::LoadBaseWorld;
	if (LoadBaseWorld() == false)
	{
		return;
	}
	
	ProcessType = EDD_LaunchProcessType::ProcessFinished;
}

void UDD_GameInstance::ProcessFinalize()
{
	if(ProcessType != EDD_LaunchProcessType::End)
	{
		return;
	}

	UnLoadBaseWorld();
	UnRegisterTick();
	DestroyManagers();
	DestroyBasicUtility();
}

bool UDD_GameInstance::CreateBasicUtility()
{
	
	if(UDD_BasicUtilLibrary::HasInstance() == false)
	{
		if(const TObjectPtr<UDD_BasicUtilLibrary> BasicGameUtility = UDD_BasicUtilLibrary::MakeInstance())
		{
			BasicGameUtility->Initialize(this);
			return true;
		}
	}
	return true;
}

bool UDD_GameInstance::DestroyBasicUtility()
{
	if(const TObjectPtr<UDD_BasicUtilLibrary> BasicGameUtility = UDD_BasicUtilLibrary::GetInstance())
	{
		BasicGameUtility->Finalize();
		BasicGameUtility->RemoveInstance();
		return true;
	}
	return true;
}

bool UDD_GameInstance::CreateManagers()
{
	if (UDD_SingletonManager* SingletonManager = UDD_SingletonManager::CreateInstance())
	{
		SingletonManager->BuiltInInitializeSingletons();
	}
	return true;
}

bool UDD_GameInstance::DestroyManagers()
{
	if (UDD_SingletonManager* SingletonManager = UDD_SingletonManager::GetInstance())
	{
		SingletonManager->FinalizeSingletons();
	}

	UDD_SingletonManager::DestroyInstance();

	return true;
}

bool UDD_GameInstance::RegisterTick()
{
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UDD_GameInstance::Tick));
	return true;
}

bool UDD_GameInstance::UnRegisterTick()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	return true;
}

bool UDD_GameInstance::RegisterState()
{
	DD_CHECK(UDD_SceneManager::HasInstance());

	gSceneMng.RegisterScenes();

	return true;
}

bool UDD_GameInstance::LoadBaseWorld()
{
	if(BaseWorld.IsValid())
	{
		gSceneMng.LoadLevelBySoftObjectPtr(BaseWorld, FDD_LoadLevelInitialized::CreateWeakLambda(this, [this](const FString& Value)
		{
			gSceneMng.ChangeScene(EDD_GameSceneType::Logo);
			gSceneMng.SetSceneBehaviorTreeAsset(SceneBTAsset);
			gSceneMng.RegistSceneBehaviorTree();
		}));
		return true;
	}
	return true;
}

bool UDD_GameInstance::UnLoadBaseWorld()
{
	if(BaseWorld.IsValid())
	{
		BaseWorld = nullptr;
		return true;
	}
	
	return true;
}

void UDD_GameInstance::RestartGame()
{
}