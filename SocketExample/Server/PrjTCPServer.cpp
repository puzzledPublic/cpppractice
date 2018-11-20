#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 256;

struct SOCKETINFO {		//���� ���� ������ ���� ����ü
	SOCKET sock;		//Ŭ���̾�Ʈ ����
	bool isIPv6;		//IPv6 �������� ����
	char buf[BUFSIZE];	//���� �� ���� ���α׷� �ۼ��� ����
	int recvBytes;		//���� ����Ʈ ��
};

int nTotalSockets = 0;	//���� ����� ���� ����
SOCKETINFO* SocketInfoArray[FD_SETSIZE];	//����� ���� �������� ���� �迭

BOOL AddSocketInfo(SOCKET sock, bool isIPv6);	//���� ���� ������ ���� �Լ�
void RemoveSocketInfo(int nIndex);				//���� ���� ������ ���� �Լ�

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;
	//���� �ʱ�ȭ
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//IPv4�� ���� listen_sock ����
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
	//IPv6�� ���� listen_sock ����
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
	//������ ��ſ� ����� ���� ����
	FD_SET rSet;
	SOCKET client_sock;
	int addrLen;
	SOCKADDR_IN client_addrv4;
	SOCKADDR_IN6 client_addrv6;

	while (TRUE) {
		//���� set �ʱ�ȭ
		FD_ZERO(&rSet);
		//�б� set�� listen_sockv4, listen_sockv6�� ���� ����� ���ϵ��� �ִ´�.
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
		//���� set �˻�(1) : Ŭ���̾�Ʈ ���� ����
		if (FD_ISSET(listen_sockv4, &rSet)) {	//listen_sockv4�� �б� ������ ��� -> accept()
			addrLen = sizeof(client_addrv4);
			client_sock = accept(listen_sockv4, (SOCKADDR*)&client_addrv4, &addrLen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
			else {
				std::cout << "[TCPv4 ����] Ŭ���̾�Ʈ ����: [" << inet_ntoa(client_addrv4.sin_addr) << "]:" << ntohs(client_addrv4.sin_port) << "\n";
				AddSocketInfo(client_sock, false);
			}
		}

		if (FD_ISSET(listen_sockv6, &rSet)) {	//listen_sockv6�� �б� ������ ��� -> accept()
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
				std::cout << "[TCPv6 ����] Ŭ���̾�Ʈ ����: " << ipAddr << "\n";
				AddSocketInfo(client_sock, true);
			}
		}
		//���� set �˻�(2) : ������ ���
		//����� ���ϵ��� ��ȸ�ϸ� �б� �������� �˻� -> recv()
		for (int i = 0; i < nTotalSockets; i++) {
			SOCKETINFO* ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rSet)) {
				//recv()
				retval = recv(ptr->sock, ptr->buf + ptr->recvBytes, BUFSIZE - ptr->recvBytes, 0);
				if (retval == 0 || retval == SOCKET_ERROR) {
					RemoveSocketInfo(i);
					continue;
				}

				ptr->recvBytes += retval;	//���� ����Ʈ �� ����

				//recv() �Լ� ȣ�� �� ������ 256����Ʈ ���� ���� ��Ŷ�� �д´ٴ� ������ ����.
				//���� �Ź� ���� ����Ʈ ���� �����ߴٰ� 256 ����Ʈ ũ�⿡ �������� ���� �����͸� �������� �Ѵ�.
				if (ptr->recvBytes == BUFSIZE) {	//BUFSIZE��ŭ �޾Ҵٸ� ����� ��� ���Ͽ� �����͸� ������.
					ptr->recvBytes = 0;

					for (int j = 0; j < nTotalSockets; j++) {
						SOCKETINFO* ptr2 = SocketInfoArray[j];
						retval = send(ptr2->sock, ptr->buf, BUFSIZE, 0);
						if (retval == SOCKET_ERROR) {
							err_display("send()");
							RemoveSocketInfo(j);
							--j;	//���� �ε��� ���� (RemoveSocketInfo() �Լ����� ���� �ڸ��� ������ ����� ������ ���Ƿ�)
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
		std::cout << "[����] ���� ������ �� �߰��� �� �����ϴ�." << "\n";
		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == nullptr) {
		std::cout << "[����] �޸𸮰� �����մϴ�." << "\n";
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
		std::cout << "[TCPv4 ����] Ŭ���̾�Ʈ ����: [" << inet_ntoa(client_addrv4.sin_addr) << "]:" << ntohs(client_addrv4.sin_port) << "\n";
	}
	else {
		SOCKADDR_IN6 client_addrv6;
		int addrLen = sizeof(client_addrv6);
		getpeername(ptr->sock, (SOCKADDR*)&client_addrv6, &addrLen);
		char ipAddr[50];
		DWORD ipAddrLen = sizeof(ipAddr);
		WSAAddressToString((SOCKADDR*)&client_addrv6, sizeof(client_addrv6), NULL, ipAddr, &ipAddrLen);
		std::cout << "[TCPv6 ����] Ŭ���̾�Ʈ ����: " << ipAddr << "\n";
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