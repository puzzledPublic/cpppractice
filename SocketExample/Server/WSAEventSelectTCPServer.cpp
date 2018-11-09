#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {			//소켓 정보 저장을 위한 구조체
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvBytes;
	int sendBytes;
};

int nTotalSockets = 0;		//SOCKETINFO 구조체의 개수, 소켓 생성마다 1씩 증가, 소켓 닫을 때마다 1씩 감소
SOCKETINFO* SocketInfoArray[WSA_MAXIMUM_WAIT_EVENTS];	//SOCKETINFO 구조체 포인터를 저장할 배열
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];			//소켓과 짝지을 이벤트 객체 핸들을 저장할 배열

//소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

//오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);

int main(int argc, char* argv[]) {
	int retval;

	//윈속 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	//listen socket 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	//bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		err_quit("bind()");
	}

	//listen
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		err_quit("listen()");
	}

	//소켓 정보 추가 & WSAEventSelect()
	AddSocketInfo(listen_sock);
	//연결 대기 소켓과 이벤트 객체를 짝짓는다. 연결 대기 소켓은 FD_ACCEPT와 FD_CLOSE 두 개의 네트워크 이벤트만 처리하면 된다.
	retval = WSAEventSelect(listen_sock, EventArray[nTotalSockets - 1], FD_ACCEPT | FD_CLOSE);
	if (retval == SOCKET_ERROR) {
		err_quit("WSAEventSelect()");
	}

	//데이터 통신에 사용할 변수
	WSANETWORKEVENTS NetworkEvents;
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int i, addrLen;

	while (TRUE) {
		//이벤트 객체 관찰하기
		i = WSAWaitForMultipleEvents(nTotalSockets, EventArray, FALSE, WSA_INFINITE, FALSE);	//이벤트 객체가 신호 상태가 될 때까지 대기
		if (i == WSA_WAIT_FAILED) {
			continue;
		}
		//WSAWaitForMultipleEvents() 함수의 리턴 값은 신호 상태가 된 이벤트 객체의 배열 인덱스 + WSA_WAIT_EVENT_0 값이다.
		//따라서 실제 인덱스 값을 얻으려면 WSA_WAIT_EVENTS_0값을 빼야한다.
		i -= WSA_WAIT_EVENT_0;

		//구체적인 네트워크 이벤트 알아내기
		retval = WSAEnumNetworkEvents(SocketInfoArray[i]->sock, EventArray[i], &NetworkEvents);
		if (retval == SOCKET_ERROR) {
			continue;
		}

		//FD_ACCEPT 이벤트 처리
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT) {
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
				err_display(NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
				continue;
			}
			addrLen = sizeof(client_addr);
			client_sock = accept(SocketInfoArray[i]->sock, (SOCKADDR*)&client_addr, &addrLen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				continue;
			}
			std::cout << "\n[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
			if (nTotalSockets >= WSA_MAXIMUM_WAIT_EVENTS) {		//현재 연결된 소켓 개수가 WSA_MAXIMUM_WAIT_EVENTS(64)를 넘어서면 더 접속을 받을 수 없다.
				std::cout << "[오류] 더 이상 접속을 받아들일 수 없습니다." << "\n";
				closesocket(client_sock);
				continue;
			}
			//소켓 정보 추가 & WSAEventSelect();
			AddSocketInfo(client_sock);
			retval = WSAEventSelect(client_sock, EventArray[nTotalSockets - 1], FD_READ | FD_WRITE | FD_CLOSE);		//클라이언트 소켓에 FD_READ, FD_WRITE, FD_CLOSE 이벤트를 등록한다.
			if (retval == SOCKET_ERROR) {
				err_quit("WSAEventSelect()");
			}
		}

		//FD_READ & FD_WRITE 이벤트 처리
		if (NetworkEvents.lNetworkEvents & FD_READ || NetworkEvents.lNetworkEvents & FD_WRITE) {
			if (NetworkEvents.lNetworkEvents & FD_READ && NetworkEvents.iErrorCode[FD_READ_BIT] != 0) {
				err_display(NetworkEvents.iErrorCode[FD_READ_BIT]);
				continue;
			}
			if (NetworkEvents.lNetworkEvents & FD_WRITE && NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0) {
				err_display(NetworkEvents.iErrorCode[FD_WRITE_BIT]);
				continue;
			}
			SOCKETINFO* ptr = SocketInfoArray[i];
			if (ptr->recvBytes == 0) {			//받은 바이트 수가 0일 경우에만 recv()함수를 호출해 데이터를 읽는다.
				//데이터 받기
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						err_display("recv()");
						RemoveSocketInfo(i);
					}
					continue;
				}
				ptr->recvBytes = retval;
				ptr->buf[retval] = '\0';
				addrLen = sizeof(client_addr);
				getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
				std::cout << "[TCP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << ptr->buf << "\n";
			}

			if (ptr->recvBytes > ptr->sendBytes) {		//받은 바이트 수가 보낸 바이트 수보다 크다면 send() 함수를 호출하여 데이터를 보낸다.
				//데이터 보내기
				retval = send(ptr->sock, ptr->buf + ptr->sendBytes, ptr->recvBytes - ptr->sendBytes, 0);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						err_display("send()");
						RemoveSocketInfo(i);
					}
					continue;
				}
				ptr->sendBytes += retval;
				//받은 데이터를 모두 보냈는지 체크
				if (ptr->recvBytes == ptr->sendBytes) {		//받은 만큼 모두 보냈다면 받은 바이트 수와 보낸 바이트 수를 다시 0으로 초기화
					ptr->recvBytes = ptr->sendBytes = 0;
				}
			}
		}
		
		//FD_CLOSE 이벤트 처리
		if (NetworkEvents.lNetworkEvents & FD_CLOSE) {
			if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
				err_display(NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
			}
			RemoveSocketInfo(i);
		}
	}

	WSACleanup();
	return 0;
}

//소켓 정보 추가
BOOL AddSocketInfo(SOCKET sock) {
	SOCKETINFO* ptr = new SOCKETINFO;	//저장할 SOCKETINFO 구조체 동적 생성

	if (ptr == nullptr) {
		std::cout << "[오류] 메모리가 부족합니다." << "\n";
		return FALSE;
	}

	WSAEVENT hEvent = WSACreateEvent();		//소켓과 짝지을 이벤트 객체 생성
	if (hEvent == WSA_INVALID_EVENT) {
		err_display("WSACreateEvent()");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvBytes = 0;
	ptr->sendBytes = 0;
	SocketInfoArray[nTotalSockets] = ptr;	//저장할 소켓과 이벤트 객체를 인덱스가 같도록 저장하여 짝짓는다.
	EventArray[nTotalSockets] = hEvent;
	++nTotalSockets;

	return TRUE;
}

//소켓 정보 삭제
void RemoveSocketInfo(int nIndex) {
	SOCKETINFO* ptr = SocketInfoArray[nIndex];		//해당 인덱스 소켓을 가져온다.
	SOCKADDR_IN client_addr;

	int addrLen = sizeof(client_addr);
	getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
	std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";

	closesocket(ptr->sock);		//소켓을 닫는다.
	delete ptr;					//동적 생성한 SOCKETINFO 구조체를 메모리 해제한다.
	WSACloseEvent(EventArray[nIndex]);	//짝지어 생성한 이벤트 객체를 닫는다.

	if (nIndex != (nTotalSockets - 1)) {	//해당 인덱스가 마지막 원소가 아니라면 해당인덱스를 마지막 원소로 옮긴다.
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
		EventArray[nIndex] = EventArray[nTotalSockets - 1];
	}
	--nTotalSockets;	//구조체 개수를 감소시킨다.
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

void err_display(int errcode) {
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << "[오류] " << (char *)lpMsgBuf << "\n";
	LocalFree(lpMsgBuf);
}