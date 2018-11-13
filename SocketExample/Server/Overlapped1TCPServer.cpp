#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {				//소켓 정보 저장을 위한 구조체와 변수
	WSAOVERLAPPED overlapped;	//WSAOVERLAPPED 구조체
	SOCKET sock;				//클라이언트 소켓
	char buf[BUFSIZE + 1];		//응용 프로그램 버퍼
	int recvBytes;				//수신 바이트 수
	int sendBytes;				//송신 바이트 수
	WSABUF wsaBuf;				//WSABUF 구조체
};

int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[WSA_MAXIMUM_WAIT_EVENTS];
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
CRITICAL_SECTION cs;

DWORD WINAPI WorkerThread(LPVOID arg);		//비동기 입출력 처리 함수

BOOL AddSocketInfo(SOCKET sock);			//소켓 관리 함수
void RemoveSocketInfo(int nIndex);

void err_quit(const char* msg);				//오류 처리 함수
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

	//더미(dummy) 이벤트 객체 생성
	WSAEVENT hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {		// '=' 대입연산자로 잘못써서 찾느라 개고생
		err_quit("WSACreateEvent()");
	}
	EventArray[nTotalSockets++] = hEvent;	//EventArray[0]에 핸들을 저장, 이 이벤트 객체는 특정 소켓과 짝짓지 않고 특별한 용도로 사용한다.

	HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);	//비동기 입출력 결과를 처리할 스레드를 생성한다.
	if (hThread == NULL) {
		return 1;
	}
	CloseHandle(hThread);

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

		//소켓 정보 추가
		if (AddSocketInfo(client_sock) == FALSE) {	//소켓 정보를 SocketInfoArray 배열에 저장하고 대응하는 이벤트 객체를 생성하여 EventArray 배열에 저장.
			closesocket(client_sock);
			std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
			continue;
		}

		//비동기 입출력 시작
		SOCKETINFO* ptr = SocketInfoArray[nTotalSockets - 1];
		flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);	//새로 생선된 소켓에 대해 WSARecv() 함수를 호출하여 비동기 입출력 시작.
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSARecv()");
				RemoveSocketInfo(nTotalSockets - 1);
				continue;
			}
		}
		//소켓의 개수(nTotalSockets) 변화를 알림
		WSASetEvent(EventArray[0]);		//EventArray[0]가 가리키는 더미 이벤트 객체를 신호 상태로 만든다. 이렇게하면 WSAWaitForMultipleEvents() 함수가 대기 상태에서 리턴하게 된다.
	}

	//윈속 종료
	WSACleanup();
	DeleteCriticalSection(&cs);
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
	int retval;

	while (TRUE) {
		//이벤트 객체가 신호 상태가 되기를 기다린다.
		DWORD index = WSAWaitForMultipleEvents(nTotalSockets, EventArray, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			err_display("wsa_wait_failed");
			continue;
		}
		index -= WSA_WAIT_EVENT_0;
		WSAResetEvent(EventArray[index]);	//WSAWaitForMultipleEvents() 함수가 리턴하면 이벤트 객체를 비신호 상태로 만들고(WSAResetEvent()) 배열 인덱스를 체크한다.
		if (index == 0) {	//배열 인덱스가 0이라면 EventArray[0]가 신호 상태가 된 것이고, 이는 클라이언트가 접속하여 새로운 소켓 정보가 추가되었다는 뜻이다.
			continue;		//소켓의 총 개수가 변경되었으므로 다시 WSAWaitForMultipleEvents()로 돌아가 다른 이벤트 객체가 신호 상태가 되기를 기다린다.
		}

		//클라이언트 정보 얻기
		SOCKETINFO* ptr = SocketInfoArray[index];
		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
		
		//비동기 입출력 결과 확인
		DWORD cbTransferred, flags;
		retval = WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &cbTransferred, FALSE, &flags);	//비동기 입출력 결과를 확인한다.
		if (retval == FALSE || cbTransferred == 0) {	//오류가 발생하거나 클라이언트가 정상 종료시 소켓 정보를 제거한다.
			RemoveSocketInfo(index);
			std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
			continue;
		}

		//데이터 전송량 갱신	(소켓 정보 구조체를 참조하면 받은 데이터인지 보낸 데이터인지 알 수 있다.)
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
		//WSASend() 함수는 비동기적으로 동작하므로, 실제 보낸 데이터 수는 다음 번에 루프를 돌때 확인할 수 있다.
		if (ptr->recvBytes > ptr->sendBytes) {
			//데이터 보내기
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
			ptr->recvBytes = 0;		//소켓 정보 중 받은 데이터 수를 초기화한 후 도착한 데이터를 읽는다.

			//데이터 받기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->overlapped.hEvent = EventArray[index];
			ptr->wsaBuf.buf = ptr->buf;
			ptr->wsaBuf.len = BUFSIZE;

			//WSARecv() 함수는 비동기적으로 동작하므로, 실제로 받은 데이터 수는, 다음 번에 루프를 돌 때 확인할 수 있다.
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

//소켓 정보 추가
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

//소켓 정보 삭제
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