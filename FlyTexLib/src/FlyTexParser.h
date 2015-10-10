#pragma once

#include <string>
#include <Windows.h>
#include <strsafe.h>
#include <locale>
#include <codecvt>
#include <fstream>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define TMP_FILENAME	"tmp"
#define TMP_FOLDER		".temp"
#define LOG_FOLDER		".log"

#define MAX_STR_LENGTH 1024

#define DEFAULT_RESOLUTION 600
#define DEFAULT_BACKGROUND_COLOR "White"
#define DEFAULT_FOREGROUND_COLOR "Black"

enum Error
{
	ERROR_OTHER,
	ERROR_OK,
	ERROR_LATEX_PARSE,
	ERROR_DVIPNG,
	ERROR_CREATE_TEMP,
	ERROR_CREATE_LOG,
	ERROR_REMOVE_TEMP,
	ERROR_CLIPBOARD_OPEN,
	ERROR_CLIPBOARD_PASTE,
	ERROR_IO_TEMPLATE_FILE,
	ERROR_IO_OUT_FILE
};

inline std::string ErrorString(Error err)
{
	switch(err)
	{
	case ERROR_OTHER:
		return "ERROR_OTHER";
		break;
	case ERROR_OK:
		return "ERROR_OK";
		break;
	case ERROR_LATEX_PARSE:
		return "LaTeX error";
		break;
	case ERROR_DVIPNG:
		return "dvipng error";
		break;
	case ERROR_CREATE_TEMP:
		return "failed to create temp folder";
		break;
	case ERROR_CREATE_LOG:
		return "failed to create log folder";
		break;
	case ERROR_REMOVE_TEMP:
		return "failed to remove temp folder";
		break;
	case ERROR_CLIPBOARD_OPEN:
		return "failed to open clipboard";
		break;
	case ERROR_CLIPBOARD_PASTE:
		return "failed to paste to clipboard";
		break;
	case ERROR_IO_TEMPLATE_FILE:
		return "failed to open template file";
		break;
	case ERROR_IO_OUT_FILE:
		return "failed to open out file";
		break;
	default:
		return "unknown error";
		break;
	}
}

std::wstring Utf8ToUtf16(const std::string& str);
std::string Utf16ToUtf8(const std::wstring& str);

DWORD ExecuteCommand(const std::string& command, bool showConsole = false);

std::string MakeFullPath(const std::string& folder, const std::string& filename, const std::string& ext);
Error MakeFileFromTemplate(const std::wstring& templatePath, const std::wstring& outPath, const std::wstring& pattern, const std::wstring& replacement);

bool RemoveDirectoryRecursive(const std::string& dirPath);

class FlyTexParser
{
public:
	FlyTexParser();
	~FlyTexParser();

	void SetTemplateFile(const std::string& path);
	void SetExecutablesPath(const std::string& latexPath, const std::string& dvipngPath);

	void SetResolution(int res);
	void SetBackgroundColor(const std::string& color);
	void SetForegroundColor(const std::string& color);

	bool IsLatexInstalled() const;

	Error ParseToImage(const std::string& expression, const std::string& imageFile, bool deleteTemp = true) const;
	Error ParseToClipboard(const std::string& expression) const;

private:
	std::string templateFile;
	std::string latexPath;
	std::string dvipngPath;

	int resolution;
	std::string backgroundColor;
	std::string foregroundColor;

	ULONG_PTR gdiToken;
};
