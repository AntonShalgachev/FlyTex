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
	hImgCheck = GetDlgItem(hWnd, IDC_CHECKIMAGE);
	hOutFile = GetDlgItem(hWnd, IDC_OUTIMAGE);

	SetWindowText(hOutFile, FLYTEX_DEFAULT_IMAGE_FILE);

	LoadSettings();
	ApplySettings();

	//parser.SetTemplateFile(FLYTEX_DEFAULT_TEMPLATE_FILE);

	Gdiplus::GdiplusStartupInput gdiInput;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, NULL);

	UpdateTemplateStatus();

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
	HBITMAP hBmp;
	if(parsingToFile)
	{
		Gdiplus::Bitmap* png = new Gdiplus::Bitmap(Utf8ToUtf16(outFile).c_str());

		HBITMAP hBmpTmp;
		png->GetHBITMAP(Gdiplus::Color::White, &hBmpTmp);
		hBmp = (HBITMAP)CopyImage(hBmpTmp, IMAGE_BITMAP, 0, 0, NULL);

		delete png;
		png = nullptr;
	}
	else
	{
		if(IsClipboardFormatAvailable(CF_BITMAP))
		{
			if(OpenClipboard(hWnd))
			{
				HBITMAP hBmpTmp = (HBITMAP)GetClipboardData(CF_BITMAP);
				hBmp = (HBITMAP)CopyImage(hBmpTmp, IMAGE_BITMAP, 0, 0, 0);
				CloseClipboard();
			}
			else
			{
				UpdateStatus(FLYTEX_STATUS_ERROR, L"Clipboard error");
				return;
			}
		}
		else
		{
			UpdateStatus(FLYTEX_STATUS_ERROR, L"Clipboard error");
			return;
		}
	}

	RECT previewRc;
	GetWindowRect(hPreview, &previewRc);
	LONG wndWidth = previewRc.right - previewRc.left;
	LONG wndHeight = previewRc.bottom - previewRc.top;

	BITMAP bmp;
	GetObject(hBmp, sizeof(BITMAP), &bmp);
	LONG imgWidth = bmp.bmWidth;
	LONG imgHeight = bmp.bmHeight;

	if(imgWidth > wndWidth || imgHeight > wndHeight)
	{
		double scaleFactor = max((double)imgWidth / (double)wndWidth, (double)imgHeight / (double)wndHeight);

		imgWidth = static_cast<LONG>(imgWidth / scaleFactor);
		imgHeight = static_cast<LONG>(imgHeight / scaleFactor);
	}

	HBITMAP hBmpScaled = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, imgWidth, imgHeight, 0);

	SendMessage(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpScaled);

	DeleteObject(hBmpScaled);
	DeleteObject(hBmp);
}

void FlyTexApp::LoadSettings()
{
	//BOOL res = GetPrivateProfileStruct(FLYTEX_APPNAME, FLYTEX_SETTINGS, &settings, sizeof(settings), FLYTEX_SETTINGS_FILE);

	std::ifstream f(FLYTEX_SETTINGS_FILE);
	if(f.is_open())
	{
		std::string resolutionStr;
		std::getline(f, settings.background);
		std::getline(f, settings.foreground);
		std::getline(f, resolutionStr);
		std::getline(f, settings.templatePath);

		std::stringstream ss;
		ss << resolutionStr;
		ss >> settings.resolution;

		f.close();
	}
	else
	{
		settings.background = FLYTEX_DEFAULT_BACKGROUND;
		settings.foreground = FLYTEX_DEFAULT_FOREGROUND;
		settings.resolution = FLYTEX_DEFAULT_RESOLUTION;
		settings.templatePath = "";

		SaveSettings();
	}
}

void FlyTexApp::SaveSettings()
{	
	//BOOL res = WritePrivateProfileStringW(FLYTEX_APPNAME, FLYTEX_SETTINGS_BACKGROUND, Utf8ToUtf16(settings.background).c_str(), FLYTEX_SETTINGS_FILE);
	//res = WritePrivateProfileStringW(FLYTEX_APPNAME, FLYTEX_SETTINGS_FOREGROUND, Utf8ToUtf16(settings.foreground).c_str(), FLYTEX_SETTINGS_FILE);

	//res = WritePrivateProfileStruct(FLYTEX_APPNAME, FLYTEX_SETTINGS, &settings, sizeof(settings), FLYTEX_SETTINGS_FILE);
	std::ofstream f(FLYTEX_SETTINGS_FILE);

	f << settings.background << std::endl;
	f << settings.foreground << std::endl;
	f << settings.resolution << std::endl;
	f << settings.templatePath << std::endl;

	f.close();
}

void FlyTexApp::ApplySettings()
{
	parser.SetBackgroundColor(settings.background);
	parser.SetForegroundColor(settings.foreground);
	parser.SetResolution(settings.resolution);
	parser.SetTemplateFile(settings.templatePath);
}

void FlyTexApp::UpdateTemplateStatus()
{
	if(settings.templatePath.empty())
	{
		UpdateStatus(FLYTEX_STATUS_NOTEMPLATE);
		EnableWindow(hEdit, FALSE);
	}
	else
	{
		UpdateStatus(FLYTEX_STATUS_READY);
		EnableWindow(hEdit, TRUE);
	}
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
		case ID_FLYTEX_SETTINGS:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsProc, reinterpret_cast<LPARAM>(&settings));
			SetFocus(hWnd);
			SaveSettings();
			ApplySettings();
			UpdateTemplateStatus();
			return TRUE;
			break;
		case ID_FLYTEX_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case IDC_CHECKIMAGE:
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				parsingToFile = (SendMessage(hImgCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
				EnableWindow(hOutFile, parsingToFile);
				return TRUE;
				break;
			}
			break;
		case IDC_OUTIMAGE:
			switch(HIWORD(wParam))
			{
			case EN_UPDATE:
				int len = GetWindowTextLengthW(hOutFile);

				WCHAR* path = new WCHAR[len + 1];

				path[len] = 0;
				GetWindowTextW(hOutFile, path, len + 1);
				std::wstring pathStr(path);

				delete path;
				path = nullptr;

				outFile = Utf16ToUtf8(pathStr);

				return TRUE;
				break;
			}
			break;
		}
		switch(HIWORD(wParam))
		{
		case EN_UPDATE:
			switch(LOWORD(wParam))
			{
			case IDC_LATEX:
				SetTimer(hWnd, FLYTEX_TIMER_COMPILE, FLYTEX_COMPILE_DELAY, NULL);
				return TRUE;
				break;
			}
			break;
		}
		break;

	case WM_TIMER:
		switch(wParam)
		{
		case FLYTEX_TIMER_COMPILE:
			if(waitingForFuture)
				return TRUE;

			KillTimer(hWnd, FLYTEX_TIMER_COMPILE);

			expLen = GetWindowTextLengthW(hEdit);
			if(expLen > 0)
			{
				expression = new WCHAR[expLen + 1];
				expression[expLen] = 0;
				GetWindowTextW(hEdit, expression, expLen + 1);

				expressionStr = Utf16ToUtf8(std::wstring(expression));

				UpdateStatus(FLYTEX_STATUS_COMPILING);

				if(parsingToFile)
				{
					parserFuture = std::async(std::launch::async, &FlyTexParser::ParseToImage, &parser, expressionStr, outFile, true);
				}
				else
				{
					parserFuture = std::async(std::launch::async, &FlyTexParser::ParseToClipboard, &parser, expressionStr, true);
				}
				waitingForFuture = true;
				SetTimer(hWnd, FLYTEX_TIMER_CHECK_FUTURE, FLYTEX_FUTURE_CHECK_DELAY, NULL);
			}
			else
			{
				//UpdateStatus(FLYTEX_STATUS_READY);
			}

			return TRUE;

			break;
		case FLYTEX_TIMER_CHECK_FUTURE:
			if(parserFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				KillTimer(hWnd, FLYTEX_TIMER_CHECK_FUTURE);
				waitingForFuture = false;

				Error result = parserFuture.get();

				if(result == ERROR_OK)
				{
					if(parsingToFile)
					{
						UpdateStatus(FLYTEX_STATUS_WRITTEN, Utf8ToUtf16(outFile));
					}
					else
					{
						UpdateStatus(FLYTEX_STATUS_COPIED);
					}
					UpdatePreview();
				}
				else
				{
					std::string errorStr = ErrorString(result);
					UpdateStatus(FLYTEX_STATUS_ERROR, Utf8ToUtf16(errorStr));
				}
			}
			return TRUE;
			break;
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK FlyTexApp::SettingsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static FlyTexSettings* settings = nullptr;
	HWND hBack = GetDlgItem(hWnd, IDC_BACKCOLOR);
	HWND hFore = GetDlgItem(hWnd, IDC_FORECOLOR);
	HWND hRes = GetDlgItem(hWnd, IDC_RESOLUTION);
	HWND hTemplate = GetDlgItem(hWnd, IDC_TEMPLATE);
	std::wstringstream ss;

	switch(msg)
	{
	case WM_INITDIALOG:
		settings = reinterpret_cast<FlyTexSettings*>(lParam);
		SetWindowTextW(hBack, Utf8ToUtf16(settings->background).c_str());
		SetWindowTextW(hFore, Utf8ToUtf16(settings->foreground).c_str());
		SetWindowTextW(hTemplate, Utf8ToUtf16(settings->templatePath).c_str());
		ss << settings->resolution;
		SetWindowTextW(hRes, ss.str().c_str());
		break;
	case WM_DESTROY:
		EndDialog(hWnd, 0);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case IDC_CHOOSETEMPLATE:
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				const int maxPathLength = 2048;
				TCHAR path[maxPathLength];
				path[0] = 0;
				OPENFILENAME ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = path;
				ofn.lpstrFilter = TEXT("LaTeX Files (.tex)\0*.tex\0All Files\0*.*\0");
				ofn.nMaxFile = maxPathLength;
				ofn.Flags = OFN_EXPLORER;
				GetOpenFileName(&ofn);
				SetWindowText(hTemplate, path);

				break;
			}
			break;
		case IDOK:
			int backLen = GetWindowTextLengthW(hBack);
			int foreLen = GetWindowTextLengthW(hFore);
			int resLen = GetWindowTextLengthW(hRes);
			int templateLen = GetWindowTextLengthW(hTemplate);

			WCHAR* backColor = new WCHAR[backLen + 1];
			WCHAR* foreColor = new WCHAR[foreLen + 1];
			WCHAR* resolution = new WCHAR[resLen + 1];
			WCHAR* templateFile = new WCHAR[templateLen + 1];

			GetWindowText(hBack, backColor, backLen + 1);
			GetWindowText(hFore, foreColor, foreLen + 1);
			GetWindowText(hRes, resolution, resLen + 1);
			GetWindowText(hTemplate, templateFile, templateLen + 1);

			std::wstring backColorStr(backColor);
			std::wstring foreColorStr(foreColor);
			std::wstring resolutionStr(resolution);
			std::wstring templateFileStr(templateFile);

			delete backColor;
			delete foreColor;
			delete resolution;
			delete templateFile;

			int res;

			std::wstringstream ss;
			ss << resolutionStr;
			ss >> res;

			settings->background = Utf16ToUtf8(backColorStr);
			settings->foreground = Utf16ToUtf8(foreColorStr);
			settings->templatePath = Utf16ToUtf8(templateFileStr);
			if(res > 0)
			{
				settings->resolution = res;
				SendMessage(hWnd, WM_CLOSE, 0, 0);
			}
			else
			{
				MessageBox(hWnd, TEXT("Resolution should be greater than zero"), TEXT("Error"), MB_OK | MB_ICONERROR);
			}
			break;
		}
		break;
	}
	return FALSE;
}
