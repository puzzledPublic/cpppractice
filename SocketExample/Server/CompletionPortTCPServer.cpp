#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {			//소켓 정보 저장을 위한 구조체
	OVERLAPPED overlapped;	//OVERALPPED 구조체
	SOCKET sock;			//클라이언트 소켓
	char buf[BUFSIZE + 1];	//응용 프로그램 버퍼
	int recvBytes;			//송신 바이트 수
	int sendBytes;			//수신 바이트 수
	WSABUF wsaBuf;			//WSABUF 구조체
};

//작업자 스레드 함수 
DWORD WINAPI WorkerThread(LPVOID arg);
//오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;

	//윈속 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	//입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) {
		return 1;
	}
	
	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(CPU 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);	//스레드 함수 인자로 입출력 완료 포트 핸들 값을 전달한다.
		if (hThread == NULL) {
			return 1;
		}
		CloseHandle(hThread);
	}
	//listen할 서버 socket 생성
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

	//데이터 통신에 사용할 변수
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

		std::cout << "\n[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";

		//소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		//소켓 정보 구조체 할당
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == nullptr) {
			break;
		}
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvBytes = ptr->sendBytes = 0;
		ptr->wsaBuf.buf = ptr->buf;
		ptr->wsaBuf.len = BUFSIZE;

		//비동기 입출력 시작	(기다리는 WorkerThread가 깨어나게끔 비동기 함수 호출)
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display("WSARecv()");
			}
			continue;
		}
	}

	//윈속 종료
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;
	HANDLE hcp = (HANDLE)arg;	//스레드 함수 인자로 전달된 입출력 완료 포트 핸들 값을 저장해둔다.

	while (TRUE) {
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		
		//비동기 입출력 완료 기다리기
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (LPDWORD)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

		//클라이언트 정보 얻기
		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);

		//비동기 입출력 결과 확인
		//GetQueuedCompletionStatus() 함수에서 오류 발생시 GetLastError() 함수로 오류 코드를 얻을 수 있다.
		//그러나 이 값은 일반적인 윈도우 API 오류 코드라는 문제가 있다.
		//따라서 WSAGetOverlappedResult() 함수를 호출해 올바른 소켓 오류 코드를 얻은 후 err_display() 함수로 오류 문자열을 출력한다.
		if (retval == 0 || cbTransferred == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &temp1, FALSE, &temp2);
				err_display("WSAGetOverlappedResult()");
			}
			//오류 처리가 끝나면 소켓을 닫고 소켓 정보를 제거한다.
			closesocket(ptr->sock);
			std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
			delete ptr;
			continue;
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
		//보낸 데이터가 받은 데이터보다 적으면 아직 보내지 못한 데이터를 마저 보낸다.
		//WSASend()함수는 비동기적으로 동작하므로 실제로 보낸 데이터 수는 다음 번에 루프를 돌 때 확인할 수 있다.
		if (ptr->recvBytes > ptr->sendBytes) {
			//데이터 보내기
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
			//소켓 정보 중 받은 데이터 수를 초기화한 후 도착한 데이터를 읽는다.
			//WSARecv()함수는 비동기적으로 동작하므로 실제로 받은 데이터 수는 다음 번에 루프를 돌 때 확인할 수 있다.
			ptr->recvBytes = 0;

			//데이터 받기
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