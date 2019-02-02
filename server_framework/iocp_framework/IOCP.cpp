#include <stdio.h>
#include <process.h>
#include "IOCP.h"

unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	IOCP* OverlappedEvent = (IOCP*)p;
	OverlappedEvent->WorkerThread();
	return 0;
}

IOCP::IOCP()
{
	m_WorkerThread = true;
	m_Accept = true;
}

IOCP::~IOCP()
{
	WSACleanup();	// ���� ����
	if (m_SocketInfo) {
		delete[] m_SocketInfo;
		m_SocketInfo = NULL;
	}
	if (m_WorkerHandle) {
		delete[] m_WorkerHandle;
		m_WorkerHandle = NULL;
	}
}

bool IOCP::Initialize()
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("[ERROR] Winsock Initialize fail\n");
		return false;
	}

	// socket
	// socket�� �ƴ� WSASocket�� �� ������ socket�� TCP�� �����Ѵ�. � Ÿ���� �� �� �𸣱� ������ WSASocket�� �����.
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET) {
		printf("[ERROR] socket fail\n");
		return false;
	}


	// bind
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = PF_INET;		// PF_INET �������� ü��
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(m_listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		printf("[ERROR] bind fail\n");
		closesocket(m_listenSocket);
		WSACleanup();
		return false;
	}

	// listen
	retval = listen(m_listenSocket, SOMAXCONN);		// SOMAXCONN�� backlog �ִ�ġ. ���� ����
	if (retval == SOCKET_ERROR) {
		printf("[ERROR] listen fail\n");
		closesocket(m_listenSocket);
		WSACleanup();
		return false;
	}

	return true;
}

void IOCP::StartServer()
{
	int retval;

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes, flags;

	// IOCP ��ü ����
	m_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hcp == NULL) return;
	
	// Worker Thread
	if (!CreateWorkerThread()) return;
	
	printf("[INFO]Server Start...\n");

	// accept
	while (m_Accept)
	{
		client_sock = WSAAccept(
			m_listenSocket, (SOCKADDR *)&clientaddr, &addrlen, NULL, NULL);
		if (client_sock == INVALID_SOCKET) {
			printf("[ERROR] accept fail\n");
			return;
		}
		// ���� ���� ����ü �Ҵ�
		m_SocketInfo = new SOCKETINFO;
		m_SocketInfo->sock = 0;
		m_SocketInfo->recvbytes = 0;
		m_SocketInfo->sendbytes = 0;
		m_SocketInfo->wsabuf.len = BUFSIZE;
		m_SocketInfo->wsabuf.buf = m_SocketInfo->buf;

		// �񵿱� ����� ����
		flags = 0;
		m_hcp = CreateIoCompletionPort((HANDLE)client_sock, m_hcp, (DWORD)m_SocketInfo, 0);
		retval = WSARecv(m_SocketInfo->sock, &m_SocketInfo->wsabuf, 1, &recvbytes, &flags, &(m_SocketInfo->overlapped), NULL);

		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("[ERROR] IO_Pending fail : ", WSAGetLastError());
				return;
			}
		}
	}
}

bool IOCP::CreateWorkerThread()
{
	unsigned int threadId;

	// CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// (CPU ���� * 2)�� �۾��� ������ ����
	m_WorkerHandle = new HANDLE[si.dwNumberOfProcessors * 2];
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		m_WorkerHandle[i] = (HANDLE *)_beginthreadex(
			NULL, 0, &CallWorkerThread, this, CREATE_SUSPENDED, &threadId
		);
		if (m_WorkerHandle[i] = NULL) {
			printf("[ERROR] Worker Thread fail\n");
			return false;
		}
		ResumeThread(m_WorkerHandle[i]);
	}
	printf("[INFO] Worker Thread ����\n");
	return true;
}

void IOCP::WorkerThread()
{
	// �Լ� ȣ�� ���� ����
	BOOL bRetval;
	int nRetval;

	// OVerlapped IO �۾����� ���۵� ������ ũ��
	DWORD recvBytes;
	DWORD sendBytes;
	// ComletionKey
	SOCKETINFO * CompletionKey;
	// IO �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
	SOCKETINFO * SocketInfo;

	DWORD dwFlags = 0;

	while (m_WorkerThread)
	{
		// ������� ���⼭ WaitingThread Queue�� �����·� ����.
		// Overlapped IO �۾��� �Ϸ�Ǹ� IOCP Queue���� �Ϸ�� �۾��� �������� ó���Ѵ�.
		bRetval = GetQueuedCompletionStatus(m_hcp, &recvBytes, (LPDWORD)&CompletionKey, (LPOVERLAPPED *)&SocketInfo, INFINITE);	// (���۵� ����Ʈ, Completion key, Overlapped IO ��ü, ��� �ð�)

		if (!bRetval && recvBytes == 0)
		{
			printf("[INFO] socket(%d) ���� ����\n", SocketInfo->sock);
			closesocket(SocketInfo->sock);
			free(SocketInfo);
			continue;
		}
		SocketInfo->wsabuf.len = recvBytes;
		if (recvBytes == 0)
		{
			closesocket(SocketInfo->sock);
			free(SocketInfo);
			continue;
		}
		else
		{
			printf("[INFO] Bytes : [%d], Msg : [%s]\n", SocketInfo->wsabuf.len, SocketInfo->wsabuf.buf);
			nRetval = WSASend(SocketInfo->sock, &(SocketInfo->wsabuf), 1, &sendBytes, dwFlags, NULL, NULL);
		}

		if (nRetval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				printf("[ERROR] WSASend fail : ", WSAGetLastError());
			}
		}

		printf("[INFO] Msg Send - Bytes : [%d], Msg : [%s]\n", SocketInfo->wsabuf.len, SocketInfo->wsabuf.buf);

		// SocketInfo ������ �ʱ�ȭ
		ZeroMemory(&(SocketInfo->overlapped), sizeof(SocketInfo->overlapped));
		SocketInfo->wsabuf.buf = SocketInfo->buf;
		SocketInfo->wsabuf.len = BUFSIZE;

		ZeroMemory(&(SocketInfo->overlapped), BUFSIZE);
		SocketInfo->recvbytes = 0;
		SocketInfo->sendbytes = 0;

		dwFlags = 0;

		// Ŭ���̾�Ʈ���� �ٽ� ������ �ޱ� ���� WSARecv ȣ��
		nRetval = WSARecv(SocketInfo->sock, &(SocketInfo->wsabuf), 1, &recvBytes, &dwFlags, (LPWSAOVERLAPPED)&(SocketInfo->overlapped), NULL);

		if (nRetval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING)
				printf("[ERROR] WSARecv fail : ", WSAGetLastError());
		}
	}
}