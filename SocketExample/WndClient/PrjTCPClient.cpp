#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include "prjresource.h"

#pragma comment(lib, "ws2_32")

const char* SERVERIPV4 = "127.0.0.1";
const char* SERVERIPV6 = "::1";
const int SERVERPORT = 9000;

const int BUFSIZE = 256;	//전송 메시지 전체 크기
const int MSGSIZE = (BUFSIZE - sizeof(int));	//채팅 메시지 최대 길이

const int CHATTING = 1000;	//메시지 타입: 채팅
const int DRAWLINE = 1001;	//메시지 타입: 선 그리기

const int WM_DRAWIT = (WM_USER + 1);	//사용자 정의 윈도우 메시지
//공통 메시지 형식
struct COMM_MSG {
	int type;
	char dummy[MSGSIZE];
};
//채팅 메시지 형식
//sizeof(CHAT_MSG) == 256
struct CHAT_MSG {
	int type;
	char buf[MSGSIZE];
};
//선 그리기 메시지 형식
//sizeof(DRAWLINE_MSG) == 256
struct DRAWLINE_MSG {
	int type;
	int color;
	int x0, y0;
	int x1, y1;
	char dummy[BUFSIZE - 6 * sizeof(int)];
};

static HINSTANCE g_hInst;					//응용 프로그램 인스턴스 핸들
static HWND g_hDrawWnd;						//그림을 그릴 윈도우
static HWND g_hButtonSendMsg;				//메시지 전송 버튼
static HWND g_hEditStatus;					//받은 메시지 출력
static char g_ipAddr[64];					//서버 IP 주소
static u_short g_port;						//서버 PORT 번호
static BOOL g_isIPv6;						//IP주소 버전
static HANDLE g_hClientThread;				//스레드 핸들
static volatile BOOL g_bStart;				//통신 시작 여부
static SOCKET g_sock;						//클라이언트 소켓
static HANDLE g_hReadEvent, g_hWriteEvent;	//이벤트 핸들
static CHAT_MSG g_chatMsg;					//채팅 메시지 저장
static DRAWLINE_MSG g_drawMsg;				//선 그리기 메시지 저장
static int g_drawColor;						//선 그리기 색상

//대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

//소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI ReadThread(LPVOID arg);
DWORD WINAPI WriteThread(LPVOID arg);

//자식 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//편집 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);

//사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags);

//오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//윈속 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	//이벤트 생성, DlgProc(), WriteThread() 함수에서 스레드 동기화에 사용.
	g_hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (g_hReadEvent == NULL) {
		return 1;
	}
	g_hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (g_hWriteEvent == NULL) {
		return 1;
	}

	//일부 전역 변수 초기화
	g_chatMsg.type = CHATTING;
	g_drawMsg.type = DRAWLINE;
	g_drawMsg.color = RGB(255, 0, 0);

	//대화상자 생성
	g_hInst = hInstance;	//인스턴스 핸들 값 저장, DlgProc() 함수에서 WM_INITDIALOG 메시지에 대응하여 윈도우를 생성할 때 필요.
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	
	//이벤트 제거
	CloseHandle(g_hReadEvent);
	CloseHandle(g_hWriteEvent);

	//윈속 종료
	WSACleanup();
	return 0;
}

//대화상자 프로시저
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
		//컨트롤의 핸들 값을 얻는다.
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
		
		//컨트롤 초기화
		SendMessage(hEditMsg, EM_SETLIMITTEXT, MSGSIZE, 0);			//입력 글자 수 제한
		EnableWindow(g_hButtonSendMsg, FALSE);						//아직 접속 전이므로 보내기 버튼 비활성화
		SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);				//서버 IP를 IPv4로 초기화
		SetDlgItemInt(hDlg, IDC_PORT, SERVERPORT, FALSE);			//서버 PORT를 9000으로 초기화
		SendMessage(hColorRed, BM_SETCHECK, BST_CHECKED, 0);		//색깔 중 빨간색만 선택한 상태로 초기화
		SendMessage(hColorGreen, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hColorBlue, BM_SETCHECK, BST_UNCHECKED, 0);

		//윈도우 클래스 등록
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

		//대화상자 오른쪽 영역에 그림을 그릴 수 있도록 자식 윈도우 생성
		g_hDrawWnd = CreateWindow("MyWndClass", "그림 그릴 윈도우", WS_CHILD, 450, 38, 425, 415, hDlg, (HMENU)NULL, g_hInst, NULL);
		if (g_hDrawWnd == NULL) {
			return 1;
		}
		ShowWindow(g_hDrawWnd, SW_SHOW);
		UpdateWindow(g_hDrawWnd);

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ISIPV6:	//IP 주소 버전을 클릭하면 해당 IP 주소 버전으로 주소를 바꾼다.
			g_isIPv6 = SendMessage(hButtonIsIPv6, BM_GETCHECK, 0, 0);
			if (g_isIPv6 == false) {
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);
			}
			else {
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV6);
			}
			return TRUE;
		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_IPADDR, g_ipAddr, sizeof(g_ipAddr));	//편집 컨트롤에 입력된 IP 주소와 포트 번호를 얻고 IPv6인지 확인한다.
			g_port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
			g_isIPv6 = SendMessage(hButtonIsIPv6, BM_GETCHECK, 0, 0);

			g_hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);	//소켓 통신 스레드를 생성한다.
			if (g_hClientThread == NULL) {	//스레드가 만들어지지 않은 경우
				MessageBox(hDlg, "클라이언트를 시작할 수 없습니다. \r\n 프로그램을 종료합니다.", "실패!", MB_ICONERROR);
				EndDialog(hDlg, 0);
			}
			else {										//스레드가 정상적으로 만들어진 경우(접속된 경우) 사용자가 정상적으로 조작할 필요가 없는 컨트롤을 모두 비활성화 후 메시지 전송 버튼 활성화
				EnableWindow(hButtonConnect, FALSE);
				while (g_bStart == FALSE);				//서버 접속 성공 기다림
				EnableWindow(hButtonIsIPv6, FALSE);		//IP 버전 체크 비활성화
				EnableWindow(hEditIPAddr, FALSE);		//IP 주소 창 비활성화
				EnableWindow(hEditPort, FALSE);			//PORT 번호 창 비활성화
				EnableWindow(g_hButtonSendMsg, TRUE);	//메시지 전송 버튼 활성화
				SetFocus(hEditMsg);						//메시지 입력 창에 포커스
			}
			return TRUE;
		case IDC_SENDMSG:	//메시지 전송 버튼 클릭시 현재 입력된 채팅 메시지를 전송한다.
			WaitForSingleObject(g_hReadEvent, INFINITE);			//읽기 이벤트가 set 될 때까지 대기
			GetDlgItemText(hDlg, IDC_MSG, g_chatMsg.buf, MSGSIZE);	//채팅 창에 있는 메시지를 버퍼에 복사
			SetEvent(g_hWriteEvent);								//쓰기 이벤트 set
			SendMessage(hEditMsg, EM_SETSEL, 0, -1);
			return TRUE;
		case IDC_COLORRED:	//색상을 선택하면 전역 변수 g_drawMsg에 저장해둔다. 나중에 선 그리기 메시지를 네트워크로 전송할 때 그대로 사용한다.
			g_drawMsg.color = RGB(255, 0, 0);
			return TRUE;
		case IDC_COLORGREEN:
			g_drawMsg.color = RGB(0, 255, 0);
			return TRUE;
		case IDC_COLORBLUE:
			g_drawMsg.color = RGB(0, 0, 255);
			return TRUE;
		case IDCANCEL:	//대화상자 오른쪽 상단 X 부분을 클릭할 때 발생한다.
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
	//선택한 IP 버전에 따라 소켓을 생성
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
	MessageBox(NULL, "서버에 접속했습니다.", "성공!", MB_ICONINFORMATION);

	//네트워크 데이터 수신과 송신을 독립적으로 하려고 스레드 두 개 생성
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread, NULL, 0, NULL);
	if (hThread[0] == NULL || hThread[1] == NULL) {
		MessageBox(NULL, "스레드를 시작할 수 없습니다. \r\n프로그램을 종료합니다.", "실패!", MB_ICONERROR);
		exit(1);
	}
	//서버와 통신할 준비가 됐으므로 전역 변수 g_bStart를 TRUE로 설정
	g_bStart = TRUE;

	//두 스레드(ReadThread, WriteThread)가 종료 하기를 기다린다. 둘 중 하나만 종료하면 나머지 하나도 강제로 종료시킨다.
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

	//통신용 스레드가 종료하여 서버가 통신할 수 없으므로 전역 변수 g_bStart를 FALSE로 설정
	g_bStart = FALSE;

	MessageBox(NULL, "서버가 접속을 끊었습니다.", "알림", MB_ICONINFORMATION);
	//채팅 메시지를 보내지 못하도록 메시지 전송 버튼을 비활성화
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
		//256 바이트 고정 길이 데이터를 읽고 오류를 처리한다.
		retval = recvn(g_sock, (char*)&comm_msg, BUFSIZE, 0);
		if (retval == 0 || retval == SOCKET_ERROR) {
			break;
		}
		//받은 메시지 타입이 채팅 메시지면 CHAT_MSG 구조체로 형변환하여 멤버 내용을 DisplayText() 함수로 편집 컨트롤에 출력한다.
		if (comm_msg.type == CHATTING) {
			chat_msg = (CHAT_MSG*)&comm_msg;
			DisplayText("[받은 메시지] %s\r\n", chat_msg->buf);
		}
		//받은 메시지 타입이 선 그리기 메시지면 DRAWLINE_MSG 구조체로 형변환한 후 좌표 정보를 꺼내 SendMessage() 함수로 윈도우에 전달한다.
		//이때 색상 정보는 전역 변수 g_drawColor에 저장해둔다.
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
		//writeEvent가 set 될 때까지 대기	(데이터 전송 버튼이나 엔터를 누를 경우)
		WaitForSingleObject(g_hWriteEvent, INFINITE);

		//입력된 문자열 길이가 0이면 데이터를 전송하지 않는다.
		if (strlen(g_chatMsg.buf) == 0) {
			EnableWindow(g_hButtonSendMsg, TRUE);
			SetEvent(g_hReadEvent);
			continue;
		}

		//데이터를 전송
		retval = send(g_sock, (char*)&g_chatMsg, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			break;
		}
		//사용자가 다음 데이터를 입력할 수 있도록 메시지 전송 버튼을 활성화하고 이벤트 객체를 통해 읽기 완료 사실을 대화상자에 알려준다.
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

		//화면을 저장할 비트맵 생성
		cx = GetDeviceCaps(hDC, HORZRES);
		cy = GetDeviceCaps(hDC, VERTRES);
		hBitmap = CreateCompatibleBitmap(hDC, cx, cy);

		//메모리 DC 생성
		hDCMem = CreateCompatibleDC(hDC);	//메모리에 존재하는 출력 영역

		//비트맵 선택 후 메모리 DC 화면을 흰색으로 칠함
		SelectObject(hDCMem, hBitmap);
		SelectObject(hDCMem, GetStockObject(WHITE_BRUSH));
		SelectObject(hDCMem, GetStockObject(WHITE_PEN));
		Rectangle(hDCMem, 0, 0, cx, cy);

		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_LBUTTONDOWN:
		x0 = LOWORD(lParam);	//마우스 왼쪽 버튼을 눌렀을때 좌표를 기억해둔다.
		y0 = HIWORD(lParam);
		bDrawing = TRUE;		//누르는 동안 bDrawing 변수는 TRUE가 된다.
		return 0;
	case WM_MOUSEMOVE:
		if (bDrawing && g_bStart) {
			x1 = LOWORD(lParam);	//마우스를 움직일때 좌표를 저장한다.
			y1 = HIWORD(lParam);

			//선 그리기 메시지 보내기
			g_drawMsg.x0 = x0;
			g_drawMsg.y0 = y0;
			g_drawMsg.x1 = x1;
			g_drawMsg.y1 = y1;
			send(g_sock, (char*)&g_drawMsg, BUFSIZE, 0);

			x0 = x1;	//저장된 마우스 좌표를 갱신한다.
			y0 = y1;
		}
		return 0;
	case WM_LBUTTONUP:	//마우스 왼쪽 버튼을 떼었으므로 bDrawing 변수는 FALSE가 된다.
		bDrawing = FALSE;
		return 0;
	case WM_DRAWIT:	//wParam과 lParam에 전달된 직선의 좌표 값과 전역 변수 g_drawColor에 저장된 색상 값을 참조하여 직선을 그린다.
		hDC = GetDC(hWnd);
		hPen = CreatePen(PS_SOLID, 3, g_drawColor);

		//화면에 그리기
		hOldPen = (HPEN)SelectObject(hDC, hPen);
		MoveToEx(hDC, LOWORD(wParam), HIWORD(wParam), NULL);
		LineTo(hDC, LOWORD(lParam), HIWORD(lParam));
		SelectObject(hDC, hOldPen);

		//메모리 비트맵에 그리기	(화면을 복원할 때 사용)
		hOldPen = (HPEN)SelectObject(hDCMem, hPen);
		MoveToEx(hDCMem, LOWORD(wParam), HIWORD(wParam), NULL);
		LineTo(hDCMem, LOWORD(lParam), HIWORD(lParam));
		SelectObject(hDC, hOldPen);

		DeleteObject(hPen);
		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_PAINT:	//운영체제는 윈도우가 화면을 다시 그려야 하는 상황이 되면 WM_PAINT 메시지를 발생시킨다.
		//이 메시지를 처리하면 화면을 복원할 수 있다. 여기서는 메모리DC에 저장된 내용응 BitBlt() 함수로 화면에 고속 전송함으로써 이전에 마우스로 그린 그림을 복구한다.
		hDC = BeginPaint(hWnd, &ps);

		//메모리 비트맵에 저장된 그림을 화면에 전송
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

