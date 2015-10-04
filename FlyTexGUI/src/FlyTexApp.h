#pragma once

#include "../../FlyTexLib/src/FlyTexParser.h"
#include "../res/resource.h"

#include <Windows.h>
#include <CommCtrl.h>

#include <map>
#include <sstream>
#include <thread>

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

#define FLYTEX_APP_NAME TEXT("FlyTex")
#define FLYTEX_TITLE TEXT("FlyTex")

class FlyTexApp
{
public:
	FlyTexApp(HINSTANCE hInst);

	void Show(int cmdShow) const;
	WPARAM Run() const;

	void UpdateStatus(LPCTSTR status) const;

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

	static std::map<HWND, FlyTexApp*> procMap;
};
