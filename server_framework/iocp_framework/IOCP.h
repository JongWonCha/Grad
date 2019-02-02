#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>

#define SERVERPORT 9000
#define BUFSIZE 1024

// 소켓 정보 저장을 위한 구조체
struct SOCKETINFO
{
	WSAOVERLAPPED overlapped;
	SOCKET sock;
	char buf[BUFSIZE];
	int recvbytes;				
	int sendbytes;				
	WSABUF wsabuf;
};

class IOCP
{
public:
	IOCP();
	~IOCP();

	bool Initialize();
	void StartServer();
	bool CreateWorkerThread();
	void WorkerThread();

private:
	SOCKETINFO* m_SocketInfo;	// 소켓 정보
	SOCKET		m_listenSocket;	// 서버 리슨 소켓
	HANDLE		m_hcp;			// IOCP 객체 핸들
	bool		m_Accept;
	bool		m_WorkerThread;	// 작업 스레드 동작 플래그
	HANDLE*		m_WorkerHandle;	// 작업 스레드 핸들
};