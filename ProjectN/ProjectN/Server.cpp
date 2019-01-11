#include "stdafx.h"
#include "Server.h"

void Server::run(int server_port)
{
	init_winsock();
	create_sock();
	bind_sock(server_port);
	listen_sock();
	create_io_completion_port();
	create_worker_thread();
	accept_sock();
}

Server::~Server()
{
	WSACleanup();
}

bool Server::init_winsock()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return false;
	}
	return true;
}

bool Server::create_sock()
{
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == INVALID_SOCKET) {
		return false;
	}
	return true;
}

bool Server::bind_sock(int server_port)
{
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);
	int retval = bind(server_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		return false;
	}
	return true;
}

bool Server::listen_sock()
{
	int retval = listen(server_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		return false;
	}
	return true;;
}

bool Server::create_io_completion_port()
{
	icp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (icp == nullptr) {
		return false;
	}
	return true;
}

void Server::create_worker_thread()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++) {
		std::thread t(worker_thread, icp);
		t.detach();
	}
}

void Server::accept_sock()
{
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int client_addr_len;
	DWORD recv_bytes, flags;

	for (;;) {
		client_addr_len = sizeof(client_addr);
		client_sock = accept(server_sock, (SOCKADDR*)&client_addr, &client_addr_len);
		if (client_sock == INVALID_SOCKET) {
			std::cout << "accept : invalid socket error" << std::endl;
			break;
		}
		std::cout << "\n[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";

		CreateIoCompletionPort((HANDLE)client_sock, icp, client_sock, 0);

		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == nullptr) {
			break;
		}
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recv_bytes = ptr->send_bytes = 0;
		ptr->wsa_buf.buf = ptr->buf;
		ptr->wsa_buf.len = 1025;
		
		flags = 0;
		int retval = WSARecv(client_sock, &ptr->wsa_buf, 1, &recv_bytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				std::cout << "WSARecv()" << std::endl;
			}
			continue;
		}
	}
}

void worker_thread(HANDLE icp)
{
	int retval;
	
	while (TRUE) {
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;

		retval = GetQueuedCompletionStatus(icp, &cbTransferred, (LPDWORD)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

		SOCKADDR_IN client_addr;
		int addrLen = sizeof(client_addr);
		getpeername(ptr->sock, (SOCKADDR*)&client_addr, &addrLen);

		if (retval == 0 || cbTransferred == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &temp1, FALSE, &temp2);
				std::cout << "WSAGetOverlappedResult()" << std::endl;
			}
			closesocket(ptr->sock);
			std::cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(client_addr.sin_addr) << ", 포트 번호=" << ntohs(client_addr.sin_port) << "\n";
			delete ptr;
			continue;
		}

		if (ptr->recv_bytes == 0) {
			ptr->recv_bytes = cbTransferred;
			ptr->send_bytes = 0;
			ptr->buf[ptr->recv_bytes] = '\0';
			std::cout << "[TCP/" << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "] " << ptr->buf << "\n";
		}
		else {
			ptr->send_bytes += cbTransferred;
		}
		if (ptr->recv_bytes > ptr->send_bytes) {
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsa_buf.buf = ptr->buf + ptr->send_bytes;
			ptr->wsa_buf.len = ptr->recv_bytes - ptr->send_bytes;

			DWORD sendBytes;
			retval = WSASend(ptr->sock, &ptr->wsa_buf, 1, &sendBytes, 0, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					std::cout << "WSASend()" << std::endl;
				}
				continue;
			}
		}
		else {
			ptr->recv_bytes = 0;

			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsa_buf.buf = ptr->buf;
			ptr->wsa_buf.len = 1025;

			DWORD recvBytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->sock, &ptr->wsa_buf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					std::cout <<  "WSARecv()" << std::endl;
				}
				continue;
			}
		}
	}
}
