#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {				//���� ���� ������ ���� ����ü�� ����
	WSAOVERLAPPED overlapped;	//WSAOVERLAPPED ����ü
	SOCKET sock;				//Ŭ���̾�Ʈ ����
	char buf[BUFSIZE + 1];		//���� ���α׷� ����
	int recvBytes;				//���� ����Ʈ ��
	int sendBytes;				//�۽� ����Ʈ ��
	WSABUF wsaBuf;				//WSABUF ����ü
};

int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[WSA_MAXIMUM_WAIT_EVENTS];
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
CRITICAL_SECTION cs;

DWORD WINAPI WorkerThread(LPVOID arg);		//�񵿱� ����� ó�� �Լ�

BOOL AddSocketInfo(SOCKET sock);			//���� ���� �Լ�
void RemoveSocketInfo(int nIndex);

void err_quit(const char* msg);				//���� ó�� �Լ�
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;
	InitializeCriticalSection(&cs);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		err_quit("bind()");
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		err_quit("listen()");
	}

	//����(dummy) �̺�Ʈ ��ü ����
	WSAEVENT hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {		// '=' ���Կ����ڷ� �߸��Ἥ ã���� �����
		err_quit("WSACreateEvent()");
	}
	EventArray[nTotalSockets++] = hEvent;	//EventArray[0]�� �ڵ��� ����, �� �̺�Ʈ ��ü�� Ư�� ���ϰ� ¦���� �ʰ� Ư���� �뵵�� ����Ѵ�.

	HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);	//�񵿱� ����� ����� ó���� �����带 �����Ѵ�.
	if (hThread == NULL) {
		return 1;
	}
	CloseHandle(hThread);

	//������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int addrLen;
	DWORD recvBytes, flags;

	while (TRUE) {
		//accept()
		addrLen = sizeof(client_addr);
		client_sock = accept(listen_sock, (SOCKADDR*)&client_addr, &addrLen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		
		std::cout << "\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";

		//���� ���� �߰�
		if (AddSocketInfo(client_sock) == FALSE) {	//���� ������ SocketInfoArray �迭�� �����ϰ� �����ϴ� �̺�Ʈ ��ü�� �����Ͽ� EventArray �迭�� ����.
			closesocket(client_sock);
			std::cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";
			continue;
		}

		//�񵿱� ����� ����
		SOCKETINFO* ptr = SocketInfoArray[nTotalSockets - 1];
		flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);	//���� ������ ���Ͽ� ���� WSARecv() �Լ��� ȣ���Ͽ� �񵿱� ����� ����.
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSARecv()");
				RemoveSocketInfo(nTotalSockets - 1);
				continue;
			}
		}
		//������ ����(nTotalSockets) ��ȭ�� �˸�
		WSASetEvent(EventArray[0]);		//EventArray[0]�� ����Ű�� ���� �̺�Ʈ ��ü�� ��ȣ ���·� �����. �̷����ϸ� WSAWaitForMultipleEvents() �Լ��� ��� ���¿��� �����ϰ� �ȴ�.
	}

	//���� ����
	WSACleanup();
	DeleteCriticalSection(&cs);
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;

	while (TRUE) {
		//�̺�Ʈ ��ü�� ��ȣ ���°� �Ǳ⸦ ��ٸ���.
		DWORD index = WSAWaitForMultipleEvents(nTotalSockets, EventArray, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			err_display("wsa_wait_failed");
			continue;
		}
		index -= WSA_WAIT_EVENT_0;
		WSAResetEvent(EventArray[index]);	//WSAWaitForMultipleEvents() �Լ��� �����ϸ� �̺�Ʈ ��ü�� ���ȣ ���·� �����(WSAResetEvent()) �迭 �ε����� üũ�Ѵ�.
		if (index == 0) {	//�迭 �ε����� 0�̶�� EventArray[0]�� ��ȣ ���°� �� ���̰�, �̴� Ŭ���̾�Ʈ�� �����Ͽ� ���ο� ���� ������ �߰��Ǿ��ٴ� ���̴�.
			continue;		//������ �� ������ ����Ǿ����Ƿ� �ٽ� WSAWaitForMultipleEvents()�� ���ư� �ٸ� �̺�Ʈ ��ü�� ��ȣ ���°� �Ǳ⸦ ��ٸ���.
		}

		//Ŭ���̾�Ʈ ���� ���
		SOCKETINFO* ptr = SocketInfoArray[index];
		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
		
		//�񵿱� ����� ��� Ȯ��
		DWORD cbTransferred, flags;
		retval = WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &cbTransferred, FALSE, &flags);	//�񵿱� ����� ����� Ȯ���Ѵ�.
		if (retval == FALSE || cbTransferred == 0) {	//������ �߻��ϰų� Ŭ���̾�Ʈ�� ���� ����� ���� ������ �����Ѵ�.
			RemoveSocketInfo(index);
			std::cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";
			continue;
		}

		//������ ���۷� ����	(���� ���� ����ü�� �����ϸ� ���� ���������� ���� ���������� �� �� �ִ�.)
		if (ptr->recvBytes == 0) {
			ptr->recvBytes = cbTransferred;
			ptr->sendBytes = 0;
			//���� ������ ���
			ptr->buf[ptr->recvBytes] = '\0';
			std::cout << "[TCP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << ptr->buf << "\n";
		}
		else {
			ptr->sendBytes += cbTransferred;
		}
		//���� �����Ͱ� ���� �����ͺ��� ������, ���� ������ ���� �����͸� ���� ������.
		//WSASend() �Լ��� �񵿱������� �����ϹǷ�, ���� ���� ������ ���� ���� ���� ������ ���� Ȯ���� �� �ִ�.
		if (ptr->recvBytes > ptr->sendBytes) {
			//������ ������
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->overlapped.hEvent = EventArray[index];
			ptr->wsaBuf.buf = ptr->buf + ptr->sendBytes;
			ptr->wsaBuf.len = ptr->recvBytes - ptr->sendBytes;

			DWORD sendBytes;
			retval = WSASend(ptr->sock, &ptr->wsaBuf, 1, &sendBytes, 0, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSASend()");
				}
				continue;
			}
		}
		else {
			ptr->recvBytes = 0;		//���� ���� �� ���� ������ ���� �ʱ�ȭ�� �� ������ �����͸� �д´�.

			//������ �ޱ�
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->overlapped.hEvent = EventArray[index];
			ptr->wsaBuf.buf = ptr->buf;
			ptr->wsaBuf.len = BUFSIZE;

			//WSARecv() �Լ��� �񵿱������� �����ϹǷ�, ������ ���� ������ ����, ���� ���� ������ �� �� Ȯ���� �� �ִ�.
			DWORD recvBytes;
			flags = 0;
			retval = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSARecv()");
				}
				continue;
			}
		}
	}
}

//���� ���� �߰�
BOOL AddSocketInfo(SOCKET sock) {
	EnterCriticalSection(&cs);
	if (nTotalSockets >= WSA_MAXIMUM_WAIT_EVENTS) {
		return FALSE;
	}
	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == nullptr) {
		return FALSE;
	}
	WSAEVENT hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {
		return FALSE;
	}

	ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
	ptr->overlapped.hEvent = hEvent;
	ptr->sock = sock;
	ptr->recvBytes = ptr->sendBytes = 0;
	ptr->wsaBuf.buf = ptr->buf;
	ptr->wsaBuf.len = BUFSIZE;
	SocketInfoArray[nTotalSockets] = ptr;
	EventArray[nTotalSockets] = hEvent;
	++nTotalSockets;

	LeaveCriticalSection(&cs);
	return TRUE;
}

//���� ���� ����
void RemoveSocketInfo(int nIndex) {
	EnterCriticalSection(&cs);

	SOCKETINFO* ptr = SocketInfoArray[nIndex];
	closesocket(ptr->sock);
	delete ptr;
	WSACloseEvent(EventArray[nIndex]);

	if (nIndex != (nTotalSockets - 1)) {
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
		EventArray[nIndex] = EventArray[nTotalSockets - 1];
	}
	--nTotalSockets;

	LeaveCriticalSection(&cs);
}

void err_quit(const char* msg) {
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const char* msg) {
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << "[" << msg << "] " << (char *)lpMsgBuf << "\n";
	LocalFree(lpMsgBuf);
}