// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include "Runtime/Core/Public/Math/Vector.h"
#include "Runtime/Core/Public/HAL/Runnable.h"

#define	MAX_BUFFER		1024
#define SERVER_PORT		8000
#define SERVER_IP		"127.0.0.1"
#define MAX_CLIENTS		5

class AMyPlayerController;
HANDLE			hIOCP;			// IOCP 객체 핸들

struct stSOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuf;
	SOCKET			socket;
	char			messageBuffer[MAX_BUFFER];
	int				recvBytes;
	int				sendBytes;
};

struct location
{
	int SessionId;
	float x;
	float y;
	float z;

	float Yaw;
	float Pitch;
	float Roll;

	bool IsAlive;
};

class cCharactersInfo
{
public:
	cCharactersInfo() {};
	~cCharactersInfo() {};

	location WorldCharacterInfo[MAX_CLIENTS];

};

/**
 *
 */
class BATTARYCOLLECTERUE_API ClientSocket : public FRunnable
{
public:
	ClientSocket();
	~ClientSocket();

	bool InitSocket();
	bool Connect(const char * pszIP, int nPort);

	// 초기 캐릭터 등록
	void EnrollCharacterInfo(location& info);

	void SendMyLocation(location& ActorLocation);

	void SetPlayerController(AMyPlayerController* pPlayerController);

	void CloseSocket();

	// FRunnable Thread members	
	FRunnableThread* Thread;
	FThreadSafeCounter StopTaskCounter;

	// FRunnable override 함수
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Exit();

	// 스레드 시작 및 종료
	bool StartListen();
	void StopListen();

	static ClientSocket* GetSingleton()
	{
		static ClientSocket ins;
		return &ins;
	}
private:
	SOCKET ServerSocket;
	char 	recvBuffer[MAX_BUFFER];
	cCharactersInfo CharactersInfo;
	AMyPlayerController* PlayerController;

	cCharactersInfo* RecvCharacterInfo(location& Recvp);
};
