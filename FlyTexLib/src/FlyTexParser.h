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
	ERROR_REMOVE_TEMP,
	ERROR_CLIPBOARD_OPEN,
	ERROR_CLIPBOARD_PASTE
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
		return "ERROR_LATEX_PARSE";
		break;
	case ERROR_DVIPNG:
		return "ERROR_DVIPNG";
		break;
	case ERROR_REMOVE_TEMP:
		return "ERROR_REMOVE_TEMP";
		break;
	case ERROR_CLIPBOARD_OPEN:
		return "ERROR_CLIPBOARD_OPEN";
		break;
	case ERROR_CLIPBOARD_PASTE:
		return "ERROR_CLIPBOARD_PASTE";
		break;
	default:
		return "Unknown error";
		break;
	}
}

std::wstring Utf8ToUtf16(const std::string& str);
std::string Utf16ToUtf8(const std::wstring& str);

DWORD ExecuteCommand(const std::string& command, bool showConsole = false);

std::string MakeFullPath(const std::string& folder, const std::string& filename, const std::string& ext);
void MakeFileFromTemplate(const std::wstring& templatePath, const std::wstring& outPath, const std::wstring& pattern, const std::wstring& replacement);

bool RemoveDirectoryRecursive(const std::string& dirPath);

class FlyTexParser
{
public:
	FlyTexParser(const std::string& templateFile, const std::string& latexPath = "latex", const std::string& dvipngPath = "dvipng");
	~FlyTexParser();

	void SetResolution(int res);
	void SetBackgroundColor(std::string color);
	void SetForegroundColor(std::string color);

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
