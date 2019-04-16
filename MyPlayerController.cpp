// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include <string>

AMyPlayerController::AMyPlayerController()
{
	SessionId = FMath::RandRange(0, 100);

	// 서버와 연결
	Socket = ClientSocket::GetSingleton();
	Socket->InitSocket();
	bIsConnected = Socket->Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
		Socket->SetPlayerController(this);
	}
}

AMyPlayerController::~AMyPlayerController()
{

}

int AMyPlayerController::GetSessionId()
{
	return SessionId;
}

void AMyPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsConnected) return;

	// 플레이어 정보 송신
	if (!SendPlayerInfo()) return;

	// 월드 동기화
	if (!UpdateWorldInfo()) return;
}

void AMyPlayerController::BeginPlay()
{
	auto Player = Cast<AbattarycollecterUECharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return;

	auto MyLocation = Player->GetActorLocation();
	auto MyRotation = Player->GetActorRotation();

	location Character;

	Character.SessionId = SessionId;
	Character.x = MyLocation.X;
	Character.y = MyLocation.Y;
	Character.z = MyLocation.Z;

	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;

	Character.IsAlive = true;

	Socket->EnrollCharacterInfo(Character);
	Socket->StartListen();
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Socket->CloseSocket();
	Socket->StopListen();
}

AActor * AMyPlayerController::FindActorBySessionId(TArray<AActor*> ActorArray, const int & SessionId)
{
	for (const auto& Actor : ActorArray)
	{
		if (std::stoi(*Actor->GetName()) == SessionId)
			return Actor;
	}
	return nullptr;
}

void AMyPlayerController::RecvWorldInfo(cCharactersInfo * ci_)
{
	ci = ci_;
}

bool AMyPlayerController::SendPlayerInfo()
{
	auto Player = Cast<AbattarycollecterUECharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return;

	auto MyLocation = Player->GetActorLocation();
	auto MyRotation = Player->GetActorRotation();

	location Character;

	Character.SessionId = SessionId;
	Character.x = MyLocation.X;
	Character.y = MyLocation.Y;
	Character.z = MyLocation.Z;

	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;

	Socket->SendMyLocation(Character);

	return true;
}

bool AMyPlayerController::UpdateWorldInfo()
{
	UWorld* const world = GetWorld();
	if (world == nullptr)
		return false;

	if (ci == nullptr)
		return false;

	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOtherCharacter::StaticClass(), SpawnedCharacters);


	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int CharacterSessionId = ci->WorldCharacterInfo[i].SessionId;
		// 플레이어 처리
		if (CharacterSessionId == SessionId)
		{
			UpdatePlayerInfo(ci->WorldCharacterInfo[i]);
			continue;
		}
		// 다른 네트워크 캐릭터 처리
		if (CharacterSessionId != -1)
		{
			// 월드내 해당 세션 아이디와 매칭되는 Actor 검색			
			auto Actor = FindActorBySessionId(SpawnedCharacters, CharacterSessionId);
			// 해당되는 세션 아이디가 없을 시 월드에 스폰
			if (Actor == nullptr && ci->WorldCharacterInfo[i].IsAlive == true)
			{
				FVector SpawnLocation;
				SpawnLocation.X = ci->WorldCharacterInfo[i].x;
				SpawnLocation.Y = ci->WorldCharacterInfo[i].y;
				SpawnLocation.Z = ci->WorldCharacterInfo[i].z;

				FRotator SpawnRotation;
				SpawnRotation.Yaw = ci->WorldCharacterInfo[i].Yaw;
				SpawnRotation.Pitch = ci->WorldCharacterInfo[i].Pitch;
				SpawnRotation.Roll = ci->WorldCharacterInfo[i].Roll;

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = Instigator;
				SpawnParams.Name = FName(*FString(std::to_string(ci->WorldCharacterInfo[i].SessionId).c_str()));

				ACharacter* const SpawnCharacter = world->SpawnActor<ACharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			}
			// 해당되는 세션 아이다가 있으면 위치 동기화
			else if (Actor != nullptr && ci->WorldCharacterInfo[i].IsAlive == true)
			{
				AOtherCharacter* OtherCharacter = Cast<AOtherCharacter>(Actor);
				if (OtherCharacter)
				{

					FVector CharacterLocation;
					CharacterLocation.X = ci->WorldCharacterInfo[CharacterSessionId].x;
					CharacterLocation.Y = ci->WorldCharacterInfo[CharacterSessionId].y;
					CharacterLocation.Z = ci->WorldCharacterInfo[CharacterSessionId].z;

					FRotator CharacterRotation;
					CharacterRotation.Yaw = ci->WorldCharacterInfo[CharacterSessionId].Yaw;
					CharacterRotation.Pitch = ci->WorldCharacterInfo[CharacterSessionId].Pitch;
					CharacterRotation.Roll = ci->WorldCharacterInfo[CharacterSessionId].Roll;

					OtherCharacter->SetActorLocation(CharacterLocation);
					OtherCharacter->SetActorRotation(CharacterRotation);
				}
			}
			else if (Actor != nullptr && ci->WorldCharacterInfo[i].IsAlive == false)
			{
				Actor->Destroy();
			}
		}
	}

	return true;
}

void AMyPlayerController::UpdatePlayerInfo(const location & info)
{
	auto Player = Cast<AbattarycollecterUECharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	UWorld* const world = GetWorld();

	if (!info.IsAlive)
	{
		Player->Destroy();
	}
}