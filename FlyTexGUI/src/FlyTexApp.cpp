#include "FlyTexApp.h"

std::map<HWND, FlyTexApp*> FlyTexApp::procMap;

FlyTexApp::FlyTexApp(HINSTANCE hInst) : hInst(hInst), hWnd(NULL)
{
	if(!Init())
	{
		std::stringstream ss;
		ss << GetLastError();
		throw std::runtime_error(ss.str());
	}
}

bool FlyTexApp::Init()
{
	InitCommonControls();
	LoadLibrary(TEXT("RichEd20.dll"));

	hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProcStatic, reinterpret_cast<LPARAM>(this));

	hEdit = GetDlgItem(hWnd, IDC_LATEX);
	hPreview = GetDlgItem(hWnd, IDC_PREVIEW);
	hStatus = GetDlgItem(hWnd, IDC_STATUS);

	UpdateStatus(TEXT("Ready"));

	return hWnd != NULL;
}

void FlyTexApp::Show(int cmdShow) const
{
	ShowWindow(hWnd, cmdShow);
}

WPARAM FlyTexApp::Run() const
{
	MSG msg;
	while(GetMessage(&msg, NULL, NULL, NULL) > 0)
	{
		if(!IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

void FlyTexApp::UpdateStatus(LPCTSTR status) const
{
	LPCTSTR base = TEXT("Status: ");
	
	size_t baseLength;
	StringCchLength(base, STRSAFE_MAX_CCH, &baseLength);
	size_t statusLength;
	StringCchLength(status, STRSAFE_MAX_CCH, &statusLength);

	TCHAR* fullStatus = new TCHAR[baseLength + statusLength + 1];

	StringCchPrintf(fullStatus, baseLength + statusLength + 1, TEXT("%s%s"), base, status);
	SetWindowText(hStatus, fullStatus);

	delete[] fullStatus;
}

INT_PTR CALLBACK FlyTexApp::DlgProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_INITDIALOG)
	{
		FlyTexApp* instance = reinterpret_cast<FlyTexApp*>(lParam);
		procMap[hWnd] = instance;

		return TRUE;
	}
	else
	{
		if(procMap.find(hWnd) != procMap.end())
		{
			FlyTexApp* instance = procMap[hWnd];
			return instance->DlgProc(hWnd, msg, wParam, lParam);
		}

		return FALSE;
	}
}

INT_PTR CALLBACK FlyTexApp::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return TRUE;
		break;

	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		return TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return TRUE;
			break;
		}
		break;
	}

	return FALSE;
}
