// Fill out your copyright notice in the Description page of Project Settings.

#include "cSocket.h"
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformAffinity.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "MyPlayerController.h"
#include <string>

int saved_packet_size = 0;
DWORD in_packet_size = 0;
char packet_buffer[MAX_BUFFER];

cSocket::cSocket() : StopTaskCounter(0)
{
}

cSocket::~cSocket()
{
	delete Thread;
	Thread = nullptr;

	closesocket(ServerSocket.socket);
	WSACleanup();
}

bool cSocket::InitSocket()
{
	WSADATA wsaData;
	// 윈속 버전을 2.2로 초기화
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0) {
		std::cout << "Error : " << WSAGetLastError() << std::endl;		
		return false;
	}

	ServerSocket.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (ServerSocket.socket == INVALID_SOCKET) return false;
	
	return true;
}

bool cSocket::Connect(const char * pszIP, int nPort)
{
	// 접속할 서버 정보를 저장할 구조체
	SOCKADDR_IN stServerAddr;

	stServerAddr.sin_family = AF_INET;
	// 접속할 서버 포트 및 IP
	stServerAddr.sin_port = htons(nPort);
	stServerAddr.sin_addr.s_addr = inet_addr(pszIP);

	int nRet = connect(ServerSocket.socket, (sockaddr*)&stServerAddr, sizeof(sockaddr));
	if (nRet == SOCKET_ERROR) return false;

	return true;
}

void cSocket::SendPacket(void * packet)
{
	char *p = reinterpret_cast<char *>(packet);
	OVER_EX *ov = new OVER_EX;
	ov->dataBuffer.len = p[0];
	ov->dataBuffer.buf = ov->messageBuffer;
	ov->is_recv = false;
	memcpy(ov->messageBuffer, p, p[0]);
	ZeroMemory(&ov->over, sizeof(ov->over));
	int error = WSASend(ServerSocket.socket, &ov->dataBuffer, 1, 0, 0, &ov->over, NULL);
}

void cSocket::MakeNewPlayer(char &ci)
{
	sc_packet_login packet;

	packet.id = ci;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN;

	SendPacket(&packet);
}

void cSocket::SetPlayerController(AMyPlayerController* pPlayerController)
{
	if (pPlayerController)
	{
		PlayerController = pPlayerController;
	}
}

void cSocket::CloseSocket()
{
	closesocket(ServerSocket.socket);
	WSACleanup();
}

bool cSocket::Init()
{
	return true;
}

uint32  cSocket::Run()
{
	// initial wait before starting
	FPlatformProcess::Sleep(0.03);

	DWORD iobyte, ioflag = 0;
	// recv loop
	while (StopTaskCounter.GetValue() == 0 && PlayerController != nullptr)
	{
		int ret = WSARecv(ServerSocket.socket, &ServerSocket.over_ex.dataBuffer, 1, &iobyte, &ioflag, NULL, NULL);
		BYTE *ptr = reinterpret_cast<BYTE *>(ServerSocket.over_ex.messageBuffer);

		if (ret != 0)
		{
			if (iobyte + saved_packet_size >= in_packet_size) {
				memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
				
				static bool first_time = true;
				switch (packet_buffer[1])
				{
				case SC_LOGIN:
				{
					sc_packet_login *packet = reinterpret_cast<sc_packet_login *>(packet_buffer);
					PlayerController->MakePlayer(packet);
				}
				break;
				case SC_PUT_PLAYER:
					break;
				case SC_REMOVE_PLAYER:
					break;
				case SC_MOVE_PLAYER:
					break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}

void cSocket::Stop()
{
	StopTaskCounter.Increment();
}

bool cSocket::StartListen()
{
	if (Thread != nullptr) return false;
	Thread = FRunnableThread::Create(this, TEXT("cSocket"), 0, TPri_BelowNormal);
	return (Thread != nullptr);
}

void cSocket::StopListen()
{
	if (Thread) {
		Stop();
		Thread->WaitForCompletion();
		Thread->Kill();
		delete Thread;
		Thread = nullptr;
		StopTaskCounter.Reset();
	}
}
