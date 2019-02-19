#pragma once

#include "stdafx.h"

class ClientSession {
public:
	OVERLAPPED overlapped;
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[1024];
	int recv_bytes;
	int send_bytes;
	WSABUF wsa_buf;
};