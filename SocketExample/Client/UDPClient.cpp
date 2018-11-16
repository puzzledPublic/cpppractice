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

	//������ ��ſ� ����� ����
	SOCKADDR_IN peer_addr;
	int addrLen;
	char buf[BUFSIZE + 1];
	int len;

	//������ ������ ���
	while (TRUE) {
		std::cout << "\n[���� ������] : ";
		std::cin >> buf;
		len = strlen(buf);
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		}
		if (strlen(buf) == 0) {
			break;
		}

		//������ ������
		retval = sendto(sock, buf, strlen(buf), 0, (SOCKADDR*)&server_addr, sizeof(server_addr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			continue;
		}
		std::cout << "[UDP Ŭ���̾�Ʈ] " << retval << "����Ʈ�� ���½��ϴ�." << "\n";

		//������ �ޱ�
		addrLen = sizeof(peer_addr);
		retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peer_addr, &addrLen);
		if (retval == SOCKET_ERROR) {
			err_display("recvFrom()");
			continue;
		}

		//�۽����� IP �ּ� üũ
		if (memcmp(&peer_addr, &server_addr, sizeof(peer_addr))) {
			std::cout << "[����] �߸��� ������ �Դϴ�!" << "\n";
			continue;
		}

		//���� ������ ���
		buf[retval] = '\0';
		std::cout << "[UDP Ŭ���̾�Ʈ] " << retval << "����Ʈ�� �޾ҽ��ϴ�." << "\n";
		std::cout << "[���� ������] " << buf << "\n";
	}
	//closesocket()
	closesocket(sock);

	//���� ����
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