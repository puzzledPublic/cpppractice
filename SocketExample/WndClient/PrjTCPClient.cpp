#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include "prjresource.h"

#pragma comment(lib, "ws2_32")

const char* SERVERIPV4 = "127.0.0.1";
const char* SERVERIPV6 = "::1";
const int SERVERPORT = 9000;

const int BUFSIZE = 256;	//���� �޽��� ��ü ũ��
const int MSGSIZE = (BUFSIZE - sizeof(int));	//ä�� �޽��� �ִ� ����

const int CHATTING = 1000;	//�޽��� Ÿ��: ä��
const int DRAWLINE = 1001;	//�޽��� Ÿ��: �� �׸���

const int WM_DRAWIT = (WM_USER + 1);	//����� ���� ������ �޽���
//���� �޽��� ����
struct COMM_MSG {
	int type;
	char dummy[MSGSIZE];
};
//ä�� �޽��� ����
//sizeof(CHAT_MSG) == 256
struct CHAT_MSG {
	int type;
	char buf[MSGSIZE];
};
//�� �׸��� �޽��� ����
//sizeof(DRAWLINE_MSG) == 256
struct DRAWLINE_MSG {
	int type;
	int color;
	int x0, y0;
	int x1, y1;
	char dummy[BUFSIZE - 6 * sizeof(int)];
};

static HINSTANCE g_hInst;					//���� ���α׷� �ν��Ͻ� �ڵ�
static HWND g_hDrawWnd;						//�׸��� �׸� ������
static HWND g_hButtonSendMsg;				//�޽��� ���� ��ư
static HWND g_hEditStatus;					//���� �޽��� ���
static char g_ipAddr[64];					//���� IP �ּ�
static u_short g_port;						//���� PORT ��ȣ
static BOOL g_isIPv6;						//IP�ּ� ����
static HANDLE g_hClientThread;				//������ �ڵ�
static volatile BOOL g_bStart;				//��� ���� ����
static SOCKET g_sock;						//Ŭ���̾�Ʈ ����
static HANDLE g_hReadEvent, g_hWriteEvent;	//�̺�Ʈ �ڵ�
static CHAT_MSG g_chatMsg;					//ä�� �޽��� ����
static DRAWLINE_MSG g_drawMsg;				//�� �׸��� �޽��� ����
static int g_drawColor;						//�� �׸��� ����

//��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

//���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI ReadThread(LPVOID arg);
DWORD WINAPI WriteThread(LPVOID arg);

//�ڽ� ������ ���ν���
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//���� ��Ʈ�� ��� �Լ�
void DisplayText(const char* fmt, ...);

//����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char* buf, int len, int flags);

//���� ��� �Լ�
void err_quit(const char* msg);
void err_display(const char* msg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//���� �ʱ�ȭ
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//�̺�Ʈ ����, DlgProc(), WriteThread() �Լ����� ������ ����ȭ�� ���.
	g_hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (g_hReadEvent == NULL) {
		return 1;
	}
	g_hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (g_hWriteEvent == NULL) {
		return 1;
	}

	//�Ϻ� ���� ���� �ʱ�ȭ
	g_chatMsg.type = CHATTING;
	g_drawMsg.type = DRAWLINE;
	g_drawMsg.color = RGB(255, 0, 0);

	//��ȭ���� ����
	g_hInst = hInstance;	//�ν��Ͻ� �ڵ� �� ����, DlgProc() �Լ����� WM_INITDIALOG �޽����� �����Ͽ� �����츦 ������ �� �ʿ�.
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	
	//�̺�Ʈ ����
	CloseHandle(g_hReadEvent);
	CloseHandle(g_hWriteEvent);

	//���� ����
	WSACleanup();
	return 0;
}

//��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hButtonIsIPv6;
	static HWND hEditIPAddr;
	static HWND hEditPort;
	static HWND hButtonConnect;
	static HWND hEditMsg;
	static HWND hColorRed;
	static HWND hColorGreen;
	static HWND hColorBlue;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		//��Ʈ���� �ڵ� ���� ��´�.
		hButtonIsIPv6 = GetDlgItem(hDlg, IDC_ISIPV6);
		hEditIPAddr = GetDlgItem(hDlg, IDC_IPADDR);
		hEditPort = GetDlgItem(hDlg, IDC_PORT);
		hButtonConnect = GetDlgItem(hDlg, IDC_CONNECT);
		g_hButtonSendMsg = GetDlgItem(hDlg, IDC_SENDMSG);
		hEditMsg = GetDlgItem(hDlg, IDC_MSG);
		g_hEditStatus = GetDlgItem(hDlg, IDC_STATUS);
		hColorRed = GetDlgItem(hDlg, IDC_COLORRED);
		hColorGreen = GetDlgItem(hDlg, IDC_COLORGREEN);
		hColorBlue = GetDlgItem(hDlg, IDC_COLORBLUE);
		
		//��Ʈ�� �ʱ�ȭ
		SendMessage(hEditMsg, EM_SETLIMITTEXT, MSGSIZE, 0);			//�Է� ���� �� ����
		EnableWindow(g_hButtonSendMsg, FALSE);						//���� ���� ���̹Ƿ� ������ ��ư ��Ȱ��ȭ
		SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);				//���� IP�� IPv4�� �ʱ�ȭ
		SetDlgItemInt(hDlg, IDC_PORT, SERVERPORT, FALSE);			//���� PORT�� 9000���� �ʱ�ȭ
		SendMessage(hColorRed, BM_SETCHECK, BST_CHECKED, 0);		//���� �� �������� ������ ���·� �ʱ�ȭ
		SendMessage(hColorGreen, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hColorBlue, BM_SETCHECK, BST_UNCHECKED, 0);

		//������ Ŭ���� ���
		WNDCLASS wndClass;
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = WndProc;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = g_hInst;
		wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndClass.lpszMenuName = NULL;
		wndClass.lpszClassName = "MyWndClass";
		if (!RegisterClass(&wndClass)) {
			return 1;
		}

		//��ȭ���� ������ ������ �׸��� �׸� �� �ֵ��� �ڽ� ������ ����
		g_hDrawWnd = CreateWindow("MyWndClass", "�׸� �׸� ������", WS_CHILD, 450, 38, 425, 415, hDlg, (HMENU)NULL, g_hInst, NULL);
		if (g_hDrawWnd == NULL) {
			return 1;
		}
		ShowWindow(g_hDrawWnd, SW_SHOW);
		UpdateWindow(g_hDrawWnd);

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ISIPV6:	//IP �ּ� ������ Ŭ���ϸ� �ش� IP �ּ� �������� �ּҸ� �ٲ۴�.
			g_isIPv6 = SendMessage(hButtonIsIPv6, BM_GETCHECK, 0, 0);
			if (g_isIPv6 == false) {
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);
			}
			else {
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV6);
			}
			return TRUE;
		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_IPADDR, g_ipAddr, sizeof(g_ipAddr));	//���� ��Ʈ�ѿ� �Էµ� IP �ּҿ� ��Ʈ ��ȣ�� ��� IPv6���� Ȯ���Ѵ�.
			g_port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
			g_isIPv6 = SendMessage(hButtonIsIPv6, BM_GETCHECK, 0, 0);

			g_hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);	//���� ��� �����带 �����Ѵ�.
			if (g_hClientThread == NULL) {	//�����尡 ��������� ���� ���
				MessageBox(hDlg, "Ŭ���̾�Ʈ�� ������ �� �����ϴ�. \r\n ���α׷��� �����մϴ�.", "����!", MB_ICONERROR);
				EndDialog(hDlg, 0);
			}
			else {										//�����尡 ���������� ������� ���(���ӵ� ���) ����ڰ� ���������� ������ �ʿ䰡 ���� ��Ʈ���� ��� ��Ȱ��ȭ �� �޽��� ���� ��ư Ȱ��ȭ
				EnableWindow(hButtonConnect, FALSE);
				while (g_bStart == FALSE);				//���� ���� ���� ��ٸ�
				EnableWindow(hButtonIsIPv6, FALSE);		//IP ���� üũ ��Ȱ��ȭ
				EnableWindow(hEditIPAddr, FALSE);		//IP �ּ� â ��Ȱ��ȭ
				EnableWindow(hEditPort, FALSE);			//PORT ��ȣ â ��Ȱ��ȭ
				EnableWindow(g_hButtonSendMsg, TRUE);	//�޽��� ���� ��ư Ȱ��ȭ
				SetFocus(hEditMsg);						//�޽��� �Է� â�� ��Ŀ��
			}
			return TRUE;
		case IDC_SENDMSG:	//�޽��� ���� ��ư Ŭ���� ���� �Էµ� ä�� �޽����� �����Ѵ�.
			WaitForSingleObject(g_hReadEvent, INFINITE);			//�б� �̺�Ʈ�� set �� ������ ���
			GetDlgItemText(hDlg, IDC_MSG, g_chatMsg.buf, MSGSIZE);	//ä�� â�� �ִ� �޽����� ���ۿ� ����
			SetEvent(g_hWriteEvent);								//���� �̺�Ʈ set
			SendMessage(hEditMsg, EM_SETSEL, 0, -1);
			return TRUE;
		case IDC_COLORRED:	//������ �����ϸ� ���� ���� g_drawMsg�� �����صд�. ���߿� �� �׸��� �޽����� ��Ʈ��ũ�� ������ �� �״�� ����Ѵ�.
			g_drawMsg.color = RGB(255, 0, 0);
			return TRUE;
		case IDC_COLORGREEN:
			g_drawMsg.color = RGB(0, 255, 0);
			return TRUE;
		case IDC_COLORBLUE:
			g_drawMsg.color = RGB(0, 0, 255);
			return TRUE;
		case IDCANCEL:	//��ȭ���� ������ ��� X �κ��� Ŭ���� �� �߻��Ѵ�.
			if (MessageBox(hDlg, "������ �����Ͻðڽ��ϱ�?", "����", MB_YESNO | MB_ICONQUESTION) == IDYES) {
				closesocket(g_sock);
				EndDialog(hDlg, IDCANCEL);
			}
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

DWORD WINAPI ClientMain(LPVOID arg) {
	int retval;
	//������ IP ������ ���� ������ ����
	//IP version 4
	if (g_isIPv6 == false) {
		g_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (g_sock == INVALID_SOCKET) {
			err_quit("socket()");
		}
		
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(g_ipAddr);
		server_addr.sin_port = ntohs(g_port);
		retval = connect(g_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
		if (retval == SOCKET_ERROR) {
			err_quit("connect()");
		}
	}
	//IP version 6
	else {
		g_sock = socket(AF_INET6, SOCK_STREAM, 0);
		if (g_sock == INVALID_SOCKET) {
			err_quit("socket()");
		}

		SOCKADDR_IN6 server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin6_family = AF_INET6;
		int addrLen = sizeof(server_addr);
		WSAStringToAddress(g_ipAddr, AF_INET6, NULL, (SOCKADDR*)&server_addr, &addrLen);
		server_addr.sin6_port = htons(g_port);
		retval = connect(g_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
		if (retval == SOCKET_ERROR) {
			err_quit("connect()");
		}
	}
	MessageBox(NULL, "������ �����߽��ϴ�.", "����!", MB_ICONINFORMATION);

	//��Ʈ��ũ ������ ���Ű� �۽��� ���������� �Ϸ��� ������ �� �� ����
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread, NULL, 0, NULL);
	if (hThread[0] == NULL || hThread[1] == NULL) {
		MessageBox(NULL, "�����带 ������ �� �����ϴ�. \r\n���α׷��� �����մϴ�.", "����!", MB_ICONERROR);
		exit(1);
	}
	//������ ����� �غ� �����Ƿ� ���� ���� g_bStart�� TRUE�� ����
	g_bStart = TRUE;

	//�� ������(ReadThread, WriteThread)�� ���� �ϱ⸦ ��ٸ���. �� �� �ϳ��� �����ϸ� ������ �ϳ��� ������ �����Ų��.
	retval = WaitForMultipleObjects(2, hThread, FALSE, INFINITE);
	retval -= WAIT_OBJECT_0;
	if (retval == 0) {
		TerminateThread(hThread[1], 1);
	}
	else {
		TerminateThread(hThread[0], 1);
	}
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	//��ſ� �����尡 �����Ͽ� ������ ����� �� �����Ƿ� ���� ���� g_bStart�� FALSE�� ����
	g_bStart = FALSE;

	MessageBox(NULL, "������ ������ �������ϴ�.", "�˸�", MB_ICONINFORMATION);
	//ä�� �޽����� ������ ���ϵ��� �޽��� ���� ��ư�� ��Ȱ��ȭ
	EnableWindow(g_hButtonSendMsg, FALSE);

	closesocket(g_sock);
	return 0;
}

DWORD WINAPI ReadThread(LPVOID arg) {
	int retval;
	COMM_MSG comm_msg;
	CHAT_MSG* chat_msg;
	DRAWLINE_MSG* draw_msg;

	while (TRUE) {
		//256 ����Ʈ ���� ���� �����͸� �а� ������ ó���Ѵ�.
		retval = recvn(g_sock, (char*)&comm_msg, BUFSIZE, 0);
		if (retval == 0 || retval == SOCKET_ERROR) {
			break;
		}
		//���� �޽��� Ÿ���� ä�� �޽����� CHAT_MSG ����ü�� ����ȯ�Ͽ� ��� ������ DisplayText() �Լ��� ���� ��Ʈ�ѿ� ����Ѵ�.
		if (comm_msg.type == CHATTING) {
			chat_msg = (CHAT_MSG*)&comm_msg;
			DisplayText("[���� �޽���] %s\r\n", chat_msg->buf);
		}
		//���� �޽��� Ÿ���� �� �׸��� �޽����� DRAWLINE_MSG ����ü�� ����ȯ�� �� ��ǥ ������ ���� SendMessage() �Լ��� �����쿡 �����Ѵ�.
		//�̶� ���� ������ ���� ���� g_drawColor�� �����صд�.
		else if (comm_msg.type == DRAWLINE) {
			draw_msg = (DRAWLINE_MSG*)&comm_msg;
			g_drawColor = draw_msg->color;
			SendMessage(g_hDrawWnd, WM_DRAWIT, MAKEWPARAM(draw_msg->x0, draw_msg->y0), MAKELPARAM(draw_msg->x1, draw_msg->y1));
		}
	}
	return 0;
}

DWORD WINAPI WriteThread(LPVOID arg) {
	int retval;
	
	while (TRUE) {
		//writeEvent�� set �� ������ ���	(������ ���� ��ư�̳� ���͸� ���� ���)
		WaitForSingleObject(g_hWriteEvent, INFINITE);

		//�Էµ� ���ڿ� ���̰� 0�̸� �����͸� �������� �ʴ´�.
		if (strlen(g_chatMsg.buf) == 0) {
			EnableWindow(g_hButtonSendMsg, TRUE);
			SetEvent(g_hReadEvent);
			continue;
		}

		//�����͸� ����
		retval = send(g_sock, (char*)&g_chatMsg, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			break;
		}
		//����ڰ� ���� �����͸� �Է��� �� �ֵ��� �޽��� ���� ��ư�� Ȱ��ȭ�ϰ� �̺�Ʈ ��ü�� ���� �б� �Ϸ� ����� ��ȭ���ڿ� �˷��ش�.
		EnableWindow(g_hButtonSendMsg, TRUE);
		SetEvent(g_hReadEvent);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	int cx, cy;
	PAINTSTRUCT ps;
	RECT rect;
	HPEN hPen, hOldPen;
	static HBITMAP hBitmap;
	static HDC hDCMem;
	static int x0, y0;
	static int x1, y1;
	static BOOL bDrawing = FALSE;

	switch (uMsg) {
	case WM_CREATE:
		hDC = GetDC(hWnd);

		//ȭ���� ������ ��Ʈ�� ����
		cx = GetDeviceCaps(hDC, HORZRES);
		cy = GetDeviceCaps(hDC, VERTRES);
		hBitmap = CreateCompatibleBitmap(hDC, cx, cy);

		//�޸� DC ����
		hDCMem = CreateCompatibleDC(hDC);	//�޸𸮿� �����ϴ� ��� ����

		//��Ʈ�� ���� �� �޸� DC ȭ���� ������� ĥ��
		SelectObject(hDCMem, hBitmap);
		SelectObject(hDCMem, GetStockObject(WHITE_BRUSH));
		SelectObject(hDCMem, GetStockObject(WHITE_PEN));
		Rectangle(hDCMem, 0, 0, cx, cy);

		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_LBUTTONDOWN:
		x0 = LOWORD(lParam);	//���콺 ���� ��ư�� �������� ��ǥ�� ����صд�.
		y0 = HIWORD(lParam);
		bDrawing = TRUE;		//������ ���� bDrawing ������ TRUE�� �ȴ�.
		return 0;
	case WM_MOUSEMOVE:
		if (bDrawing && g_bStart) {
			x1 = LOWORD(lParam);	//���콺�� �����϶� ��ǥ�� �����Ѵ�.
			y1 = HIWORD(lParam);

			//�� �׸��� �޽��� ������
			g_drawMsg.x0 = x0;
			g_drawMsg.y0 = y0;
			g_drawMsg.x1 = x1;
			g_drawMsg.y1 = y1;
			send(g_sock, (char*)&g_drawMsg, BUFSIZE, 0);

			x0 = x1;	//����� ���콺 ��ǥ�� �����Ѵ�.
			y0 = y1;
		}
		return 0;
	case WM_LBUTTONUP:	//���콺 ���� ��ư�� �������Ƿ� bDrawing ������ FALSE�� �ȴ�.
		bDrawing = FALSE;
		return 0;
	case WM_DRAWIT:	//wParam�� lParam�� ���޵� ������ ��ǥ ���� ���� ���� g_drawColor�� ����� ���� ���� �����Ͽ� ������ �׸���.
		hDC = GetDC(hWnd);
		hPen = CreatePen(PS_SOLID, 3, g_drawColor);

		//ȭ�鿡 �׸���
		hOldPen = (HPEN)SelectObject(hDC, hPen);
		MoveToEx(hDC, LOWORD(wParam), HIWORD(wParam), NULL);
		LineTo(hDC, LOWORD(lParam), HIWORD(lParam));
		SelectObject(hDC, hOldPen);

		//�޸� ��Ʈ�ʿ� �׸���	(ȭ���� ������ �� ���)
		hOldPen = (HPEN)SelectObject(hDCMem, hPen);
		MoveToEx(hDCMem, LOWORD(wParam), HIWORD(wParam), NULL);
		LineTo(hDCMem, LOWORD(lParam), HIWORD(lParam));
		SelectObject(hDC, hOldPen);

		DeleteObject(hPen);
		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_PAINT:	//�ü���� �����찡 ȭ���� �ٽ� �׷��� �ϴ� ��Ȳ�� �Ǹ� WM_PAINT �޽����� �߻���Ų��.
		//�� �޽����� ó���ϸ� ȭ���� ������ �� �ִ�. ���⼭�� �޸�DC�� ����� ������ BitBlt() �Լ��� ȭ�鿡 ��� ���������ν� ������ ���콺�� �׸� �׸��� �����Ѵ�.
		hDC = BeginPaint(hWnd, &ps);

		//�޸� ��Ʈ�ʿ� ����� �׸��� ȭ�鿡 ����
		GetClientRect(hWnd, &rect);
		BitBlt(hDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hDCMem, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		DeleteObject(hBitmap);
		DeleteDC(hDCMem);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int recvn(SOCKET s, char* buf, int len, int flags) {
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR) {
			return SOCKET_ERROR;
		}
		else if (received == 0) {
			break;
		}
		left -= received;
		ptr += received;
	}

	return (len - left);
}

void DisplayText(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditStatus);
	SendMessage(g_hEditStatus, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditStatus, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
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

