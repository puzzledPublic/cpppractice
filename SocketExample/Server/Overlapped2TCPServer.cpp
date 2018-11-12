#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

//소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO {
	WSAOVERLAPPED overlapped;
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvBytes;
	int sendBytes;
	WSABUF wsaBuf;
};

SOCKET client_sock;
HANDLE hReadEvent, hWriteEvent;

//비동기 입출력 시작과 처리 함수
DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

//오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(const int errcode);

int main(int argc, char* argv[]) {
	int retval;
	
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

	//이벤트 객체 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) {
		return 1;
	}
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) {
		return 1;
	}

	//스레드 생성
	HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
	if (hThread == NULL) {
		return 1;
	}
	CloseHandle(hThread);

	while (TRUE) {
		WaitForSingleObject(hReadEvent, INFINITE);
		//accept()
		client_sock = accept(listen_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		SetEvent(hWriteEvent);
	}

	//윈속 종료
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;

	while (TRUE) {
		while (TRUE) {
			//alertable wait
			DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE);
			if (result == WAIT_OBJECT_0) {
				break;
			}
			if (result != WAIT_IO_COMPLETION) {
				return 1;
			}
		}

		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(client_sock, (SOCKADDR*)&client_addr, &addrLen);
		std::cout << "\n[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";

		//소켓 정보 구조체 할당과 초기화
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == nullptr) {
			std::cout << "[오류] 메모리가 부족합니다." << "\n";
			return 1;
		}
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		SetEvent(hReadEvent);
		ptr->recvBytes = ptr->sendBytes = 0;
		ptr->wsaBuf.buf = ptr->buf;
		ptr->wsaBuf.len = BUFSIZE;

		//비동기 입출력 시작
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

	//클라이언트 정보 얻기
	SOCKETINFO* ptr = (SOCKETINFO*)lpOverlapped;
	SOCKADDR_IN client_addr;
	int addrLen = sizeof(client_addr);
	getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);

	//비동기 입출력 결과 확인
	if (dwError != 0 || cbTransferred == 0) {
		if (dwError != 0) {
			err_display(dwError);
		}
		closesocket(ptr->sock);
		std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
		delete ptr;
		return;
	}

	//데이터 전송량 갱신
	if (ptr->recvBytes == 0) {
		ptr->recvBytes = cbTransferred;
		ptr->sendBytes = 0;
		//받은 데이터 출력
		ptr->buf[ptr->recvBytes] = '\0';
		std::cout << "[TCP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << ptr->buf << "\n";
	}
	else {
		ptr->sendBytes += cbTransferred;
	}

	if (ptr->recvBytes > ptr->sendBytes) {
		//데이터 보내기
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
		ptr->recvBytes = 0;
		//데이터 받기
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
	std::cout << "[오류] " << (char *)lpMsgBuf << "\n";
	LocalFree(lpMsgBuf);
}
