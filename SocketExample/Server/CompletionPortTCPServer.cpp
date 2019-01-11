#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {			//���� ���� ������ ���� ����ü
	OVERLAPPED overlapped;	//OVERALPPED ����ü
	SOCKET sock;			//Ŭ���̾�Ʈ ����
	char buf[BUFSIZE + 1];	//���� ���α׷� ����
	int recvBytes;			//�۽� ����Ʈ ��
	int sendBytes;			//���� ����Ʈ ��
	WSABUF wsaBuf;			//WSABUF ����ü
};

//�۾��� ������ �Լ� 
DWORD WINAPI WorkerThread(LPVOID arg);
//���� ��� �Լ�
void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;

	//���� �ʱ�ȭ
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	//����� �Ϸ� ��Ʈ ����
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) {
		return 1;
	}
	
	//CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(CPU ���� * 2)���� �۾��� ������ ����
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);	//������ �Լ� ���ڷ� ����� �Ϸ� ��Ʈ �ڵ� ���� �����Ѵ�.
		if (hThread == NULL) {
			return 1;
		}
		CloseHandle(hThread);
	}
	//listen�� ���� socket ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		err_quit("bind()");
	}

	//listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		err_quit("listen()");
	}

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

		//���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		//���� ���� ����ü �Ҵ�
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == nullptr) {
			break;
		}
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvBytes = ptr->sendBytes = 0;
		ptr->wsaBuf.buf = ptr->buf;
		ptr->wsaBuf.len = BUFSIZE;

		//�񵿱� ����� ����	(��ٸ��� WorkerThread�� ����Բ� �񵿱� �Լ� ȣ��)
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display("WSARecv()");
			}
			continue;
		}
	}

	//���� ����
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;
	HANDLE hcp = (HANDLE)arg;	//������ �Լ� ���ڷ� ���޵� ����� �Ϸ� ��Ʈ �ڵ� ���� �����صд�.

	while (TRUE) {
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		
		//�񵿱� ����� �Ϸ� ��ٸ���
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (LPDWORD)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

		//Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);

		//�񵿱� ����� ��� Ȯ��
		//GetQueuedCompletionStatus() �Լ����� ���� �߻��� GetLastError() �Լ��� ���� �ڵ带 ���� �� �ִ�.
		//�׷��� �� ���� �Ϲ����� ������ API ���� �ڵ��� ������ �ִ�.
		//���� WSAGetOverlappedResult() �Լ��� ȣ���� �ùٸ� ���� ���� �ڵ带 ���� �� err_display() �Լ��� ���� ���ڿ��� ����Ѵ�.
		if (retval == 0 || cbTransferred == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &temp1, FALSE, &temp2);
				err_display("WSAGetOverlappedResult()");
			}
			//���� ó���� ������ ������ �ݰ� ���� ������ �����Ѵ�.
			closesocket(ptr->sock);
			std::cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";
			delete ptr;
			continue;
		}

		//������ ���۷� ����
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
		//���� �����Ͱ� ���� �����ͺ��� ������ ���� ������ ���� �����͸� ���� ������.
		//WSASend()�Լ��� �񵿱������� �����ϹǷ� ������ ���� ������ ���� ���� ���� ������ �� �� Ȯ���� �� �ִ�.
		if (ptr->recvBytes > ptr->sendBytes) {
			//������ ������
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
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
			//���� ���� �� ���� ������ ���� �ʱ�ȭ�� �� ������ �����͸� �д´�.
			//WSARecv()�Լ��� �񵿱������� �����ϹǷ� ������ ���� ������ ���� ���� ���� ������ �� �� Ȯ���� �� �ִ�.
			ptr->recvBytes = 0;

			//������ �ޱ�
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsaBuf.buf = ptr->buf;
			ptr->wsaBuf.len = BUFSIZE;

			DWORD recvBytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSARecv()");
				}
				continue;
			}
		}
	}
	return 0;
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