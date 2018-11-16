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
		server_addr.sin_family = AF_INET;				//���ͳ� �ּ� ü�踦 ����Ѵٴ� �ǹ̷� AF_INET�� ����
		server_addr.sin_addr.s_addr = inet_addr(SERVERIP);	//������ ��� INADDR_ANY(0���� ���ǵǴ� ��)�� ����ϴ� ���� �ٶ���(������ IP�ּҸ� �� �� �̻� ������ ��쿡 Ŭ���̾�Ʈ�� ��� IP�� �����ϵ� �޾Ƶ��� �� �ִ�.
		server_addr.sin_port = htons(SERVERPORT);
		retval = connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
		if (retval == SOCKET_ERROR) {
			err_quit("connect()");
		}

		char buf[BUFSIZE + 1];
		int len;

		while (TRUE) {
			std::cout << "\n[���� ������] ";
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

			std::cout << "[TCP Ŭ���̾�Ʈ] " << retval << "����Ʈ�� ���½��ϴ�." << "\n";

			retval = recvn(sock, buf, retval, 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0) {
				break;
			}

			buf[retval] = '\0';
			std::cout << "[TCP Ŭ���̾�Ʈ] " << retval << "����Ʈ�� �޾ҽ��ϴ�." << "\n";
			std::cout << "[���� ������] " << buf << "\n";
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