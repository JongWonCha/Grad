// Fill out your copyright notice in the Description page of Project Settings.

#include "cSocket.h"
#include <iostream>
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformAffinity.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "MyPlayerController.h"


ClientSocket::ClientSocket():StopTaskCounter(0)
{
}

ClientSocket::~ClientSocket()
{
	delete Thread;
	Thread = nullptr;

	closesocket(ServerSocket);
	WSACleanup();
}

bool ClientSocket::InitSocket()
{
	WSADATA wsaData;
	// 윈속 버전을 2.2로 초기화
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;		
		return false;
	}

	// TCP 소켓 생성
	// m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	// UdpServerSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (ServerSocket == INVALID_SOCKET) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	// std::cout << "socket initialize success." << std::endl;
	return true;
}

bool ClientSocket::Connect(const char * pszIP, int nPort)
{
	// 접속할 서버 정보를 저장할 구조체
	SOCKADDR_IN stServerAddr;

	stServerAddr.sin_family = AF_INET;
	// 접속할 서버 포트 및 IP
	stServerAddr.sin_port = htons(nPort);
	stServerAddr.sin_addr.s_addr = inet_addr(pszIP);

	int nRet = connect(ServerSocket, (sockaddr*)&stServerAddr, sizeof(sockaddr));
	if (nRet == SOCKET_ERROR) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	// std::cout << "Connection success..." << std::endl;

	return true;
}

void ClientSocket::EnrollCharacterInfo(location & info)
{
	location loc;
	loc.x = info.x;
	loc.y = info.y;
	loc.z = info.z;

	loc.Yaw = info.Yaw;
	loc.Pitch = info.Pitch;
	loc.Roll = info.Roll;

	// 캐릭터 정보 전송
	int nSendLen = send(ServerSocket, (CHAR*)&loc, sizeof(location), 0);
	if (nSendLen == -1)
	{
		return;
	}
}

void ClientSocket::SendMyLocation(location& ActorLocation)
{
	location loc;
	loc.x = ActorLocation.x;
	loc.y = ActorLocation.y;
	loc.z = ActorLocation.z;

	loc.Yaw = ActorLocation.Yaw;
	loc.Pitch= ActorLocation.Pitch;
	loc.Roll = ActorLocation.Roll;

	// char		szOutMsg[1024];

	// sprintf_s(szOutMsg, "X : %f, Y : %f, Z : %f\n", ActorLocation.X, ActorLocation.Y, ActorLocation.Z);

	// int nSendLen = send(m_Socket, szOutMsg, strlen(szOutMsg), 0);	
	int nSendLen = send(ServerSocket, (CHAR*)&loc, sizeof(location), 0);

	if (nSendLen == -1) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		// return FVector();
	}
}

cCharactersInfo * ClientSocket::RecvCharacterInfo(location& Recvp)
{
	CharactersInfo.WorldCharacterInfo->SessionId = Recvp.SessionId;
	CharactersInfo.WorldCharacterInfo->x = Recvp.x;
	CharactersInfo.WorldCharacterInfo->y = Recvp.y;
	CharactersInfo.WorldCharacterInfo->z = Recvp.z;
	CharactersInfo.WorldCharacterInfo->Yaw = Recvp.Yaw;
	CharactersInfo.WorldCharacterInfo->Pitch = Recvp.Pitch;
	CharactersInfo.WorldCharacterInfo->Roll = Recvp.Roll;
	CharactersInfo.WorldCharacterInfo->IsAlive = Recvp.IsAlive;

	return &CharactersInfo;
}

void ClientSocket::SetPlayerController(AMyPlayerController* pPlayerController)
{
	// 플레이어 컨트롤러 세팅
	if (pPlayerController)
	{
		PlayerController = pPlayerController;
	}
}

void ClientSocket::CloseSocket()
{
	closesocket(ServerSocket);
	WSACleanup();
}

bool ClientSocket::Init()
{
	return true;
}

uint32 ClientSocket::Run()
{
	// 서버로부터 데이터 수신
	while (StopTaskCounter.GetValue() == 0 && PlayerController != nullptr)
	{
		location *Recvp;
		int nRecvLen = recv(ServerSocket, (CHAR*)&recvBuffer, MAX_BUFFER, 0);
		if (nRecvLen > 0)
		{
			char* temp = reinterpret_cast<char *>(&Recvp);
			memcpy(temp, recvBuffer, MAX_BUFFER);
			Recvp = reinterpret_cast<location *>(&temp);

			PlayerController->RecvWorldInfo(RecvCharacterInfo(*Recvp));
		}
	}

	return 0;
}

void ClientSocket::Stop()
{
	// thread safety 변수를 조작해 while loop 가 돌지 못하게 함
	StopTaskCounter.Increment();
}

void ClientSocket::Exit()
{
}

bool ClientSocket::StartListen()
{
	// 스레드 시작
	if (Thread != nullptr) return false;
	Thread = FRunnableThread::Create(this, TEXT("ClientSocket"), 0, TPri_BelowNormal);
	return (Thread != nullptr);
}

void ClientSocket::StopListen()
{
	// 스레드 종료
	Stop();
	Thread->WaitForCompletion();
	Thread->Kill();
	delete Thread;
	Thread = nullptr;
	StopTaskCounter.Reset();
}
