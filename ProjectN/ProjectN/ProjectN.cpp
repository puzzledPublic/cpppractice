// ProjectN.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include "Server.h"

int main()
{
	Server server;
	server.run(9000);
    return 0;
}

