#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include "resource1.h"

#pragma comment(lib, "ws2_32")

const char* SERVERIP = "127.0.0.1";
const int SERVERPORT = 9000;
const int BUFSIZE = 512;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

void DisplayText(const char* fmt, ...);

void err_quit(const char* msg);
void err_display(const char* msg);

int recvn(SOCKET s, char* buf, int len, int flags);

DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock;		//소켓
char buf[BUFSIZE + 1];	//데이터 송수신 버퍼
HANDLE hReadEvent, hWriteEvent;	//읽기, 쓰기 이벤트
HWND hSendButton;	//보내기 버튼
HWND hEdit1, hEdit2;	//편집 컨트롤

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) {
		return 1;
	}
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) {
		return 1;
	}

	//소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
	
	//대화 상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG2), NULL, DlgProc);

	//이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	//소켓 종료
	closesocket(sock);

	//윈속 종료
	WSACleanup();
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT3);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT4);
		hSendButton = GetDlgItem(hDlg, IDOK);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:	//보내기 버튼을 눌렀을때
			EnableWindow(hSendButton, FALSE);	//보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE);	//읽기 이벤트가 set 될때까지 대기 (처음에 ReadEvent를 TRUE로 생성했으므로 바로 통과)
			GetDlgItemText(hDlg, IDC_EDIT3, buf, BUFSIZE + 1);	//메시지 창에 쓰인 글을 버퍼에 담는다.
			SetEvent(hWriteEvent);	//쓰기 완료 알림
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void DisplayText(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

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

DWORD WINAPI ClientMain(LPVOID arg) {
	int retval;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
	server_addr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		err_quit("connect()");
	}

	while (TRUE) {
		WaitForSingleObject(hWriteEvent, INFINITE);	//메인 스레드에서 WriteEvent가 set 될때까지 대기

		if (strlen(buf) == 0) {		//만일 버퍼에 아무것도 없다면 보내지 않고 다시 ReadEvent를 set 시켜 입력을 받는다.
			EnableWindow(hSendButton, TRUE);
			SetEvent(hReadEvent);
			continue;
		}

		retval = send(sock, buf, strlen(buf), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		DisplayText("[TCP 클라이언트] %d 바이트를 보냈습니다.\r\n", retval);

		retval = recvn(sock, buf, retval, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) {
			break;
		}
		buf[retval] = '\0';
		DisplayText("[TCP 클라이언트] %d 바이트를 받았습니다.\r\n", retval);
		DisplayText("[받은 데이터] %s\r\n", buf);

		EnableWindow(hSendButton, TRUE);	//메시지를 다 전송했으면 ReadEvent를 set 시켜 다음 입력을 대기한다.
		SetEvent(hReadEvent);
	}
	return 0;
}
