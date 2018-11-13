#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

//���� ���� ������ ���� ����ü�� ����
struct SOCKETINFO {
	WSAOVERLAPPED overlapped;	//WSAOVERLAPPED ����ü
	SOCKET sock;				//Ŭ���̾�Ʈ ����
	char buf[BUFSIZE + 1];		//���� ���α׷� ����
	int recvBytes;				//�۽� ����Ʈ ��
	int sendBytes;				//���� ����Ʈ ��
	WSABUF wsaBuf;				//WSABUF ����ü
};

SOCKET client_sock;				//accept() �Լ��� ���� ���� ������ ����, �� �����忡 �����ϹǷ� ���� ������ �����ߴ�.
HANDLE hReadEvent, hWriteEvent;	//client_sock ������ ��ȣ�ϱ� ���� �̺�Ʈ ��ü �ڵ�

//�񵿱� ����� ���۰� ó�� �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

//���� ��� �Լ�
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(const int errcode);

int main(int argc, char* argv[]) {
	int retval;
	//���� �ʱ�ȭ	
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//listen�� ���� ����(listen_sock)
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
	}
	//���� �ּ� ����ü�� ����� listen_sock�� bind()
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
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

	//�̺�Ʈ ��ü ����
	//hReadEvent �̺�Ʈ ��ü�� WorkerThread �����尡 client_sock ���� ���� �о����� ���� �����忡 �˸��� �뵵�� ����Ѵ�.
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) {
		return 1;
	}
	//hWriteEvent �̺�Ʈ ��ü�� ���� �����尡 client_sock ���� ���� ���������� alertable wait ������ WorkerThread �����忡 �˸��� �뵵�� ����Ѵ�.
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) {
		return 1;
	}

	//������ ����. �� ������� alertable wait ���°� �����ν�, �񵿱� ������� �Ϸ��ϸ� �Ϸ� ��ƾ�� ȣ��ǰ� �Ѵ�.
	HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
	if (hThread == NULL) {
		return 1;
	}
	CloseHandle(hThread);

	while (TRUE) {
		WaitForSingleObject(hReadEvent, INFINITE);	//hReadEvent �̺�Ʈ ��ü�� ��ȣ ���¸� ��ٸ���. �����Ҷ� �ʱ���´� ��ȣ ���¸� �����Ƿ� �ٷ� �����Ѵ�.
		//accept()
		client_sock = accept(listen_sock, NULL, NULL);	//Ŭ���̾�Ʈ�� ������ �Ҷ����� hWriteEvent �̺�Ʈ ��ü�� ��ȣ ���·� ����� alertable wait ������ �����带 �����.
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		SetEvent(hWriteEvent);	//hWriteEvent �̺�Ʈ ��ü�� ��ȣ ���·� ��ȯ.
	}

	//���� ����
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;

	while (TRUE) {
		while (TRUE) {
			//WaitForSingleObjectEx() �Լ��� ȣ���� alertable wait ���¿� �����Ѵ�.
			DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE);
			if (result == WAIT_OBJECT_0) {	//���ο� Ŭ���̾�Ʈ�� ������ ���Ƿ� ������ �����.
				break;
			}
			if (result != WAIT_IO_COMPLETION) {	//�񵿱� ����� �۾��� �̿� ���� �Ϸ� ��ƾ ȣ���� ���� ����̹Ƿ� �ٽ� alertable wait ���·� �����Ѵ�.
				return 1;
			}
		}

		//������ Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(client_sock, (SOCKADDR*)&client_addr, &addrLen);
		std::cout << "\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";

		//���� ���� ����ü �Ҵ�� �ʱ�ȭ
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == nullptr) {
			std::cout << "[����] �޸𸮰� �����մϴ�." << "\n";
			return 1;
		}
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		SetEvent(hReadEvent);	//client_sock ���� ���� �о�� ��� hReadEvent �̺�Ʈ ��ü�� ��ȣ ���·� �����.
		ptr->recvBytes = ptr->sendBytes = 0;
		ptr->wsaBuf.buf = ptr->buf;
		ptr->wsaBuf.len = BUFSIZE;

		//�񵿱� ����� ����
		DWORD recvBytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSARecv()");
				return 1;
			}
		}
	}
	return 0;
}

void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags) {
	int retval;

	//Ŭ���̾�Ʈ ���� ���
	SOCKETINFO* ptr = (SOCKETINFO*)lpOverlapped;
	SOCKADDR_IN client_addr;
	int addrLen = sizeof(client_addr);
	getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);

	//�񵿱� ����� ��� Ȯ��
	if (dwError != 0 || cbTransferred == 0) {	//������ �߻��߰ų� Ŭ���̾�Ʈ�� ���� ������ ���� ���� ������ �����Ѵ�.
		if (dwError != 0) {
			err_display(dwError);
		}
		closesocket(ptr->sock);
		std::cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";
		delete ptr;
		return;
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
	//���� �����Ͱ� ���� �����ͺ��� ������, ���� ������ ���� �����͸� ���� ������.
	//WSASend() �Լ��� �񵿱������� �����ϹǷ� ������ ���� ������ ���� ���� ���� �Ϸ� ��ƾ�� ȣ��Ǹ� Ȯ���� �� �ִ�.
	if (ptr->recvBytes > ptr->sendBytes) {
		//������ ������
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->wsaBuf.buf = ptr->buf + ptr->sendBytes;
		ptr->wsaBuf.len = ptr->recvBytes - ptr->sendBytes;

		DWORD sendBytes;
		retval = WSASend(ptr->sock, &ptr->wsaBuf, 1, &sendBytes, 0, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSASend()");
				return;
			}
		}
	}
	else {
		//���� ���� �� ���� ������ ���� �ʱ�ȭ �� ������ �����͸� �д´�.
		//WSARecv() �Լ��� �񵿱������� �����ϹǷ� ������ ���� ������ ���� ���� ���� �Ϸ� ��ƾ�� ȣ��Ǹ� Ȯ���� �� �ִ�.
		ptr->recvBytes = 0;
		//������ �ޱ�
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->wsaBuf.buf = ptr->buf;
		ptr->wsaBuf.len = BUFSIZE;

		DWORD recvBytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSARecv()");
				return;
			}
		}
	}
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
void err_display(const int errcode) {
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << "[����] " << (char *)lpMsgBuf << "\n";
	LocalFree(lpMsgBuf);
}
