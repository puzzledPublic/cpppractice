#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;

	//윈속 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	//socket()	UDP이므로 SOCK_STREAM이 아닌 SOCK_DGRAM으로 설정한다.
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	//bind()
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVERPORT);
	retval = bind(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		err_quit("bind()");
	}

	//데이터 통신에 사용할 변수
	SOCKADDR_IN client_addr;
	int addrLen;
	char buf[BUFSIZE + 1];

	//클라이언트와 데이터 통신
	while (1) {
		//데이터 받기
		addrLen = sizeof(client_addr);
		retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&client_addr, &addrLen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			continue;
		}

		//받은 데이터 출력
		buf[retval] = '\0';
		std::cout << "[UDP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << buf << "\n";

		//데이터 보내기
		retval = sendto(sock, buf, retval, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			continue;
		}
	}
	
	//closesocket()
	closesocket(sock);

	//윈속 종료
	WSACleanup();
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