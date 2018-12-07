#pragma once

class Network {
public:
	void startServer();
	~Network();
private:
	void initWinSock();
	void createSocket();

	WSADATA wsaData;
	SOCKET listen_sock, client_sock;
	int client_addrLen;
	SOCKADDR_IN server_addr, client_addr;
};