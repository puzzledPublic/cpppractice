#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

#define WM_SOCKET (WM_USER + 1)

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {				//소켓 정보 저장을 위한 구조체
	SOCKET sock;				//클라이언트 소켓
	char buf[BUFSIZE + 1];		//해당 소켓의 버퍼
	int recvBytes;				//받은 바이트 수
	int sendBytes;				//보낸 바이트 수
	BOOL recvDelayed;			//FD_READ 메시지를 받았지만 대응 함수 recv()함수를 호출하지 않은 경우 TRUE로 설정한다.
	SOCKETINFO* next;			//SOCKETINFO를 단일 연결 리스트로 관리하기 위한 다음 SOCKETINFO 포인터
};

SOCKETINFO* socketInfoList;		//SOCKETINFO 단일 연결 리스트의 헤더(시작점)

HWND hEdit;
HINSTANCE hInst;

//윈도우 메시지 처리 함수
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);

//소켓관리함수
BOOL AddSocketInfo(SOCKET sock);
SOCKETINFO* GetSocketInfo(SOCKET sock);
void RemoveSocketInfo(SOCKET sock);

//오류출력함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);
void DisplayText(char *fmt, ...);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	hInst = hInstance;

	//윈도우 클래스 등록
	WNDCLASS wndClass;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = "MyWndClass";
	if (!RegisterClass(&wndClass)) return 1;

	//윈도우 생성
	HWND hWnd = CreateWindow("MyWndClass", "TCP 서버", WS_OVERLAPPEDWINDOW, 0, 0, 600, 200, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//Winsock 초기화
	int retval;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//listen socket 생성
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

	//WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);	//연결 대기 소켓은 송수신 목적은 아니므로 FD_ACCEPT, FD_CLOSE 이벤트만 등록
	if (retval == SOCKET_ERROR) {
		err_quit("WSAAsyncSelect()");
	}

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		hEdit = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY, 0, 0, 0, 0, hWnd, (HMENU)100, hInst, NULL);
		DisplayText("nothing");
		return 0;
	case WM_SIZE:
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	case WM_SETFOCUS:
		SetFocus(hEdit);
		return 0;
	case WM_SOCKET:		//소켓 관련 윈도우 메시지
		ProcessSocketMessage(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//데이터 통신에 사용할 변수
	SOCKETINFO* ptr;
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int addrLen, retval;

	//오류 발생 여부 확인(lParam의 상위16비트)
	if (WSAGETSELECTERROR(lParam)) {
		err_display(WSAGETSELECTERROR(lParam));
		RemoveSocketInfo(wParam);
		return;
	}
	
	//네트워크 이벤트 별 메시지 처리(lParam의 하위16비트)
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_ACCEPT:		//accept()
		addrLen = sizeof(client_addr);
		client_sock = accept(wParam, (SOCKADDR*)&client_addr, &addrLen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			return;
		}
		DisplayText("\r\n[TCP 서버] 클라이언트 접속: IP 주소 = %s, 포트 번호 = %d\r\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		AddSocketInfo(client_sock);
		//클라이언트 소켓에 대해 WSAAsyncSelect()함수를 호출함으로써 관심 있는 이벤트를 다시 등록한다.
		//데이터 전송에 사용할 것이므로 FD_READ, FD_WRITE_ FD_CLOSE 이벤트를 등록한다.
		retval = WSAAsyncSelect(client_sock, hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);	
		if (retval == SOCKET_ERROR) {
			err_display("WSAAsyncSelect()");
			RemoveSocketInfo(client_sock);
		}
		break;
	case FD_READ:		//recv()
		ptr = GetSocketInfo(wParam);
		//이전에 받았으나 아직 보내지 않은 데이터가 있다면 recv()를 호출하지 않고 바로 리턴한다.
		//단, recvDelayed 변수를 TRUE로 설정하여 이 사실을 기록한다. 대응 함수를 호출하지 않았으므로 받은 데이터가 있더라도 FD_READ이벤트는 다시 발생하지 않는다.
		//따라서 나중에 PostMessage() API 함수를 사용하여 응용 프로그램이 직접 FD_READ 이벤트를 발생시켜야 한다.
		if (ptr->recvBytes > 0) {
			ptr->recvDelayed = TRUE;
			return;
		}
		retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			RemoveSocketInfo(wParam);
			return;
		}

		ptr->recvBytes = retval;
		ptr->buf[retval] = '\0';
		addrLen = sizeof(client_addr);
		getpeername(wParam, (SOCKADDR*)&client_addr, &addrLen);
		DisplayText("[TCP%s:%d] %s\r\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ptr->buf);
		//FD_READ 이벤트 처리 후 바로 FD_WRITE 이벤트도 처리한다.
	case FD_WRITE:
		ptr = GetSocketInfo(wParam);
		if (ptr->recvBytes <= ptr->sendBytes) {		//받은 바이트 수가 보낸 바이트 수보다 크지 않으면 보낼 데이터가 없으므로 바로 리턴
			return;
		}
		retval = send(ptr->sock, ptr->buf + ptr->sendBytes, ptr->recvBytes - ptr->sendBytes, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			RemoveSocketInfo(wParam);
			return;
		}
		ptr->sendBytes += retval;
		if (ptr->recvBytes == ptr->sendBytes) {		//받은 데이터를 모두 보냈다면 받은 바이트 수와 보낸 바이트 수를 0 으로 초기화
			ptr->recvBytes = ptr->sendBytes = 0;
			//이전 FD_READ 이벤트에 대한 대응 함수를 호출하지 않았다면 PostMessage() API 함수를 호출해 강제로 발생시킨다.
			//다음 번에 FD_READ 이벤트가 발생하므로 이전에 도착했으나 처리하지 못한 데이터를 읽을 수 있다.
			if (ptr->recvDelayed) {
				ptr->recvDelayed = FALSE;
				PostMessage(hWnd, WM_SOCKET, wParam, FD_READ);
			}
		}
		break;
	case FD_CLOSE:		//정상 종료를 의미하므로 소켓을 닫고 소켓 정보를 제거한다.
		RemoveSocketInfo(wParam);
		break;
	}
}

BOOL AddSocketInfo(SOCKET sock) {
	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		DisplayText("[오류] 메모리가 부족합니다.\r\n");
		return FALSE;
	}
	ptr->sock = sock;
	ptr->recvBytes = 0;
	ptr->sendBytes = 0;
	ptr->recvDelayed = FALSE;
	ptr->next = socketInfoList;
	socketInfoList = ptr;
	return TRUE;
}

SOCKETINFO* GetSocketInfo(SOCKET sock) {
	SOCKETINFO* ptr = socketInfoList;
	while (ptr) {
		if (ptr->sock == sock) {
			return ptr;
		}
		ptr = ptr->next;
	}
	return NULL;
}

void RemoveSocketInfo(SOCKET sock) {
	SOCKADDR_IN client_addr;
	int addrLen = sizeof(client_addr);
	getpeername(sock, (SOCKADDR*)&client_addr, &addrLen);
	DisplayText("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\r\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	SOCKETINFO* curr = socketInfoList;
	SOCKETINFO* prev = nullptr;
	
	while (curr) {
		if (curr->sock == sock) {
			if (prev) {
				prev->next = curr->next;
			}
			else {
				socketInfoList = curr->next;
			}
			closesocket(curr->sock);
			delete curr;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
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
	DisplayText("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void err_display(int errcode) {
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[오류] %s", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void DisplayText(char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hEdit);
	SendMessage(hEdit, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}