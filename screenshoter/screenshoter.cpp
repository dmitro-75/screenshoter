// screenshoter.cpp : main source file for screenshoter.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"


#include "MainDlg.h"
#include "wic.h"
#include "atlcoll.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int ExecuteByParams(LPTSTR lpstrCmdLine);

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;

	if (ExecuteByParams(lpstrCmdLine))
	{

		AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

		hRes = _Module.Init(NULL, hInstance);
		ATLASSERT(SUCCEEDED(hRes));

		nRet = Run(lpstrCmdLine, nCmdShow);
		_Module.Term();
	}

	::CoUninitialize();

	return nRet;
}

#define MESSAGE_CAPTURE 0
#define SCREEN_CAPTURE 1

int ExecuteByParams(LPTSTR lpstrCmdLine)
{
	DWORD pid = 0;
	WCHAR FileName[MAX_PATH] = {0};
	int Capture = SCREEN_CAPTURE;

	int nArgs;
	LPWSTR *pArgs = CommandLineToArgvW(lpstrCmdLine, &nArgs);

	for (int i = 0; i < nArgs; i++)
	{
		LPWSTR arg = pArgs[i];
		if (!wcscmp(arg, L"-m"))
			Capture = MESSAGE_CAPTURE;
		else if (!wcscmp(arg, L"-s"))
			Capture = SCREEN_CAPTURE;
		else if (!wcsncmp(arg, L"-p", 2))
			pid = _wtoi(arg + 2);
		else if (!wcsncmp(arg, L"-f", 2))
			wcscpy_s(FileName, arg + 2);
	}

	LocalFree(pArgs);

	if (!*FileName)
		return 1;

	HWND hMain;
	CAtlArray<HWND> moduls;
	if (pid)
		for (hMain = GetWindow(GetDesktopWindow(), GW_CHILD); hMain; hMain = GetWindow(hMain, GW_HWNDNEXT))
		{
			LONG style = GetWindowLong(hMain, GWL_STYLE);

			if (!(style & WS_VISIBLE))
				continue;

			if (style & WS_POPUP)
			{
				moduls.Add(hMain);
				continue;
			}

			DWORD id;
			GetWindowThreadProcessId(hMain, &id);
			if (id == pid)
				break;
		}
	else
	{
		hMain = GetDesktopWindow();
		Capture = SCREEN_CAPTURE;
	}
	
	if (!hMain)
		return 2;

	BOOL b;
	b = ShowWindow(hMain, SW_SHOWMAXIMIZED);
	RECT rect;
	b = GetWindowRect(hMain, &rect);
	int dx = -rect.left;
	int dy = -rect.top;
	InflateRect(&rect, -dx, -dy);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	
	HDC hScreenDC = GetDC(NULL);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	if (Capture == SCREEN_CAPTURE)
	{
		b = SetForegroundWindow(hMain);
		Sleep(200);
		b = BitBlt(hMemoryDC, 0, 0, rect.right, rect.bottom, hScreenDC, 0, 0, SRCCOPY);
	}
	else if (Capture == MESSAGE_CAPTURE)
	{
		b = SetWindowOrgEx(hMemoryDC, dx, dy, NULL);
		b = PrintWindow(hMain, hMemoryDC, 0);
		for (UINT i = 0; i < moduls.GetCount(); ++i)
		{
			GetWindowRect(moduls[i], &rect);
			b = SetWindowOrgEx(hMemoryDC, -rect.left, -rect.top, NULL);
			b = PrintWindow(moduls[i], hMemoryDC, 0);
		}
	}

	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);
	
	HRESULT hr;

	hr = WriteBitmap(hBitmap, FileName);
	
	DeleteObject(hBitmap);

	if (FAILED(hr))
		return 4;

	return 0;
}