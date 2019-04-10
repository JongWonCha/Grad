// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "cSocket.h"
#include "TestCCharacter.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TESTC_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();
	~AMyPlayerController();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	int GetId() { return Playerid; }
	void SetId(int id) { Playerid = id; }

	void MakePlayer(sc_packet_login * packet);
	bool SendPlayerInfo();

	//AActor * FindActorbyId(TArray<AActor*> ActorArray, const int &Playerid);

private:
	cSocket * socket;
	bool bIsConnected;
	int Playerid;

};
