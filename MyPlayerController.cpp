// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include <string>

AMyPlayerController::AMyPlayerController()
{
	socket = cSocket::GetSingleton();
	socket->InitSocket();
	bIsConnected = socket->Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
		socket->SetPlayerController(this);
	}
}

AMyPlayerController::~AMyPlayerController()
{
}

void AMyPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsConnected) return;

	if (!SendPlayerInfo()) return;
}

void AMyPlayerController::BeginPlay()
{
	char TempId = GetId();

	socket->MakeNewPlayer(TempId);
	socket->StartListen();
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// 끊겼다고 서버에 통신 필요
	socket->CloseSocket();
	socket->StopListen();
}

void AMyPlayerController::MakePlayer(sc_packet_login * Rpacket)
{
	auto Player = Cast<ATestCCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player) return;

	SetId(Rpacket->id);
}
/*
AActor * AMyPlayerController::FindActorbyId(TArray<AActor*> ActorArray, const int &Playerid)
{
	for (const auto& Actor : ActorArray)
	{
		if (std::stoi(*Actor->GetName()) == Playerid) return Actor;
	}
	return nullptr;
}
*/
bool AMyPlayerController::SendPlayerInfo()
{
	auto Player = Cast<ATestCCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player) return false;

	auto MyLocation = Player->GetActorLocation();
	auto MyRocation = Player->GetActorRotation();

	sc_packet_move_player packet;

	packet.id = Playerid;
	packet.size = sizeof(packet);
	packet.type = SC_MOVE_PLAYER;

	packet.x = MyLocation.X;
	packet.y = MyLocation.Y;
	packet.z = MyLocation.Z;
	packet.yaw = MyRocation.Yaw;
	packet.pitch = MyRocation.Pitch;
	packet.roll = MyRocation.Roll;

	socket->SendPacket(&packet);

	return true;
}