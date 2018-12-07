
#include "stdafx.h"
#include "network.h"

void Network::startServer()
{
	initWinSock();
	createSocket();
}

Network::~Network()
{
	WSACleanup();
}

void Network::initWinSock()
{
	int retval = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (retval != 0) {
		exit(0);
	}
}

void Network::createSocket()
{
	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET) {
		std::cout << "Error : socket()" << std::endl;
		return;
	}

	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(9000);
	server_addr.sin_addr.s_addr = ADDR_ANY;
	int retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		std::cout << "Error : bind()" << std::endl;
		return;
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		std::cout << "Error : listen()" << std::endl;
		return;
	}

	while (true) {
		client_addrLen = sizeof(client_addr);
		client_sock = accept(listen_sock, (SOCKADDR*)&client_addr, &client_addrLen);

		std::thread t1([](SOCKET* client_sock, SOCKADDR_IN* client_addr) {
			char buf[513];
			int retval;
			while (true) {
				retval = recv(*client_sock, buf, 512, 0);
				if (retval == SOCKET_ERROR) {
					std::cout << "Error : recv()" << std::endl;
					break;
				}
				else if (retval == 0) {
					break;
				}

				buf[retval] = '\0';
				char addrBuf[50];
				DWORD addrBufLen = sizeof(addrBuf);
				WSAAddressToString((SOCKADDR*)client_addr, sizeof(*client_addr), NULL, addrBuf, &addrBufLen);
				std::cout << addrBuf << " | " << ntohs(client_addr->sin_port) << " : " << buf << std::endl;
				retval = send(*client_sock, buf, 512, 0);
				if (retval == SOCKET_ERROR) {
					std::cout << "Error : send()" << std::endl;
					break;
				}
			}
			closesocket(*client_sock);
		}, &client_sock, &client_addr);

		t1.join();
	}
	
	closesocket(listen_sock);
}
