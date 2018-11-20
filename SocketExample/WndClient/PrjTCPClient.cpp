#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include "prjresource.h"

#pragma comment(lib, "ws2_32")

const char* SERVERIPV4 = "127.0.0.1";
const char* SERVERIPV6 = "::1";
const int SERVERPORT = 9000;

const int BUFSIZE = 256;
const int MSGSIZE = (BUFSIZE - sizeof(int));

const int CHATTING = 1000;
const int DRAWLINE = 1001;

const int WM_DRAWIT = (WM_USER + 1);

struct COMM_MSG {
	int type;
	char dummy[MSGSIZE];
};

struct CHAT_MSG {
	int type;
	char buf[MSGSIZE];
};

struct DRAWLINE_MSG {
	int type;
	int color;
	int x0, y0;
	int x1, y1;
	char dummy[BUFSIZE - 6 * sizeof(int)];
};

static HINSTANCE g_hInst;
static HWND g_hDrawWnd;
static HWND g_hButtonSendMsg;
static HWND g_hEditStatus;
static char g_ipAddr[64];
static u_short g_port;
static BOOL g_isIPv6;
static HANDLE g_hClientThread;
static volatile BOOL g_bStart;
static SOCKET g_sock;
static HANDLE g_hReadEvent, g_hWriteEvent;
static CHAT_MSG g_charMsg;
static DRAWLINE_MSG g_drawMsg;
static int g_drawColor;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI ReadThread(LPVOID arg);
DWORD WINAPI WriteThread(LPVOID arg);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void DisplayText(const char* fmt, ...);

int recvn(SOCKET s, char* buf, int len, int flags);

void err_quit(const char* msg);
void err_display(const char* msg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	g_hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (g_hReadEvent == NULL) {
		return 1;
	}
	g_hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (g_hWriteEvent == NULL) {
		return 1;
	}

	g_charMsg.type = CHATTING;
	g_drawMsg.type = DRAWLINE;
	g_drawMsg.color = RGB(255, 0, 0);

	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	CloseHandle(g_hReadEvent);
	CloseHandle(g_hWriteEvent);

	WSACleanup();
	return 0;
}

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

		SendMessage(hEditMsg, EM_SETLIMITTEXT, MSGSIZE, 0);
		EnableWindow(g_hButtonSendMsg, FALSE);
		SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);
		SetDlgItemInt(hDlg, IDC_PORT, SERVERPORT, FALSE);
		SendMessage(hColorRed, BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(hColorGreen, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hColorBlue, BM_SETCHECK, BST_UNCHECKED, 0);

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

		g_hDrawWnd = CreateWindow("MyWndClass", "그림 그릴 윈도우", WS_CHILD, 450, 38, 425, 415, hDlg, (HMENU)NULL, g_hInst, NULL);
		if (g_hDrawWnd == NULL) {
			return 1;
		}
		ShowWindow(g_hDrawWnd, SW_SHOW);
		UpdateWindow(g_hDrawWnd);

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ISIPV6:
			g_isIPv6 = SendMessage(hButtonIsIPv6, BM_GETCHECK, 0, 0);
			if (g_isIPv6 == false) {
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);
			}
			else {
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV6);
			}
			return TRUE;
		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_IPADDR, g_ipAddr, sizeof(g_ipAddr));
			g_port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
			g_isIPv6 = SendMessage(hButtonIsIPv6, BM_GETCHECK, 0, 0);

			g_hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
			if (g_hClientThread == NULL) {
				MessageBox(hDlg, "클라이언트를 시작할 수 없습니다. \r\n 프로그램을 종료합니다.", "실패!", MB_ICONERROR);
				EndDialog(hDlg, 0);
			}
			else {
				EnableWindow(hButtonConnect, FALSE);
				while (g_bStart == FALSE);
				EnableWindow(hButtonIsIPv6, FALSE);
				EnableWindow(hEditIPAddr, FALSE);
				EnableWindow(hEditPort, FALSE);
				EnableWindow(g_hButtonSendMsg, TRUE);
				SetFocus(hEditMsg);
			}
			return TRUE;
		case IDC_SENDMSG:
			WaitForSingleObject(g_hReadEvent, INFINITE);
			GetDlgItemText(hDlg, IDC_MSG, g_charMsg.buf, MSGSIZE);
			SetEvent(g_hWriteEvent);
			SendMessage(hEditMsg, EM_SETSEL, 0, -1);
			return TRUE;
		case IDC_COLORRED:
			g_drawMsg.color = RGB(255, 0, 0);
			return TRUE;
		case IDC_COLORGREEN:
			g_drawMsg.color = RGB(0, 255, 0);
			return TRUE;
		case IDC_COLORBLUE:
			g_drawMsg.color = RGB(0, 0, 255);
			return TRUE;
		case IDCANCEL:
			if (MessageBox(hDlg, "정말로 종료하시겠습니까?", "질문", MB_YESNO | MB_ICONQUESTION) == IDYES) {
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
	MessageBox(NULL, "서버에 접속했습니다.", "성공!", MB_ICONINFORMATION);

	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread, NULL, 0, NULL);
	if (hThread[0] == NULL || hThread[1] == NULL) {
		MessageBox(NULL, "스레드를 시작할 수 없습니다. \r\n프로그램을 종료합니다.", "실패!", MB_ICONERROR);
		exit(1);
	}

	g_bStart = TRUE;

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

	g_bStart = FALSE;

	MessageBox(NULL, "서버가 접속을 끊었습니다.", "알림", MB_ICONINFORMATION);
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
		retval = recvn(g_sock, (char*)&comm_msg, BUFSIZE, 0);
		if (retval == 0 || retval == SOCKET_ERROR) {
			break;
		}
		if (comm_msg.type == CHATTING) {
			chat_msg = (CHAT_MSG*)&comm_msg;
			DisplayText("[받은 메시지] %s\r\n", chat_msg->buf);
		}
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
		WaitForSingleObject(g_hWriteEvent, INFINITE);

		if (strlen(g_charMsg.buf) == 0) {
			EnableWindow(g_hButtonSendMsg, TRUE);
			SetEvent(g_hReadEvent);
			continue;
		}

		retval = send(g_sock, (char*)&g_charMsg, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			break;
		}

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

		cx = GetDeviceCaps(hDC, HORZRES);
		cy = GetDeviceCaps(hDC, VERTRES);
		hBitmap = CreateCompatibleBitmap(hDC, cx, cy);

		hDCMem = CreateCompatibleDC(hDC);

		SelectObject(hDCMem, hBitmap);
		SelectObject(hDCMem, GetStockObject(WHITE_BRUSH));
		SelectObject(hDCMem, GetStockObject(WHITE_PEN));
		Rectangle(hDCMem, 0, 0, cx, cy);

		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_LBUTTONDOWN:
		x0 = LOWORD(lParam);
		y0 = HIWORD(lParam);
		bDrawing = TRUE;
		return 0;
	case WM_MOUSEMOVE:
		if (bDrawing && g_bStart) {
			x1 = LOWORD(lParam);
			y1 = HIWORD(lParam);

			g_drawMsg.x0 = x0;
			g_drawMsg.y0 = y0;
			g_drawMsg.x1 = x1;
			g_drawMsg.y1 = y1;
			send(g_sock, (char*)&g_drawMsg, BUFSIZE, 0);

			x0 = x1;
			y0 = y1;
		}
		return 0;
	case WM_LBUTTONUP:
		bDrawing = FALSE;
		return 0;
	case WM_DRAWIT:
		hDC = GetDC(hWnd);
		hPen = CreatePen(PS_SOLID, 3, g_drawColor);

		hOldPen = (HPEN)SelectObject(hDC, hPen);
		MoveToEx(hDC, LOWORD(wParam), HIWORD(wParam), NULL);
		LineTo(hDC, LOWORD(lParam), HIWORD(lParam));
		SelectObject(hDC, hOldPen);

		hOldPen = (HPEN)SelectObject(hDCMem, hPen);
		MoveToEx(hDCMem, LOWORD(wParam), HIWORD(wParam), NULL);
		LineTo(hDCMem, LOWORD(lParam), HIWORD(lParam));
		SelectObject(hDC, hOldPen);

		DeleteObject(hPen);
		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);

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

