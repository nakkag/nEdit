/*
 * nEdit
 *
 * main.c
 *
 * Copyright (C) 1996-2015 by Ohno Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef	_INC_OLE
#include <tchar.h>

#include "nEdit.h"
#include "resource.h"

/* Define */
#define MAIN_WND_CLASS					TEXT("nEditMainWndClass")
#define WINDOW_TITLE					TEXT("nEdit")

#define IDC_EDIT_EDIT					1

#define BUF_SIZE						256

#ifdef UNICODE
#define char_to_tchar_size(buf)			(MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0) - 1)
#else
#define char_to_tchar_size(buf)			(lstrlen(buf))
#endif

#ifdef UNICODE
#define char_to_tchar(buf, wret, len)	(MultiByteToWideChar(CP_ACP, 0, buf, -1, wret, len + 1))
#else
#define char_to_tchar(buf, wret, len)	(lstrcpyn(wret, buf, len + 1))
#endif

/* Global Variables */
static HINSTANCE hInst;
static TCHAR file_path[MAX_PATH];
static TCHAR cmd_line[BUF_SIZE];
static LOGFONT lf;

/* Local Function Prototypes */
static HFONT font_select(const HWND hWnd);
static void get_ini(const HWND hWnd, const TCHAR *path);
static void put_ini(const HWND hWnd, const TCHAR *path);
static BOOL read_file(const HWND hWnd, TCHAR *path);
static BOOL save_file(const HWND hWnd, TCHAR *path);
static BOOL save_confirm(const HWND hWnd);
static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL init_application(HINSTANCE hInstance);
static HWND init_instance(HINSTANCE hInstance, int CmdShow);

/*
 * message_get_error - エラー値からメッセージを取得
 */
static BOOL message_get_error(const int err_code, TCHAR *err_str)
{
	if (err_str == NULL) {
		return FALSE;
	}
	*err_str = TEXT('\0');
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_code, 0, err_str, BUF_SIZE - 1, NULL);
	return TRUE;
}

/*
 * message_get_res - リソースからメッセージを取得
 */
TCHAR *message_get_res(const UINT id)
{
	static TCHAR buf[BUF_SIZE];

	*buf = TEXT('\0');
	LoadString(hInst, id, buf, BUF_SIZE - 1);
	return buf;
}

/*
 * font_select - フォントの選択
 */
static HFONT font_select(const HWND hWnd)
{
	CHOOSEFONT cf;

	// フォント選択ダイアログを表示
	ZeroMemory(&cf, sizeof(CHOOSEFONT));
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
	cf.nFontType = SCREEN_FONTTYPE;
	if (ChooseFont(&cf) == FALSE) {
		return NULL;
	}
	return CreateFontIndirect((CONST LOGFONT *)&lf);
}

/*
 * get_ini - 設定読み込み
 */
static void get_ini(const HWND hWnd, const TCHAR *path)
{
	HFONT hFont;
	RECT rect;
	HDC hdc;

	rect.left = GetPrivateProfileInt(TEXT("WINDOW"), TEXT("left"), 0, path);
	rect.top = GetPrivateProfileInt(TEXT("WINDOW"), TEXT("top"), 0, path);
	rect.right = GetPrivateProfileInt(TEXT("WINDOW"), TEXT("right"), 500, path);
	rect.bottom = GetPrivateProfileInt(TEXT("WINDOW"), TEXT("bottom"), 300, path);
	SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_HIDEWINDOW);

	GetPrivateProfileString(TEXT("FONT"), TEXT("name"), TEXT(""), lf.lfFaceName, LF_FACESIZE - 1, path);
	lf.lfHeight = GetPrivateProfileInt(TEXT("FONT"), TEXT("height"), -16, path);
	lf.lfWeight = GetPrivateProfileInt(TEXT("FONT"), TEXT("weight"), 0, path);
	lf.lfItalic = GetPrivateProfileInt(TEXT("FONT"), TEXT("italic"), 0, path);
	hdc = GetDC(hWnd);
	lf.lfCharSet = GetPrivateProfileInt(TEXT("FONT"), TEXT("charset"), GetTextCharset(hdc), path);
	ReleaseDC(hWnd, hdc);
	if ((hFont = CreateFontIndirect((CONST LOGFONT *)&lf)) != NULL) {
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		DeleteObject(hFont);
	}

	if (GetPrivateProfileInt(TEXT("TEXT"), TEXT("wordwrap"), 0, path) == 0) {
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETWORDWRAP, FALSE, 0);
		CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), ID_MENUITEM_WORDWRAP, MF_UNCHECKED);
	} else {
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETWORDWRAP, TRUE, 0);
		CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), ID_MENUITEM_WORDWRAP, MF_CHECKED);
	}
}

/*
 * put_ini - 設定書き込み
 */
static void put_ini(const HWND hWnd, const TCHAR *path)
{
	RECT rect;
	TCHAR num_buf[BUF_SIZE];

	GetWindowRect(hWnd, (LPRECT)&rect);

	wsprintf(num_buf, TEXT("%d"), rect.left);
	WritePrivateProfileString(TEXT("WINDOW"), TEXT("left"), num_buf, path);
	wsprintf(num_buf, TEXT("%d"), rect.top);
	WritePrivateProfileString(TEXT("WINDOW"), TEXT("top"), num_buf, path);
	wsprintf(num_buf, TEXT("%d"), rect.right - rect.left);
	WritePrivateProfileString(TEXT("WINDOW"), TEXT("right"), num_buf, path);
	wsprintf(num_buf, TEXT("%d"), rect.bottom - rect.top);
	WritePrivateProfileString(TEXT("WINDOW"), TEXT("bottom"), num_buf, path);

	WritePrivateProfileString(TEXT("FONT"), TEXT("name"), lf.lfFaceName, path);
	wsprintf(num_buf, TEXT("%d"), lf.lfHeight);
	WritePrivateProfileString(TEXT("FONT"), TEXT("height"), num_buf, path);
	wsprintf(num_buf, TEXT("%d"), lf.lfWeight);
	WritePrivateProfileString(TEXT("FONT"), TEXT("weight"), num_buf, path);
	wsprintf(num_buf, TEXT("%d"), lf.lfItalic);
	WritePrivateProfileString(TEXT("FONT"), TEXT("italic"), num_buf, path);
	wsprintf(num_buf, TEXT("%d"), lf.lfCharSet);
	WritePrivateProfileString(TEXT("FONT"), TEXT("charset"), num_buf, path);

	wsprintf(num_buf, TEXT("%d"), (int)SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_GETWORDWRAP, 0, 0));
	WritePrivateProfileString(TEXT("TEXT"), TEXT("wordwrap"), num_buf, path);
}

/*
 * read_file - ファイルの読み込み
 */
static BOOL read_file(const HWND hWnd, TCHAR *path)
{
	OPENFILENAME of;
	HANDLE hFile;
	TCHAR *buf;
	TCHAR err_str[BUF_SIZE];
	DWORD fSizeLow, fSizeHigh;
	DWORD ret;

	if (*path == TEXT('\0')) {
		lstrcpy(path, TEXT("*.txt"));
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hwndOwner = hWnd;
		of.lpstrFilter = TEXT("*.txt\0*.txt\0*.*\0*.*\0\0");
		of.lpstrTitle = message_get_res(IDS_STRING_OPEN_TITLE);
		of.lpstrFile = path;
		of.nMaxFile = MAX_PATH - 1;
		of.lpstrDefExt = TEXT("txt");
		of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileName((LPOPENFILENAME)&of) == FALSE) {
			return FALSE;
		}
	}
	
	// ファイルを開く
	hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		message_get_error(GetLastError(), err_str);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}
	if ((fSizeLow = GetFileSize(hFile, &fSizeHigh)) == 0xFFFFFFFF) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}
	
	if ((buf = (TCHAR *)LocalAlloc(LPTR, fSizeLow)) == NULL) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}
	// ファイルを読み込む
	if (ReadFile(hFile, buf, fSizeLow, &ret, NULL) == FALSE) {
		message_get_error(GetLastError(), err_str);
		LocalFree(buf);
		CloseHandle(hFile);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}
#ifdef UNICODE
	if(fSizeLow > 2 && *((BYTE *)buf) == 0xFF && *(((BYTE *)buf) + 1) == 0xFE) {
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETMEM, ret - 2, (LPARAM)(((BYTE *)buf) + 2));
	} else {
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETMEM, ret, (LPARAM)buf);
	}
#else
	SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETMEM, ret, (LPARAM)buf);
#endif
	
	LocalFree(buf);
	CloseHandle(hFile);

	// EDIT の変更フラグを除去する
	SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_SETMODIFY, (WPARAM)FALSE, 0);
	return TRUE;
}

/*
 * save_file - ファイルの保存
 */
static BOOL save_file(const HWND hWnd, TCHAR *path)
{
	OPENFILENAME of;
	HANDLE hFile;
	TCHAR *buf;
	TCHAR err_str[BUF_SIZE];
	DWORD ret;
	int len;

	if (*path == TEXT('\0')) {
		lstrcpy(path, TEXT(""));
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hwndOwner = hWnd;
		of.lpstrFilter = TEXT("*.txt\0*.txt\0*.*\0*.*\0\0");
		of.lpstrTitle = message_get_res(IDS_STRING_SAVE_TITLE);
		of.lpstrFile = path;
		of.nMaxFile = MAX_PATH - 1;
		of.lpstrDefExt = TEXT("txt");
		of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		if (GetSaveFileName((LPOPENFILENAME)&of) == FALSE) {
			return FALSE;
		}
	}

	// ファイルを開く
	hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		message_get_error(GetLastError(), err_str);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}

	len = (int)SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_GETMEMSIZE, 0, 0);
	if ((buf = (TCHAR *)LocalAlloc(LPTR, len)) == NULL) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}
	SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_GETMEM, 0, (LPARAM)buf);
	
	// ファイルに書き込む
#ifdef UNICODE
	{
		BYTE tmp[2];
		tmp[0] = 0xFF;
		tmp[1] = 0xFE;
		if (WriteFile(hFile, tmp, 2, &ret, NULL) == FALSE) {
			message_get_error(GetLastError(), err_str);
			LocalFree(buf);
			CloseHandle(hFile);
			MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
			return FALSE;
		}
	}
#endif
	if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
		message_get_error(GetLastError(), err_str);
		LocalFree(buf);
		CloseHandle(hFile);
		MessageBox(hWnd, err_str, WINDOW_TITLE, MB_ICONERROR);
		return FALSE;
	}
	LocalFree(buf);
	CloseHandle(hFile);
	
	// EDIT の変更フラグを除去する
	SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_SETMODIFY, (WPARAM)FALSE, 0);
	return TRUE;
}

/*
 * save_confirm - 変更の保存確認
 */
static BOOL save_confirm(const HWND hWnd)
{
	if (SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_GETMODIFY, 0, 0) == TRUE) {
		switch (MessageBox(hWnd, message_get_res(IDS_STRING_MSG_MODIFY), WINDOW_TITLE, MB_ICONEXCLAMATION | MB_YESNOCANCEL)) {
		case IDYES:
			SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_SAVE, 0);
			if (SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_GETMODIFY, 0, 0) == TRUE) {
				return FALSE;
			}
			break;

		case IDNO:
			break;

		case IDCANCEL:
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * wnd_proc - メインウィンドウプロシージャ
 */
static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static TCHAR app_path[BUF_SIZE];
	TCHAR buf[BUF_SIZE];
	RECT rect;

	switch (msg) {
	case WM_CREATE:
		// EDIT作成
		CreateWindowEx(0, NEDIT_WND_CLASS, TEXT(""),
			WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_NOHIDESEL,
			0, 0, 0, 0,
			hWnd, (HMENU)IDC_EDIT_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

		// INIファイル
		GetModuleFileName(((LPCREATESTRUCT)lParam)->hInstance, app_path, BUF_SIZE - 1);
		if (lstrcmpi(app_path + lstrlen(app_path) - 4, TEXT(".exe")) == 0) {
			lstrcpy(app_path + lstrlen(app_path) - 4, TEXT(".ini"));
		} else {
			lstrcat(app_path, TEXT(".ini"));
		}
		get_ini(hWnd, app_path);

		// コマンドライン
		if (*cmd_line != TEXT('\0') && read_file(hWnd, cmd_line) == TRUE) {
			lstrcpy(file_path, cmd_line);
			wsprintf(buf, TEXT("%s - [%s]"), WINDOW_TITLE, file_path);
			SetWindowText(hWnd, buf);
		}
		break;

	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hWnd, IDC_EDIT_EDIT));
		break;

	case WM_SIZE:
		// ウィンドウ内のEDITのサイズをウィンドウのサイズに合わせる
		GetClientRect(hWnd, &rect);
		MoveWindow(GetDlgItem(hWnd, IDC_EDIT_EDIT), 0, 0,
			rect.right, rect.bottom, TRUE);
		break;

	case WM_QUERYENDSESSION:
		if (save_confirm(hWnd) == FALSE) {
			return FALSE;
		}
		put_ini(hWnd, app_path);
		return TRUE;

	case WM_ENDSESSION:
		if ((BOOL)wParam == FALSE) {
			return 0;
		}
		DestroyWindow(hWnd);
		return 0;

	case WM_CLOSE:
		if (save_confirm(hWnd) == FALSE) {
			return 0;
		}
		put_ini(hWnd, app_path);
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_DROPFILES:
		if (save_confirm(hWnd) == FALSE) {
			break;
		}
		DragQueryFile((HANDLE)wParam, 0, buf, BUF_SIZE - 1);
		if (read_file(hWnd, buf) == TRUE) {
			lstrcpy(file_path, buf);
			wsprintf(buf, TEXT("%s - [%s]"), WINDOW_TITLE, file_path);
			SetWindowText(hWnd, buf);
		}
		DragFinish((HANDLE)wParam);
		break;

	case WM_INITMENUPOPUP:
		if (LOWORD(lParam) == 1) {
			DWORD st, en;

			EnableMenuItem((HMENU)wParam, ID_MENUITEM_UNDO, (SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_CANUNDO, 0, 0) == TRUE) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem((HMENU)wParam, ID_MENUITEM_REDO, (SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_CANREDO, 0, 0) == TRUE) ? MF_ENABLED : MF_GRAYED);

			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_GETSEL, (WPARAM)&st, (LPARAM)&en);
			EnableMenuItem((HMENU)wParam, ID_MENUITEM_CUT, (st != en) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem((HMENU)wParam, ID_MENUITEM_COPY, (st != en) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem((HMENU)wParam, ID_MENUITEM_DELETE, (st != en) ? MF_ENABLED : MF_GRAYED);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_MENUITEM_NEW:
			if (save_confirm(hWnd) == FALSE) {
				break;
			}
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETTEXT, 0, (LPARAM)TEXT(""));
			SetWindowText(hWnd, WINDOW_TITLE);
			break;

		case ID_MENUITEM_OPEN:
			if (save_confirm(hWnd) == FALSE) {
				break;
			}
			*buf = TEXT('\0');
			if (read_file(hWnd, buf) == TRUE) {
				lstrcpy(file_path, buf);
				wsprintf(buf, TEXT("%s - [%s]"), WINDOW_TITLE, file_path);
				SetWindowText(hWnd, buf);
			}
			break;

		case ID_MENUITEM_SAVE:
			save_file(hWnd, file_path);
			break;

		case ID_MENUITEM_SAVEAS:
			*buf = TEXT('\0');
			if (save_file(hWnd, buf) == TRUE) {
				lstrcpy(file_path, buf);
			}
			break;

		case ID_MENUITEM_CLOSE:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case ID_MENUITEM_UNDO:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_UNDO, 0, 0);
			break;
		
		case ID_MENUITEM_REDO:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_REDO, 0, 0);
			break;

		case ID_MENUITEM_CUT:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_CUT, 0, 0);
			break;

		case ID_MENUITEM_COPY:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_COPY, 0, 0);
			break;

		case ID_MENUITEM_PASTE:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_PASTE, 0, 0);
			break;

		case ID_MENUITEM_DELETE:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_CLEAR, 0, 0);
			break;

		case ID_MENUITEM_SELECT_ALL:
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_SETSEL, 0, -1);
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), EM_SCROLLCARET, 0, 0);
			break;

		case ID_MENUITEM_FONT:
			{
				HFONT hFont;

				if ((hFont = font_select(hWnd)) != NULL) {
					SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
					DeleteObject(hFont);
				}
			}
			break;

		case ID_MENUITEM_WORDWRAP:
			if (SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_GETWORDWRAP, 0, 0) == FALSE) {
				SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETWORDWRAP, TRUE, 0);
				CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), LOWORD(wParam), MF_CHECKED);
			} else {
				SendMessage(GetDlgItem(hWnd, IDC_EDIT_EDIT), WM_SETWORDWRAP, FALSE, 0);
				CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), LOWORD(wParam), MF_UNCHECKED);
			}
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * init_application - ウィンドウクラスの登録
 */
static BOOL init_application(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	wc.lpfnWndProc = (WNDPROC)wnd_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
	wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
	wc.lpszClassName = MAIN_WND_CLASS;
	// ウィンドウクラスの登録
	return RegisterClass(&wc);
}

/*
 * init_instance - ウィンドウの作成
 */
static HWND init_instance(HINSTANCE hInstance, int CmdShow)
{
	HWND hWnd =  NULL;

	// ウィンドウの作成
	hWnd = CreateWindowEx(WS_EX_ACCEPTFILES,
		MAIN_WND_CLASS,
		WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) {
		return NULL;
	}

	// ウィンドウの表示
	ShowWindow(hWnd, CmdShow);
	UpdateWindow(hWnd);
	return hWnd;
}

/*
 * WinMain - メイン
 */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HANDLE hAccel;
	HWND hWnd;

	hInst = hInstance;

    if (*lpCmdLine == TEXT('\"')) {
		TCHAR *p;
		lstrcpy(cmd_line, lpCmdLine + 1);
		for (p = cmd_line; *p != TEXT('\0') && *p != TEXT('\"'); p++)
			;
		if (*p == TEXT('\"')) {
			*p = TEXT('\0');
		}
	} else {
		lstrcpy(cmd_line, lpCmdLine);
	}

	// EDIT登録
	if (RegisterNedit(hInstance) == FALSE) {
		return 0;
	}
	// ウィンドウクラスの登録
	if (init_application(hInstance) == FALSE) {
		return 0;
	}
	// ウィンドウの作成
	if ((hWnd = init_instance(hInstance, nCmdShow)) == NULL) {
		return 0;
	}

	// リソースからアクセラレータをロード
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	// ウィンドウメッセージ処理
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}
/* End of source */
