#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 256;

struct SOCKETINFO {		//소켓 정보 저장을 위한 구조체
	SOCKET sock;		//클라이언트 소켓
	bool isIPv6;		//IPv6 소켓인지 여부
	char buf[BUFSIZE];	//소켓 별 응용 프로그램 송수신 버퍼
	int recvBytes;		//받은 바이트 수
};

int nTotalSockets = 0;	//현재 연결된 소켓 개수
SOCKETINFO* SocketInfoArray[FD_SETSIZE];	//연결된 소켓 정보들을 담을 배열

BOOL AddSocketInfo(SOCKET sock, bool isIPv6);	//소켓 정보 생성을 위한 함수
void RemoveSocketInfo(int nIndex);				//소켓 정보 삭제를 위한 함수

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;
	//윈속 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//IPv4를 위한 listen_sock 생성
	SOCKET listen_sockv4 = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sockv4 == INVALID_SOCKET) {
		err_quit("socket()");
	}
	//IPv4 bind()
	SOCKADDR_IN server_addrv4;
	ZeroMemory(&server_addrv4, sizeof(server_addrv4));
	server_addrv4.sin_family = AF_INET;
	server_addrv4.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addrv4.sin_port = htons(SERVERPORT);
	retval = bind(listen_sockv4, (SOCKADDR*)&server_addrv4, sizeof(server_addrv4));
	if (retval == SOCKET_ERROR) {
		err_quit("bind()");
	}
	//IPv4 listen()
	retval = listen(listen_sockv4, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		err_quit("listen()");
	}
	//IPv6를 위한 listen_sock 생성
	SOCKET listen_sockv6 = socket(AF_INET6, SOCK_STREAM, 0);
	if (listen_sockv6 == INVALID_SOCKET) {
		err_quit("socket()");
	}
	//IPv6 bind()
	SOCKADDR_IN6 server_addrv6;
	ZeroMemory(&server_addrv6, sizeof(server_addrv6));
	server_addrv6.sin6_family = AF_INET6;
	server_addrv6.sin6_addr = in6addr_any;
	server_addrv6.sin6_port = htons(SERVERPORT);
	retval = bind(listen_sockv6, (SOCKADDR*)&server_addrv6, sizeof(server_addrv6));
	if (retval == SOCKET_ERROR) {
		err_quit("bind()");
	}
	//IPv6 listen()
	retval = listen(listen_sockv6, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		err_quit("listen()");
	}
	//데이터 통신에 사용할 공통 변수
	FD_SET rSet;
	SOCKET client_sock;
	int addrLen;
	SOCKADDR_IN client_addrv4;
	SOCKADDR_IN6 client_addrv6;

	while (TRUE) {
		//소켓 set 초기화
		FD_ZERO(&rSet);
		//읽기 set에 listen_sockv4, listen_sockv6와 현재 연결된 소켓들을 넣는다.
		FD_SET(listen_sockv4, &rSet);
		FD_SET(listen_sockv6, &rSet);
		for (int i = 0; i < nTotalSockets; i++) {
			FD_SET(SocketInfoArray[i]->sock, &rSet);
		}
		//select()
		retval = select(0, &rSet, NULL, NULL, NULL);
		if (retval == SOCKET_ERROR) {
			err_display("select()");
			break;
		}
		//소켓 set 검사(1) : 클라이언트 접속 수용
		if (FD_ISSET(listen_sockv4, &rSet)) {	//listen_sockv4가 읽기 가능한 경우 -> accept()
			addrLen = sizeof(client_addrv4);
			client_sock = accept(listen_sockv4, (SOCKADDR*)&client_addrv4, &addrLen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
			else {
				std::cout << "[TCPv4 서버] 클라이언트 접속: [" << inet_ntoa(client_addrv4.sin_addr) << "]:" << ntohs(client_addrv4.sin_port) << "\n";
				AddSocketInfo(client_sock, false);
			}
		}

		if (FD_ISSET(listen_sockv6, &rSet)) {	//listen_sockv6가 읽기 가능한 경우 -> accept()
			addrLen = sizeof(client_addrv6);
			client_sock = accept(listen_sockv6, (SOCKADDR*)&client_addrv6, &addrLen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
			else {
				char ipAddr[50];
				DWORD ipAddrLen = sizeof(ipAddr);
				WSAAddressToString((SOCKADDR*)&client_addrv6, sizeof(client_addrv6), NULL, ipAddr, &ipAddrLen);
				std::cout << "[TCPv6 서버] 클라이언트 접속: " << ipAddr << "\n";
				AddSocketInfo(client_sock, true);
			}
		}
		//소켓 set 검사(2) : 데이터 통신
		//연결된 소켓들을 순회하며 읽기 가능한지 검사 -> recv()
		for (int i = 0; i < nTotalSockets; i++) {
			SOCKETINFO* ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rSet)) {
				//recv()
				retval = recv(ptr->sock, ptr->buf + ptr->recvBytes, BUFSIZE - ptr->recvBytes, 0);
				if (retval == 0 || retval == SOCKET_ERROR) {
					RemoveSocketInfo(i);
					continue;
				}

				ptr->recvBytes += retval;	//받은 바이트 수 누적

				//recv() 함수 호출 한 번으로 256바이트 고정 길이 패킷을 읽는다는 보장이 없다.
				//따라서 매번 받은 바이트 수를 누적했다가 256 바이트 크기에 도달했을 때만 데이터를 보내도록 한다.
				if (ptr->recvBytes == BUFSIZE) {	//BUFSIZE만큼 받았다면 연결된 모든 소켓에 데이터를 보낸다.
					ptr->recvBytes = 0;

					for (int j = 0; j < nTotalSockets; j++) {
						SOCKETINFO* ptr2 = SocketInfoArray[j];
						retval = send(ptr2->sock, ptr->buf, BUFSIZE, 0);
						if (retval == SOCKET_ERROR) {
							err_display("send()");
							RemoveSocketInfo(j);
							--j;	//루프 인덱스 보정 (RemoveSocketInfo() 함수에서 현재 자리에 마지막 연결된 소켓이 오므로)
							continue;
						}
					}
				}
			}
		}
	}
	return 0;
}

BOOL AddSocketInfo(SOCKET sock, bool isIPv6) {
	if (nTotalSockets >= FD_SETSIZE) {
		std::cout << "[오류] 소켓 정보를 더 추가할 수 없습니다." << "\n";
		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == nullptr) {
		std::cout << "[오류] 메모리가 부족합니다." << "\n";
		return FALSE;
	}

	ptr->sock = sock;
	ptr->isIPv6 = isIPv6;
	ptr->recvBytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr;

	return TRUE;
}

void RemoveSocketInfo(int nIndex) {
	SOCKETINFO* ptr = SocketInfoArray[nIndex];

	if (ptr->isIPv6 == false) {
		SOCKADDR_IN client_addrv4;
		int addrLen = sizeof(client_addrv4);
		getpeername(ptr->sock, (SOCKADDR*)&client_addrv4, &addrLen);
		std::cout << "[TCPv4 서버] 클라이언트 종료: [" << inet_ntoa(client_addrv4.sin_addr) << "]:" << ntohs(client_addrv4.sin_port) << "\n";
	}
	else {
		SOCKADDR_IN6 client_addrv6;
		int addrLen = sizeof(client_addrv6);
		getpeername(ptr->sock, (SOCKADDR*)&client_addrv6, &addrLen);
		char ipAddr[50];
		DWORD ipAddrLen = sizeof(ipAddr);
		WSAAddressToString((SOCKADDR*)&client_addrv6, sizeof(client_addrv6), NULL, ipAddr, &ipAddrLen);
		std::cout << "[TCPv6 서버] 클라이언트 종료: " << ipAddr << "\n";
	}

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