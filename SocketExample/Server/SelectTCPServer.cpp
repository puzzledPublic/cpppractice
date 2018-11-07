#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {		//���� ���� ������ ���� ����ü
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvBytes;		//���� ����Ʈ ��
	int sendBytes;		//���� ����Ʈ ��
};

int nTotalSockets = 0;	//SOCKETINFO ����ü�� ����, ���� �����ø��� 1�� ����, ���������� 1�� ����
SOCKETINFO* SocketInfoArray[FD_SETSIZE];	//SOCKETINFO �����͸� ������ �迭, ���Ұ����� Select�𵨿��� ó���� �� �ִ� ������ �ִ밳��(FD_SETSIZE)�� ����

BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

void err_quit(const char* msg);
void err_display(const char* msg);

int main(int argc, char* argv[]) {
	int retval;
	//Winsock �ʱ�ȭ
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//listen socket ����
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
	//�ͺ��ŷ �������� ��ȯ, Select �𵨿����� ���ŷ���� �ͺ��ŷ�� �� �� ȿ����,
	//��, �ͺ��ŷ �������� ����Ҷ� send()�Լ� ȣ�� �� ������ ������ ���� ���� send()�Լ��� ���� ������ ���� �� ������ ����
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
		//�б� set, ���� set �ʱ�ȭ ��, ������(listen) ������ �б� set�� �ִ´�.
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		//SOCKETINFO�� �����Ͽ� ��� ������ �б� �Ǵ� ���� set�� �ִ´�. ���� �����Ͱ� ���� �����ͺ��� ������ ���� set��, �׷��������� �б� set�� �ִ´�.
		for (int i = 0; i < nTotalSockets; i++) {
			if (SocketInfoArray[i]->recvBytes > SocketInfoArray[i]->sendBytes) {
				FD_SET(SocketInfoArray[i]->sock, &wset);
			}
			else {
				FD_SET(SocketInfoArray[i]->sock, &rset);
			}
		}
		//select()�Լ� ȣ��(���� ������� �ʴ� ���� set��, timeout�� NULL)
		retval = select(0, &rset, &wset, NULL, NULL);
		if (retval == SOCKET_ERROR) {
			err_quit("select()");
		}
		//���� set �˻�(1): Ŭ���̾�Ʈ ���� ����
		//���� �б� set�� �˻��Ͽ� ������ Ŭ���̾�Ʈ�� �ִ��� Ȯ��, ������ ������ �б� set�� �ִٸ� ������ Ŭ���̾�Ʈ�� �ִٴ� ��.
		if (FD_ISSET(listen_sock, &rset)) {
			addrLen = sizeof(client_addr);
			client_sock = accept(listen_sock, (SOCKADDR*)&client_addr, &addrLen);	//�����쿡�� listen socket�� �ͺ��ŷ �����̸� accept�� ���ϵ� �ͺ��ŷ ����
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
			}
			else {
				std::cout << "\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";
				AddSocketInfo(client_sock);		//SOCKETINFO �߰�.
			}
		}
		//���� set �˻�(2): ������ ���
		//select() �Լ��� ������ �����ϴ� ���� ������ ���������� ��������� �˷����� �ʴ´�. ���� ��� ���Ͽ� ���� �ش� set�� �ִ��� Ȯ���ؾ��Ѵ�.
		for (int i = 0; i < nTotalSockets; i++) {
			SOCKETINFO *ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rset)) {	//������ �б� set�� ����ִٸ� recv()�Լ� ȣ�� �� ���� ���� Ȯ���Ͽ� ó��
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {	//���� ������ ����
					err_display("recv()");
					RemoveSocketInfo(i);	//SOCKETINFO ����
					continue;
				}
				else if (retval == 0) {	//���� ��������
					RemoveSocketInfo(i);	//SOCKETINFO ����
					continue;
				}
				ptr->recvBytes = retval;	//���� ����Ʈ �� ����
				addrLen = sizeof(client_addr);
				getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
				ptr->buf[retval] = '\0';
				std::cout << "[TCP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << ptr->buf << "\n";
			}
			if (FD_ISSET(ptr->sock, &wset)) {	//������ ���� set�� ����ִٸ� send()�Լ��� ȣ�� �� ���� ���� Ȯ���Ͽ� ó��
				retval = send(ptr->sock, ptr->buf + ptr->sendBytes, ptr->recvBytes - ptr->sendBytes, 0);
				if (retval == SOCKET_ERROR) {	//���� ������ ����
					err_display("send()");
					RemoveSocketInfo(i);		//SOCKETINFO ����
					continue;
				}
				ptr->sendBytes += retval;	//���� ����Ʈ �� ����
				if (ptr->recvBytes == ptr->sendBytes) {		//���� �����͸� ��� �������� ���� ����Ʈ ��, ���� ����Ʈ ���� 0���� �ʱ�ȭ
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
		std::cout << "[����] ���� ������ �߰��� �� �����ϴ�." << "\n";
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if (ptr == nullptr) {
		std::cout << "[����] �޸𸮰� �����մϴ�." << "\n";
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
	std::cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";

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