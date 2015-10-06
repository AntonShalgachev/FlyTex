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

	parser.SetTemplateFile(FLYTEX_DEFAULT_TEMPLATE_FILE);
	parser.SetBackgroundColor(FLYTEX_DEFAULT_BACKGROUND);

	UpdateStatus(FLYTEX_STATUS_READY);

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

void FlyTexApp::UpdateStatus(LPCWSTR status, const std::wstring& detailStr) const
{
	LPCTSTR base = L"Status: ";
	
	size_t baseLength;
	StringCchLengthW(base, STRSAFE_MAX_CCH, &baseLength);
	size_t statusLength;
	StringCchLengthW(status, STRSAFE_MAX_CCH, &statusLength);

	size_t detailLen = detailStr.size();

	TCHAR* fullStatus = new TCHAR[baseLength + statusLength + detailLen + 1];

	StringCchPrintfW(fullStatus, baseLength + statusLength + detailLen + 1, L"%s%s%s", base, status, detailStr.c_str());
	SetWindowTextW(hStatus, fullStatus);

	delete[] fullStatus;
}

void FlyTexApp::UpdatePreview() const
{
	if(IsClipboardFormatAvailable(CF_BITMAP))
	{
		if(OpenClipboard(hWnd))
		{
			HBITMAP hBmp = (HBITMAP)GetClipboardData(CF_BITMAP);
			CloseClipboard();
			HBITMAP hBmpCopy = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, 0, 0, 0);
			LRESULT res = SendMessage(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpCopy);

			CloseClipboard();

			return;
		}
	}

	UpdateStatus(FLYTEX_STATUS_ERROR, L"Clipboard error");
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
	int expLen;
	WCHAR* expression;
	std::string expressionStr;
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
		switch(HIWORD(wParam))
		{
		case EN_UPDATE:
			switch(LOWORD(wParam))
			{
			case IDC_LATEX:
				SetTimer(hWnd, FLYTEX_TIMER_COMPILE, FLYTEX_COMPILE_DELAY, NULL);
				break;
			}
			break;
		}
		break;

	case WM_TIMER:
		switch(wParam)
		{
		case FLYTEX_TIMER_COMPILE:
			KillTimer(hWnd, FLYTEX_TIMER_COMPILE);

			expLen = GetWindowTextLengthW(hEdit);
			if(expLen > 0)
			{
				expression = new WCHAR[expLen + 1];
				expression[expLen] = 0;
				GetWindowTextW(hEdit, expression, expLen + 1);

				expressionStr = Utf16ToUtf8(std::wstring(expression));

				UpdateStatus(FLYTEX_STATUS_COMPILING);

				parserFuture = std::async(std::launch::async, &FlyTexParser::ParseToClipboard, &parser, expressionStr);

				SetTimer(hWnd, FLYTEX_TIMER_CHECK_FUTURE, FLYTEX_FUTURE_CHECK_DELAY, NULL);
			}
			else
			{
				//UpdateStatus(FLYTEX_STATUS_READY);
			}

			break;
		case FLYTEX_TIMER_CHECK_FUTURE:
			if(parserFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				KillTimer(hWnd, FLYTEX_TIMER_CHECK_FUTURE);

				Error result = parserFuture.get();

				if(result == ERROR_OK)
				{
					UpdateStatus(FLYTEX_STATUS_COPIED);
					UpdatePreview();
				}
				else
				{
					std::string errorStr = ErrorString(result);
					UpdateStatus(FLYTEX_STATUS_ERROR, Utf8ToUtf16(errorStr));
				}
			}
			break;
		}
		break;
	}

	return FALSE;
}
