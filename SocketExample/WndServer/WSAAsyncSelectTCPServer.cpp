#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32")

#define WM_SOCKET (WM_USER + 1)

const int SERVERPORT = 9000;
const int BUFSIZE = 512;

struct SOCKETINFO {				//���� ���� ������ ���� ����ü
	SOCKET sock;				//Ŭ���̾�Ʈ ����
	char buf[BUFSIZE + 1];		//�ش� ������ ����
	int recvBytes;				//���� ����Ʈ ��
	int sendBytes;				//���� ����Ʈ ��
	BOOL recvDelayed;			//FD_READ �޽����� �޾����� ���� �Լ� recv()�Լ��� ȣ������ ���� ��� TRUE�� �����Ѵ�.
	SOCKETINFO* next;			//SOCKETINFO�� ���� ���� ����Ʈ�� �����ϱ� ���� ���� SOCKETINFO ������
};

SOCKETINFO* socketInfoList;		//SOCKETINFO ���� ���� ����Ʈ�� ���(������)

HWND hEdit;
HINSTANCE hInst;

//������ �޽��� ó�� �Լ�
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);

//���ϰ����Լ�
BOOL AddSocketInfo(SOCKET sock);
SOCKETINFO* GetSocketInfo(SOCKET sock);
void RemoveSocketInfo(SOCKET sock);

//��������Լ�
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);
void DisplayText(char *fmt, ...);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	hInst = hInstance;

	//������ Ŭ���� ���
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

	//������ ����
	HWND hWnd = CreateWindow("MyWndClass", "TCP ����", WS_OVERLAPPEDWINDOW, 0, 0, 600, 200, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//Winsock �ʱ�ȭ
	int retval;
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

	//WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);	//���� ��� ������ �ۼ��� ������ �ƴϹǷ� FD_ACCEPT, FD_CLOSE �̺�Ʈ�� ���
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
	case WM_SOCKET:		//���� ���� ������ �޽���
		ProcessSocketMessage(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//������ ��ſ� ����� ����
	SOCKETINFO* ptr;
	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int addrLen, retval;

	//���� �߻� ���� Ȯ��(lParam�� ����16��Ʈ)
	if (WSAGETSELECTERROR(lParam)) {
		err_display(WSAGETSELECTERROR(lParam));
		RemoveSocketInfo(wParam);
		return;
	}
	
	//��Ʈ��ũ �̺�Ʈ �� �޽��� ó��(lParam�� ����16��Ʈ)
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_ACCEPT:		//accept()
		addrLen = sizeof(client_addr);
		client_sock = accept(wParam, (SOCKADDR*)&client_addr, &addrLen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			return;
		}
		DisplayText("\r\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ� = %s, ��Ʈ ��ȣ = %d\r\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		AddSocketInfo(client_sock);
		//Ŭ���̾�Ʈ ���Ͽ� ���� WSAAsyncSelect()�Լ��� ȣ�������ν� ���� �ִ� �̺�Ʈ�� �ٽ� ����Ѵ�.
		//������ ���ۿ� ����� ���̹Ƿ� FD_READ, FD_WRITE_ FD_CLOSE �̺�Ʈ�� ����Ѵ�.
		retval = WSAAsyncSelect(client_sock, hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);	
		if (retval == SOCKET_ERROR) {
			err_display("WSAAsyncSelect()");
			RemoveSocketInfo(client_sock);
		}
		break;
	case FD_READ:		//recv()
		ptr = GetSocketInfo(wParam);
		//������ �޾����� ���� ������ ���� �����Ͱ� �ִٸ� recv()�� ȣ������ �ʰ� �ٷ� �����Ѵ�.
		//��, recvDelayed ������ TRUE�� �����Ͽ� �� ����� ����Ѵ�. ���� �Լ��� ȣ������ �ʾ����Ƿ� ���� �����Ͱ� �ִ��� FD_READ�̺�Ʈ�� �ٽ� �߻����� �ʴ´�.
		//���� ���߿� PostMessage() API �Լ��� ����Ͽ� ���� ���α׷��� ���� FD_READ �̺�Ʈ�� �߻����Ѿ� �Ѵ�.
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
		//FD_READ �̺�Ʈ ó�� �� �ٷ� FD_WRITE �̺�Ʈ�� ó���Ѵ�.
	case FD_WRITE:
		ptr = GetSocketInfo(wParam);
		if (ptr->recvBytes <= ptr->sendBytes) {		//���� ����Ʈ ���� ���� ����Ʈ ������ ũ�� ������ ���� �����Ͱ� �����Ƿ� �ٷ� ����
			return;
		}
		retval = send(ptr->sock, ptr->buf + ptr->sendBytes, ptr->recvBytes - ptr->sendBytes, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			RemoveSocketInfo(wParam);
			return;
		}
		ptr->sendBytes += retval;
		if (ptr->recvBytes == ptr->sendBytes) {		//���� �����͸� ��� ���´ٸ� ���� ����Ʈ ���� ���� ����Ʈ ���� 0 ���� �ʱ�ȭ
			ptr->recvBytes = ptr->sendBytes = 0;
			//���� FD_READ �̺�Ʈ�� ���� ���� �Լ��� ȣ������ �ʾҴٸ� PostMessage() API �Լ��� ȣ���� ������ �߻���Ų��.
			//���� ���� FD_READ �̺�Ʈ�� �߻��ϹǷ� ������ ���������� ó������ ���� �����͸� ���� �� �ִ�.
			if (ptr->recvDelayed) {
				ptr->recvDelayed = FALSE;
				PostMessage(hWnd, WM_SOCKET, wParam, FD_READ);
			}
		}
		break;
	case FD_CLOSE:		//���� ���Ḧ �ǹ��ϹǷ� ������ �ݰ� ���� ������ �����Ѵ�.
		RemoveSocketInfo(wParam);
		break;
	}
}

BOOL AddSocketInfo(SOCKET sock) {
	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		DisplayText("[����] �޸𸮰� �����մϴ�.\r\n");
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
	DisplayText("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\r\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
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
	DisplayText("[����] %s", (char*)lpMsgBuf);
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