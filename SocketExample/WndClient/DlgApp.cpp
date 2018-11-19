#include <Windows.h>
#include <iostream>
#include "resource.h"

const int BUFSIZE = 25;	//위쪽 편집 컨트롤에 입력할 수 있는 글자의 최대 개수(영숫자 기준)

//대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

//편집 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);		//아래쪽 편집 컨트롤에 문자열을 출력하는 함수

//편집 컨트롤
HWND hEdit1, hEdit2;	//두 편집 컨트롤에 언제든지 접근할 수 있도록 핸들 값을 저장할 전역 변수

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static char buf[BUFSIZE + 1];	//위쪽 편집 컨트롤에 입력한 문자열을 저장하기 위한 버퍼

	switch (uMsg)
	{
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);	//전역 변수에 핸들 값을 저장해둔다.
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);	//위쪽 편집 컨트롤에 입력할 수 있는 글자의 최대 개수를 설정
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT1, buf, BUFSIZE + 1);	//위쪽 편집 컨트롤에 입력된 문자열을 얻어서 아래쪽 편집 컨트롤에 출력한다.
			DisplayText("%s\r\n", buf);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);	//위쪽 편집 컨트롤에 입력된 문자열이 모두 선택 상태가 되게 한다.
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