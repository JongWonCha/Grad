#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>

#define SERVERPORT 9000
#define BUFSIZE 1024

// ���� ���� ������ ���� ����ü
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
	SOCKETINFO* m_SocketInfo;	// ���� ����
	SOCKET		m_listenSocket;	// ���� ���� ����
	HANDLE		m_hcp;			// IOCP ��ü �ڵ�
	bool		m_Accept;
	bool		m_WorkerThread;	// �۾� ������ ���� �÷���
	HANDLE*		m_WorkerHandle;	// �۾� ������ �ڵ�
};