#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const char* SERVERIP = "127.0.0.1";
const int SERVERPORT = 9000;
const int BUFSIZE = 512;

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
	server_addr.sin_port = htons(SERVERPORT);

	//데이터 통신에 사용할 변수
	SOCKADDR_IN peer_addr;
	int addrLen;
	char buf[BUFSIZE + 1];
	int len;

	//서버와 데이터 통신
	while (TRUE) {
		std::cout << "\n[보낼 데이터] : ";
		std::cin >> buf;
		len = strlen(buf);
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		}
		if (strlen(buf) == 0) {
			break;
		}

		//데이터 보내기
		retval = sendto(sock, buf, strlen(buf), 0, (SOCKADDR*)&server_addr, sizeof(server_addr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			continue;
		}
		std::cout << "[UDP 클라이언트] " << retval << "바이트를 보냈습니다." << "\n";

		//데이터 받기
		addrLen = sizeof(peer_addr);
		retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peer_addr, &addrLen);
		if (retval == SOCKET_ERROR) {
			err_display("recvFrom()");
			continue;
		}

		//송신자의 IP 주소 체크
		if (memcmp(&peer_addr, &server_addr, sizeof(peer_addr))) {
			std::cout << "[오류] 잘못된 데이터 입니다!" << "\n";
			continue;
		}

		//받은 데이터 출력
		buf[retval] = '\0';
		std::cout << "[UDP 클라이언트] " << retval << "바이트를 받았습니다." << "\n";
		std::cout << "[받은 데이터] " << buf << "\n";
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