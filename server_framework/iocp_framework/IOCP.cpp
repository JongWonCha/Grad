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
	WSACleanup();	// 윈속 종료
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

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("[ERROR] Winsock Initialize fail\n");
		return false;
	}

	// socket
	// socket이 아닌 WSASocket을 쓴 이유는 socket은 TCP만 지원한다. 어떤 타입을 쓸 지 모르기 떄문에 WSASocket을 사용함.
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET) {
		printf("[ERROR] socket fail\n");
		return false;
	}


	// bind
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = PF_INET;		// PF_INET 프로토콜 체계
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
	retval = listen(m_listenSocket, SOMAXCONN);		// SOMAXCONN는 backlog 최대치. 추후 수정
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

	// IOCP 객체 생성
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
		// 소켓 정보 구조체 할당
		m_SocketInfo = new SOCKETINFO;
		m_SocketInfo->sock = 0;
		m_SocketInfo->recvbytes = 0;
		m_SocketInfo->sendbytes = 0;
		m_SocketInfo->wsabuf.len = BUFSIZE;
		m_SocketInfo->wsabuf.buf = m_SocketInfo->buf;

		// 비동기 입출력 시작
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

	// CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// (CPU 개수 * 2)개 작업자 스레드 생성
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
	printf("[INFO] Worker Thread 동작\n");
	return true;
}

void IOCP::WorkerThread()
{
	// 함수 호출 성공 여부
	BOOL bRetval;
	int nRetval;

	// OVerlapped IO 작업에서 전송된 데이터 크기
	DWORD recvBytes;
	DWORD sendBytes;
	// ComletionKey
	SOCKETINFO * CompletionKey;
	// IO 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
	SOCKETINFO * SocketInfo;

	DWORD dwFlags = 0;

	while (m_WorkerThread)
	{
		// 쓰레드는 여기서 WaitingThread Queue에 대기상태로 들어간다.
		// Overlapped IO 작업이 완료되면 IOCP Queue에서 완료된 작업을 가져가서 처리한다.
		bRetval = GetQueuedCompletionStatus(m_hcp, &recvBytes, (LPDWORD)&CompletionKey, (LPOVERLAPPED *)&SocketInfo, INFINITE);	// (전송된 바이트, Completion key, Overlapped IO 객체, 대기 시간)

		if (!bRetval && recvBytes == 0)
		{
			printf("[INFO] socket(%d) 접속 끊김\n", SocketInfo->sock);
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

		// SocketInfo 데이터 초기화
		ZeroMemory(&(SocketInfo->overlapped), sizeof(SocketInfo->overlapped));
		SocketInfo->wsabuf.buf = SocketInfo->buf;
		SocketInfo->wsabuf.len = BUFSIZE;

		ZeroMemory(&(SocketInfo->overlapped), BUFSIZE);
		SocketInfo->recvbytes = 0;
		SocketInfo->sendbytes = 0;

		dwFlags = 0;

		// 클라이언트에게 다시 응답을 받기 위해 WSARecv 호출
		nRetval = WSARecv(SocketInfo->sock, &(SocketInfo->wsabuf), 1, &recvBytes, &dwFlags, (LPWSAOVERLAPPED)&(SocketInfo->overlapped), NULL);

		if (nRetval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING)
				printf("[ERROR] WSARecv fail : ", WSAGetLastError());
		}
	}
}