#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const char* SERVERIP = "127.0.0.1";
const int SERVERPORT = 9000;
const int BUFSIZE = 512;

int recvn(SOCKET s, char* buf, int len, int flags);

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {

		int retval;
		
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			return 1;
		}

		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) {
			err_quit("socket()");
		}

		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));	//memset(&server_addr, 0, sizeof(server_addr))
		server_addr.sin_family = AF_INET;				//인터넷 주소 체계를 사용한다는 의미로 AF_INET을 대입
		server_addr.sin_addr.s_addr = inet_addr(SERVERIP);	//서버의 경우 INADDR_ANY(0으로 정의되는 값)을 사용하는 것이 바람직(서버가 IP주소를 두 개 이상 보유한 경우에 클라이언트가 어느 IP로 접속하든 받아들일 수 있다.
		server_addr.sin_port = htons(SERVERPORT);
		retval = connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
		if (retval == SOCKET_ERROR) {
			err_quit("connect()");
		}

		char buf[BUFSIZE + 1];
		int len;

		while (TRUE) {
			std::cout << "\n[보낼 데이터] ";
			std::cin.getline(buf, BUFSIZE);
			
			std::cout << buf << "\n";

			len = strlen(buf);
			if (buf[len - 1] == '\n') {
				buf[len - 1] = '\0';
			}
			if (strlen(buf) == 0) {
				break;
			}

			retval = send(sock, buf, strlen(buf), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			std::cout << "[TCP 클라이언트] " << retval << "바이트를 보냈습니다." << "\n";

			retval = recvn(sock, buf, retval, 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0) {
				break;
			}

			buf[retval] = '\0';
			std::cout << "[TCP 클라이언트] " << retval << "바이트를 받았습니다." << "\n";
			std::cout << "[받은 데이터] " << buf << "\n";
		}

		closesocket(sock);

		WSACleanup();
		return 0;
}

int recvn(SOCKET s, char* buf, int len, int flags) {
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR) {
			return SOCKET_ERROR;
		}
		else if (received == 0) {
			break;
		}
		left -= received;
		ptr += received;
	}

	return (len - left);
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