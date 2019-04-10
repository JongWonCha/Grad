// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#pragma comment(lib, "ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <iostream>
#include "CoreMinimal.h"
#include "Runtime/Core/Public/HAL/Runnable.h"

#define	MAX_BUFFER		1024

class AMyPlayerController;
constexpr int MAX_CLIENTS = 5;

//====================================================
													//
struct cs_packet_move {								//
	char size;										//
	char type;										//
};													//
													//
constexpr int SC_LOGIN = 1;							//
constexpr int SC_PUT_PLAYER = 2;					//
constexpr int SC_REMOVE_PLAYER = 3;					//
constexpr int SC_MOVE_PLAYER = 4;					//
													//
struct sc_packet_login {							//
	char size;										//
	char type;										//
	char id;										//
};													//
													//
struct sc_packet_put_player {						//
	char size;										//
	char type;										//
	char id;										//
	char x, y, z, yaw, pitch, roll;					//
};													//
													//
struct sc_packet_remove_player {					//
	char size;										//
	char type;										//
	char id;										//
};													//
													//
struct sc_packet_move_player {						//
	char size;										//
	char type;										//
	char id;										//
	char x, y, z, yaw, pitch, roll;					//
};													//
//====================================================

struct OVER_EX {
	WSAOVERLAPPED	over;
	WSABUF			dataBuffer;
	char			messageBuffer[MAX_BUFFER];
	bool			is_recv;
};

class UserCharacter {
public:
	UserCharacter() {};
	~UserCharacter() {};

	int id;					// ���̵�
	float x, y, z;			// ��ġ
	float Yaw, Pitch, Roll;	// ȸ��

	bool IsAlive;			// ����
	bool IsJump;			// ����
	float health;			// ü��
};

class SOCKETINFO
{
public:
	bool in_use;						// ���� �������
	OVER_EX over_ex;
	SOCKET socket;
	char packet_buffer[MAX_BUFFER];
	int prev_size;
	UserCharacter CharacterInfo;

	SOCKETINFO() {
		in_use = false;
		over_ex.dataBuffer.len = MAX_BUFFER;
		over_ex.dataBuffer.buf = over_ex.messageBuffer;
		over_ex.is_recv = true;
		ZeroMemory(&over_ex.over, sizeof(over_ex.over));
	}
};

class TESTC_API cSocket : public FRunnable
{
public:
	cSocket();
	virtual ~cSocket();

	bool InitSocket();
	bool Connect(const char * pszIP, int nPort);

	void SendPacket(void *packet);

	void MakeNewPlayer(char &id);
	void SetPlayerController(AMyPlayerController* pPlayerController);
	void CloseSocket();

	// FRunnable Threaad members
	FRunnableThread* Thread;
	// For Stop this thread
	FThreadSafeCounter StopTaskCounter;

	// FRunnable override �Լ�
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

	// ������ ���� ����
	bool StartListen();
	void StopListen();

	// �̱��� ��ü ��������
	static cSocket* GetSingleton()
	{
		static cSocket ins;
		return &ins;
	}
private:
	SOCKETINFO ServerSocket;
	AMyPlayerController* PlayerController;
};