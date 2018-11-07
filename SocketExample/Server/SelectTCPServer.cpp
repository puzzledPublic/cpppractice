#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {		//소켓 정보 저장을 위한 구조체
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvBytes;		//받은 바이트 수
	int sendBytes;		//보낸 바이트 수
};

int nTotalSockets = 0;	//SOCKETINFO 구조체의 개수, 소켓 생성시마다 1씩 증가, 닫을때마다 1씩 감소
SOCKETINFO* SocketInfoArray[FD_SETSIZE];	//SOCKETINFO 포인터를 저장할 배열, 원소개수는 Select모델에서 처리할 수 있는 소켓의 최대개수(FD_SETSIZE)로 정의

BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;
	//Winsock 초기화
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
	//넌블록킹 소켓으로 전환, Select 모델에서는 블로킹보다 넌블로킹이 좀 더 효율적,
	//단, 넌블로킹 소켓으로 사용할때 send()함수 호출 시 지정한 값보다 작은 값이 send()함수의 리턴 값으로 나올 수 있음을 주의
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR) {
		err_display("ioctlsocket()");
	}

	FD_SET rset, wset;
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int addrLen;

	while (TRUE) {
		//읽기 set, 쓰기 set 초기화 후, 연결대기(listen) 소켓을 읽기 set에 넣는다.
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		//SOCKETINFO를 참조하여 모든 소켓을 읽기 또는 쓰기 set에 넣는다. 받은 데이터가 보낸 데이터보다 많으면 쓰기 set에, 그렇지않으면 읽기 set에 넣는다.
		for (int i = 0; i < nTotalSockets; i++) {
			if (SocketInfoArray[i]->recvBytes > SocketInfoArray[i]->sendBytes) {
				FD_SET(SocketInfoArray[i]->sock, &wset);
			}
			else {
				FD_SET(SocketInfoArray[i]->sock, &rset);
			}
		}
		//select()함수 호출(지금 사용하지 않는 예외 set과, timeout은 NULL)
		retval = select(0, &rset, &wset, NULL, NULL);
		if (retval == SOCKET_ERROR) {
			err_quit("select()");
		}
		//소켓 set 검사(1): 클라이언트 접속 수용
		//먼저 읽기 set을 검사하여 접속한 클라이언트가 있는지 확인, 연결대기 소켓이 읽기 set에 있다면 접속한 클라이언트가 있다는 뜻.
		if (FD_ISSET(listen_sock, &rset)) {
			addrLen = sizeof(client_addr);
			client_sock = accept(listen_sock, (SOCKADDR*)&client_addr, &addrLen);	//윈도우에선 listen socket이 넌블록킹 소켓이면 accept한 소켓도 넌블로킹 소켓
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
			}
			else {
				std::cout << "\n[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
				AddSocketInfo(client_sock);		//SOCKETINFO 추가.
			}
		}
		//소켓 set 검사(2): 데이터 통신
		//select() 함수는 조건을 만족하는 소켓 개수를 리턴하지만 어떤소켓인지 알려주지 않는다. 따라서 모든 소켓에 대해 해당 set에 있는지 확인해야한다.
		for (int i = 0; i < nTotalSockets; i++) {
			SOCKETINFO *ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rset)) {	//소켓이 읽기 set에 들어있다면 recv()함수 호출 후 리턴 값을 확인하여 처리
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {	//소켓 비정상 종료
					err_display("recv()");
					RemoveSocketInfo(i);	//SOCKETINFO 제거
					continue;
				}
				else if (retval == 0) {	//소켓 정상종료
					RemoveSocketInfo(i);	//SOCKETINFO 제거
					continue;
				}
				ptr->recvBytes = retval;	//받은 바이트 수 갱신
				addrLen = sizeof(client_addr);
				getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
				ptr->buf[retval] = '\0';
				std::cout << "[TCP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << ptr->buf << "\n";
			}
			if (FD_ISSET(ptr->sock, &wset)) {	//소켓이 쓰기 set에 들어있다면 send()함수를 호출 후 리턴 값을 확인하여 처리
				retval = send(ptr->sock, ptr->buf + ptr->sendBytes, ptr->recvBytes - ptr->sendBytes, 0);
				if (retval == SOCKET_ERROR) {	//소켓 비정상 종료
					err_display("send()");
					RemoveSocketInfo(i);		//SOCKETINFO 제거
					continue;
				}
				ptr->sendBytes += retval;	//보낸 바이트 수 갱신
				if (ptr->recvBytes == ptr->sendBytes) {		//받은 데이터를 모두 보냈으면 받은 바이트 수, 보낸 바이트 수를 0으로 초기화
					ptr->recvBytes = ptr->sendBytes = 0;
				}
			}
		}
	}
	WSACleanup();
	return 0;
}

BOOL AddSocketInfo(SOCKET sock) {
	if (nTotalSockets >= FD_SETSIZE) {
		std::cout << "[오류] 소켓 정보를 추가할 수 없습니다." << "\n";
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if (ptr == nullptr) {
		std::cout << "[오류] 메모리가 부족합니다." << "\n";
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvBytes = 0;
	ptr->sendBytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr;
	return TRUE;
}

void RemoveSocketInfo(int nIndex) {
	SOCKETINFO* ptr = SocketInfoArray[nIndex];

	SOCKADDR_IN client_addr;
	int addrLen = sizeof(client_addr);
	getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
	std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";

	closesocket(ptr->sock);
	delete ptr;

	if (nIndex != (nTotalSockets - 1)) {
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
	}

	--nTotalSockets;
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