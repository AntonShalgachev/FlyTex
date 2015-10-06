#pragma once

#include "../../FlyTexLib/src/FlyTexParser.h"
#include "../res/resource.h"

#include <Windows.h>
#include <CommCtrl.h>

#include <map>
#include <sstream>
#include <future>

#pragma comment(lib, "ComCtl32.lib")

#pragma comment(linker, \
	"\"/manifestdependency:type='Win32' "\
	"name='Microsoft.Windows.Common-Controls' "\
	"version='6.0.0.0' "\
	"processorArchitecture='*' "\
	"publicKeyToken='6595b64144ccf1df' "\
	"language='*'\"")

#ifdef _DEBUG
#pragma comment(lib, "../Debug/FlyTexLib.lib")
#else
#pragma comment(lib, "../Release/FlyTexLib.lib")
#endif

#define FLYTEX_TIMER_COMPILE 1
#define FLYTEX_TIMER_CHECK_FUTURE 2

#define FLYTEX_STATUS_READY L"Ready"
#define FLYTEX_STATUS_COPIED L"Copied to clipboard"
#define FLYTEX_STATUS_COMPILING L"Parsing..."
#define FLYTEX_STATUS_ERROR L"Error "

#define FLYTEX_COMPILE_DELAY 500
#define FLYTEX_FUTURE_CHECK_DELAY USER_TIMER_MINIMUM

#define FLYTEX_DEFAULT_TEMPLATE_FILE "template.tex"
#define FLYTEX_DEFAULT_BACKGROUND "Transparent"

class FlyTexApp
{
public:
	FlyTexApp(HINSTANCE hInst);

	void Show(int cmdShow) const;
	WPARAM Run() const;

	void UpdateStatus(LPCWSTR status, const std::wstring& detailStr = L"") const;
	void UpdatePreview() const;

private:
	bool Init();

private:
	static INT_PTR CALLBACK DlgProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE hInst;
	HWND hWnd;

	HWND hEdit;
	HWND hPreview;
	HWND hStatus;

	FlyTexParser parser;
	std::future<Error> parserFuture;

	static std::map<HWND, FlyTexApp*> procMap;
};
