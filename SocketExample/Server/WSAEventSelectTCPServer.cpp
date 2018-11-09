#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {			//���� ���� ������ ���� ����ü
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvBytes;
	int sendBytes;
};

int nTotalSockets = 0;		//SOCKETINFO ����ü�� ����, ���� �������� 1�� ����, ���� ���� ������ 1�� ����
SOCKETINFO* SocketInfoArray[WSA_MAXIMUM_WAIT_EVENTS];	//SOCKETINFO ����ü �����͸� ������ �迭
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];			//���ϰ� ¦���� �̺�Ʈ ��ü �ڵ��� ������ �迭

//���� ���� �Լ�
BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

//���� ��� �Լ�
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);

int main(int argc, char* argv[]) {
	int retval;

	//���� �ʱ�ȭ
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

	//���� ���� �߰� & WSAEventSelect()
	AddSocketInfo(listen_sock);
	//���� ��� ���ϰ� �̺�Ʈ ��ü�� ¦���´�. ���� ��� ������ FD_ACCEPT�� FD_CLOSE �� ���� ��Ʈ��ũ �̺�Ʈ�� ó���ϸ� �ȴ�.
	retval = WSAEventSelect(listen_sock, EventArray[nTotalSockets - 1], FD_ACCEPT | FD_CLOSE);
	if (retval == SOCKET_ERROR) {
		err_quit("WSAEventSelect()");
	}

	//������ ��ſ� ����� ����
	WSANETWORKEVENTS NetworkEvents;
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int i, addrLen;

	while (TRUE) {
		//�̺�Ʈ ��ü �����ϱ�
		i = WSAWaitForMultipleEvents(nTotalSockets, EventArray, FALSE, WSA_INFINITE, FALSE);	//�̺�Ʈ ��ü�� ��ȣ ���°� �� ������ ���
		if (i == WSA_WAIT_FAILED) {
			continue;
		}
		//WSAWaitForMultipleEvents() �Լ��� ���� ���� ��ȣ ���°� �� �̺�Ʈ ��ü�� �迭 �ε��� + WSA_WAIT_EVENT_0 ���̴�.
		//���� ���� �ε��� ���� �������� WSA_WAIT_EVENTS_0���� �����Ѵ�.
		i -= WSA_WAIT_EVENT_0;

		//��ü���� ��Ʈ��ũ �̺�Ʈ �˾Ƴ���
		retval = WSAEnumNetworkEvents(SocketInfoArray[i]->sock, EventArray[i], &NetworkEvents);
		if (retval == SOCKET_ERROR) {
			continue;
		}

		//FD_ACCEPT �̺�Ʈ ó��
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
			std::cout << "\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";
			if (nTotalSockets >= WSA_MAXIMUM_WAIT_EVENTS) {		//���� ����� ���� ������ WSA_MAXIMUM_WAIT_EVENTS(64)�� �Ѿ�� �� ������ ���� �� ����.
				std::cout << "[����] �� �̻� ������ �޾Ƶ��� �� �����ϴ�." << "\n";
				closesocket(client_sock);
				continue;
			}
			//���� ���� �߰� & WSAEventSelect();
			AddSocketInfo(client_sock);
			retval = WSAEventSelect(client_sock, EventArray[nTotalSockets - 1], FD_READ | FD_WRITE | FD_CLOSE);		//Ŭ���̾�Ʈ ���Ͽ� FD_READ, FD_WRITE, FD_CLOSE �̺�Ʈ�� ����Ѵ�.
			if (retval == SOCKET_ERROR) {
				err_quit("WSAEventSelect()");
			}
		}

		//FD_READ & FD_WRITE �̺�Ʈ ó��
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
			if (ptr->recvBytes == 0) {			//���� ����Ʈ ���� 0�� ��쿡�� recv()�Լ��� ȣ���� �����͸� �д´�.
				//������ �ޱ�
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

			if (ptr->recvBytes > ptr->sendBytes) {		//���� ����Ʈ ���� ���� ����Ʈ ������ ũ�ٸ� send() �Լ��� ȣ���Ͽ� �����͸� ������.
				//������ ������
				retval = send(ptr->sock, ptr->buf + ptr->sendBytes, ptr->recvBytes - ptr->sendBytes, 0);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						err_display("send()");
						RemoveSocketInfo(i);
					}
					continue;
				}
				ptr->sendBytes += retval;
				//���� �����͸� ��� ���´��� üũ
				if (ptr->recvBytes == ptr->sendBytes) {		//���� ��ŭ ��� ���´ٸ� ���� ����Ʈ ���� ���� ����Ʈ ���� �ٽ� 0���� �ʱ�ȭ
					ptr->recvBytes = ptr->sendBytes = 0;
				}
			}
		}
		
		//FD_CLOSE �̺�Ʈ ó��
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

//���� ���� �߰�
BOOL AddSocketInfo(SOCKET sock) {
	SOCKETINFO* ptr = new SOCKETINFO;	//������ SOCKETINFO ����ü ���� ����

	if (ptr == nullptr) {
		std::cout << "[����] �޸𸮰� �����մϴ�." << "\n";
		return FALSE;
	}

	WSAEVENT hEvent = WSACreateEvent();		//���ϰ� ¦���� �̺�Ʈ ��ü ����
	if (hEvent == WSA_INVALID_EVENT) {
		err_display("WSACreateEvent()");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvBytes = 0;
	ptr->sendBytes = 0;
	SocketInfoArray[nTotalSockets] = ptr;	//������ ���ϰ� �̺�Ʈ ��ü�� �ε����� ������ �����Ͽ� ¦���´�.
	EventArray[nTotalSockets] = hEvent;
	++nTotalSockets;

	return TRUE;
}

//���� ���� ����
void RemoveSocketInfo(int nIndex) {
	SOCKETINFO* ptr = SocketInfoArray[nIndex];		//�ش� �ε��� ������ �����´�.
	SOCKADDR_IN client_addr;

	int addrLen = sizeof(client_addr);
	getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);
	std::cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ=" << ntohs(client_addr.sin_port) << "\n";

	closesocket(ptr->sock);		//������ �ݴ´�.
	delete ptr;					//���� ������ SOCKETINFO ����ü�� �޸� �����Ѵ�.
	WSACloseEvent(EventArray[nIndex]);	//¦���� ������ �̺�Ʈ ��ü�� �ݴ´�.

	if (nIndex != (nTotalSockets - 1)) {	//�ش� �ε����� ������ ���Ұ� �ƴ϶�� �ش��ε����� ������ ���ҷ� �ű��.
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
		EventArray[nIndex] = EventArray[nTotalSockets - 1];
	}
	--nTotalSockets;	//����ü ������ ���ҽ�Ų��.
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
	std::cout << "[����] " << (char *)lpMsgBuf << "\n";
	LocalFree(lpMsgBuf);
}