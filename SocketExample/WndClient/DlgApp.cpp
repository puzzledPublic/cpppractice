#include <Windows.h>
#include <iostream>
#include "resource.h"

const int BUFSIZE = 25;	//���� ���� ��Ʈ�ѿ� �Է��� �� �ִ� ������ �ִ� ����(������ ����)

//��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

//���� ��Ʈ�� ��� �Լ�
void DisplayText(const char* fmt, ...);		//�Ʒ��� ���� ��Ʈ�ѿ� ���ڿ��� ����ϴ� �Լ�

//���� ��Ʈ��
HWND hEdit1, hEdit2;	//�� ���� ��Ʈ�ѿ� �������� ������ �� �ֵ��� �ڵ� ���� ������ ���� ����

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static char buf[BUFSIZE + 1];	//���� ���� ��Ʈ�ѿ� �Է��� ���ڿ��� �����ϱ� ���� ����

	switch (uMsg)
	{
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);	//���� ������ �ڵ� ���� �����صд�.
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);	//���� ���� ��Ʈ�ѿ� �Է��� �� �ִ� ������ �ִ� ������ ����
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT1, buf, BUFSIZE + 1);	//���� ���� ��Ʈ�ѿ� �Էµ� ���ڿ��� �� �Ʒ��� ���� ��Ʈ�ѿ� ����Ѵ�.
			DisplayText("%s\r\n", buf);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);	//���� ���� ��Ʈ�ѿ� �Էµ� ���ڿ��� ��� ���� ���°� �ǰ� �Ѵ�.
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