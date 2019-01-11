#pragma once

#include "stdafx.h"
struct SOCKETINFO{
	OVERLAPPED overlapped;
	SOCKET sock;
	char buf[1025];
	int recv_bytes;
	int send_bytes;
	WSABUF wsa_buf;
};

class Server {
	friend void worker_thread(HANDLE icp);
public:
	void run(int);
	~Server();
	bool set_sock_opt(int, int, const char*, int);
private:
	bool init_winsock();
	bool create_sock();
	bool bind_sock(int);
	bool listen_sock();
	bool create_io_completion_port();
	void create_worker_thread();
	void accept_sock();

private:
	WSADATA wsa;
	SOCKET server_sock;
	SOCKADDR_IN server_addr;
	HANDLE icp;
};