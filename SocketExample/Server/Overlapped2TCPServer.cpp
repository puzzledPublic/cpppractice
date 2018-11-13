#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

//소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO {
	WSAOVERLAPPED overlapped;	//WSAOVERLAPPED 구조체
	SOCKET sock;				//클라이언트 소켓
	char buf[BUFSIZE + 1];		//응용 프로그램 버퍼
	int recvBytes;				//송신 바이트 수
	int sendBytes;				//수신 바이트 수
	WSABUF wsaBuf;				//WSABUF 구조체
};

SOCKET client_sock;				//accept() 함수의 리턴 값을 저장할 변수, 두 스레드에 접근하므로 전역 변수로 선언했다.
HANDLE hReadEvent, hWriteEvent;	//client_sock 변수를 보호하기 위한 이벤트 객체 핸들

//비동기 입출력 시작과 처리 함수
DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

//오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(const int errcode);

int main(int argc, char* argv[]) {
	int retval;
	//윈속 초기화	
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//listen할 소켓 생성(listen_sock)
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
	}
	//소켓 주소 구조체를 만들어 listen_sock과 bind()
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

	//이벤트 객체 생성
	//hReadEvent 이벤트 객체는 WorkerThread 스레드가 client_sock 변수 값을 읽었음을 메인 스레드에 알리는 용도로 사용한다.
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) {
		return 1;
	}
	//hWriteEvent 이벤트 객체는 메인 스레드가 client_sock 변수 값을 변경했음을 alertable wait 상태인 WorkerThread 스레드에 알리는 용도로 사용한다.
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) {
		return 1;
	}

	//스레드 생성. 이 스레드는 alertable wait 상태가 됨으로써, 비동기 입출력이 완료하면 완료 루틴이 호출되게 한다.
	HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
	if (hThread == NULL) {
		return 1;
	}
	CloseHandle(hThread);

	while (TRUE) {
		WaitForSingleObject(hReadEvent, INFINITE);	//hReadEvent 이벤트 객체의 신호 상태를 기다린다. 생성할때 초기상태는 신호 상태를 줬으므로 바로 리턴한다.
		//accept()
		client_sock = accept(listen_sock, NULL, NULL);	//클라이언트가 접속을 할때마다 hWriteEvent 이벤트 객체를 신호 상태로 만들어 alertable wait 상태인 스레드를 깨운다.
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		SetEvent(hWriteEvent);	//hWriteEvent 이벤트 객체를 신호 상태로 변환.
	}

	//윈속 종료
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;

	while (TRUE) {
		while (TRUE) {
			//WaitForSingleObjectEx() 함수를 호출해 alertable wait 상태에 진입한다.
			DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE);
			if (result == WAIT_OBJECT_0) {	//새로운 클라이언트가 접속한 경우므로 루프를 벗어난다.
				break;
			}
			if (result != WAIT_IO_COMPLETION) {	//비동기 입출력 작업과 이에 따른 완료 루틴 호출이 끝난 경우이므로 다시 alertable wait 상태로 진입한다.
				return 1;
			}
		}

		//접속한 클라이언트 정보 출력
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
		SetEvent(hReadEvent);	//client_sock 변수 값을 읽어가는 즉시 hReadEvent 이벤트 객체를 신호 상태로 만든다.
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
	if (dwError != 0 || cbTransferred == 0) {	//오류가 발생했거나 클라이언트가 정상 종료한 경우면 소켓 정보를 제거한다.
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
	//보낸 데이터가 받은 데이터보다 적으면, 아직 보내지 못한 데이터를 마저 보낸다.
	//WSASend() 함수는 비동기적으로 동작하므로 실제로 보낸 데이터 수는 다음 번에 완료 루틴이 호출되면 확인할 수 있다.
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
		//소켓 정보 중 받은 데이터 수를 초기화 후 도착한 데이터를 읽는다.
		//WSARecv() 함수는 비동기적으로 동작하므로 실제로 받은 데이터 수는 다음 번에 완료 루틴이 호출되면 확인할 수 있다.
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
