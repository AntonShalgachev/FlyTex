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
#define FLYTEX_STATUS_NOTEMPLATE L"No template file specified. Set one in Settings dialog"
#define FLYTEX_STATUS_COPIED L"Copied to clipboard"
#define FLYTEX_STATUS_WRITTEN L"Written to "
#define FLYTEX_STATUS_COMPILING L"Parsing..."
#define FLYTEX_STATUS_ERROR L"Error "

#define FLYTEX_COMPILE_DELAY 500
#define FLYTEX_FUTURE_CHECK_DELAY USER_TIMER_MINIMUM

#define FLYTEX_DEFAULT_TEMPLATE_FILE "template.tex"
#define FLYTEX_DEFAULT_BACKGROUND "Transparent"
#define FLYTEX_DEFAULT_FOREGROUND "Black"
#define FLYTEX_DEFAULT_RESOLUTION 600
#define FLYTEX_DEFAULT_IMAGE_FILE TEXT("out.png")

#define FLYTEX_SETTINGS_FILE L"FlyTex.cfg"

struct FlyTexSettings
{
	std::string background;
	std::string foreground;
	int resolution;
	std::string templatePath;
	bool debugMode;
};

class FlyTexApp
{
public:
	FlyTexApp(HINSTANCE hInst);

	void Show(int cmdShow) const;
	WPARAM Run() const;

	void UpdateStatus(LPCWSTR status, const std::wstring& detailStr = L"") const;
	void UpdatePreview() const;

	void LoadSettings();
	void SaveSettings();
	void ApplySettings();

	void UpdateTemplateStatus();

private:
	bool Init();

private:
	static INT_PTR CALLBACK DlgProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR CALLBACK SettingsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE hInst;
	HWND hWnd;

	HWND hEdit;
	HWND hPreview;
	HWND hStatus;
	HWND hImgCheck;
	HWND hOutFile;

	FlyTexParser parser;
	std::future<Error> parserFuture;
	bool waitingForFuture;

	bool parsingToFile;
	std::string outFile;
	FlyTexSettings settings;

	ULONG_PTR gdiToken;

	static std::map<HWND, FlyTexApp*> procMap;
};
