// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "cSocket.h"
#include "OtherCharacter.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BATTARYCOLLECTERUE_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AMyPlayerController();
	~AMyPlayerController();

	// id
	UFUNCTION(BlueprintPure, Category = "Properties")
	int GetSessionId();

	// �ٸ� ĳ����
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class ACharacter> WhoToSpawn;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// id ��Ī
	AActor* FindActorBySessionId(TArray<AActor*> ActorArray, const int& SeesionId);

	// ���� ���� ����
	void RecvWorldInfo(cCharactersInfo * ci);
	
private:
	ClientSocket * Socket;
	bool bIsConnected;
	int SessionId;
	cCharactersInfo * ci;

	bool SendPlayerInfo();
	bool UpdateWorldInfo();
	void UpdatePlayerInfo(const location & info);
};
