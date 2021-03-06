#include "FlyTexParser.h"

std::wstring Utf8ToUtf16(const std::string& str)
{
	std::wstring_convert< std::codecvt_utf8<wchar_t> > converter;
	return converter.from_bytes(str);
}

std::string Utf16ToUtf8(const std::wstring& str)
{
	std::wstring_convert< std::codecvt_utf8<wchar_t> > converter;
	return converter.to_bytes(str);
}

DWORD ExecuteCommand(const std::string& command, bool showConsole)
{
	WCHAR sysFolder[MAX_STR_LENGTH];
	WCHAR cmdPath[MAX_STR_LENGTH];

	GetSystemDirectoryW(sysFolder, MAX_STR_LENGTH);
	StringCchPrintfW(cmdPath, MAX_STR_LENGTH, TEXT("%s\\cmd.exe"), sysFolder);

	std::wstring cmdArgs = L"/C \"" + Utf8ToUtf16(command) + L'"';
	//std::wstring cmdArgs = L"/C " + Utf8ToUtf16(command);

	//std::string arguments = "/C " + command;
	//tstring cmdArgs = CStrToTStr(arguments.c_str());

	PROCESS_INFORMATION pInfo;
	STARTUPINFOW startupInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	DWORD creationFlags;
	if(showConsole)
		creationFlags = CREATE_NEW_CONSOLE;
	else
		creationFlags = CREATE_NO_WINDOW;

	TCHAR* cmdArgsTchar = new TCHAR[cmdArgs.size() + 1];
	StringCchCopyW(cmdArgsTchar, cmdArgs.size()+1, cmdArgs.c_str());

	BOOL res = CreateProcessW(cmdPath, cmdArgsTchar, NULL, NULL, FALSE, creationFlags, NULL, NULL, &startupInfo, &pInfo);

	delete[] cmdArgsTchar;

	if(res)
	{
		WaitForSingleObject(pInfo.hProcess, INFINITE);

		DWORD exitCode;
		GetExitCodeProcess(pInfo.hProcess, &exitCode);

		CloseHandle(pInfo.hProcess);
		CloseHandle(pInfo.hThread);

		return exitCode;
	}
	else
	{
		throw std::runtime_error("Failed to create a proccess");
	}

	return -1;
}

std::string MakeFullPath(const std::string& folder, const std::string& filename, const std::string& ext)
{
	std::wstring path = Utf8ToUtf16(folder) + L'\\' + Utf8ToUtf16(filename) + L'.' + Utf8ToUtf16(ext);
	return Utf16ToUtf8(path);
}

Error MakeFileFromTemplate(const std::string& templatePath, const std::string& outPath, const std::string& pattern, const std::string& replacement)
{
	std::ifstream templateFile;
	std::ofstream outFile;

	templateFile.open(Utf8ToUtf16(templatePath));
	outFile.open(Utf8ToUtf16(outPath));

	if(!templateFile.is_open())
		return ERROR_IO_TEMPLATE_FILE;
	if(!outFile.is_open())
		return ERROR_IO_OUT_FILE;

	std::string dataUTF8((std::istreambuf_iterator<char>(templateFile)), std::istreambuf_iterator<char>());
	std::wstring dataUTF16 = Utf8ToUtf16(dataUTF8);

	std::wstring patternUtf16 = Utf8ToUtf16(pattern);
	std::wstring replacementUtf16 = Utf8ToUtf16(replacement);

	size_t pos = dataUTF16.find(patternUtf16);
	if(pos != std::wstring::npos)
		dataUTF16.replace(pos, patternUtf16.length(), replacementUtf16);

	dataUTF8 = Utf16ToUtf8(dataUTF16);

	outFile.write(dataUTF8.c_str(), dataUTF8.size());

	outFile.close();
	templateFile.close();

	return ERROR_OK;
}

bool RemoveDirectoryRecursive(const std::string& dirPath)
{
	std::wstring dirPathUtf16 = Utf8ToUtf16(dirPath);

	//LPWSTR dirPathTerm = new WCHAR[dirPath.size() + 2];
	//dirPathTerm[dirPath.size()] = 0;
	//dirPathTerm[dirPath.size() + 1] = 0;
	//StringCchCopyW(dirPathTerm, dirPath.size() + 2, dirPathUtf16.c_str());

	dirPathUtf16.push_back(L'\0');

	SHFILEOPSTRUCTW fileOp;
	fileOp.hwnd = NULL;
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = dirPathUtf16.c_str();
	fileOp.pTo = NULL;
	fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	fileOp.fAnyOperationsAborted = FALSE;
	fileOp.hNameMappings = NULL;
	fileOp.lpszProgressTitle = NULL;

	int ret = SHFileOperationW(&fileOp);

	return (ret == 0) && (fileOp.fAnyOperationsAborted == FALSE);
}

FlyTexParser::FlyTexParser()
{
	SetTemplateFile("");
	SetExecutablesPath("latex", "dvipng");

	SetResolution(DEFAULT_RESOLUTION);
	SetBackgroundColor(DEFAULT_BACKGROUND_COLOR);
	SetForegroundColor(DEFAULT_FOREGROUND_COLOR);

	if(!IsLatexInstalled())
	{
		throw std::invalid_argument("Couldn't find valid latex or dvipng executable");
	}

	Gdiplus::GdiplusStartupInput gdiInput;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, NULL);
}

void FlyTexParser::SetTemplateFile(const std::string& path)
{
	this->templateFile = path;
}

void FlyTexParser::SetExecutablesPath(const std::string& latexPath, const std::string& dvipngPath)
{
	this->latexPath = '"' + latexPath + '"';
	this->dvipngPath = '"' + dvipngPath + '"';
}

FlyTexParser::~FlyTexParser()
{
	Gdiplus::GdiplusShutdown(gdiToken);
}

void FlyTexParser::SetResolution(int res)
{
	this->resolution = res;
}

void FlyTexParser::SetBackgroundColor(const std::string& color)
{
	backgroundColor = color;
}

void FlyTexParser::SetForegroundColor(const std::string& color)
{
	foregroundColor = color;
}

bool FlyTexParser::IsLatexInstalled() const
{
	bool latexExists = ExecuteCommand(latexPath + " -version") == EXIT_SUCCESS;
	bool dvipngExists = ExecuteCommand(dvipngPath + " -version") == EXIT_SUCCESS;
	return latexExists && dvipngExists;
}

Error FlyTexParser::ParseToImage(const std::string& expression, const std::string& imageFile, bool deleteTemp) const
{
	std::string texFilePath = MakeFullPath(TMP_FOLDER, TMP_FILENAME, "tex");
	std::string dviFilePath = MakeFullPath(TMP_FOLDER, TMP_FILENAME, "dvi");

	std::string latexLogFrom = MakeFullPath(TMP_FOLDER, TMP_FILENAME, "log");
	std::string latexLogTo = MakeFullPath(LOG_FOLDER, TMP_FILENAME, "log");
	std::string dvipngStderr = MakeFullPath(LOG_FOLDER, "dvipng_stderr", "log");
	std::string latexStderr = MakeFullPath(LOG_FOLDER, "latex_stderr", "log");
	std::string dvipngStdout = MakeFullPath(LOG_FOLDER, "dvipng_stdout", "log");
	std::string latexStdout = MakeFullPath(LOG_FOLDER, "latex_stdout", "log");

	class flytex_exception : std::exception
	{
	public:
		flytex_exception(Error err) : err(err) {}
		const char* what() const
		{
			return ErrorString(err).c_str();
		}
		Error get() const
		{
			return err;
		}

	private:
		Error err;

	};

	Error error = ERROR_OK;

	try
	{
		if(CreateDirectory(TEXT(TMP_FOLDER), NULL) != TRUE)
		{
			if(GetLastError() != ERROR_ALREADY_EXISTS)
				throw flytex_exception(ERROR_CREATE_TEMP);
		}
		if(CreateDirectory(TEXT(LOG_FOLDER), NULL) != TRUE)
		{
			if(GetLastError() != ERROR_ALREADY_EXISTS)
				throw flytex_exception(ERROR_CREATE_LOG);
		}

		Error result = MakeFileFromTemplate(templateFile, texFilePath, "%::", expression);
		if(result != ERROR_OK)
		{
			throw flytex_exception(result);
		}

		DWORD latexExitCode = ExecuteCommand(latexPath + ' ' + texFilePath + " -interaction=nonstopmode -halt-on-error -output-directory=" + TMP_FOLDER + " >" + latexStdout + " 2>" + latexStderr);

		CopyFileW(Utf8ToUtf16(latexLogFrom).c_str(), Utf8ToUtf16(latexLogTo).c_str(), FALSE);

		if(latexExitCode != 0)
		{
			throw flytex_exception(ERROR_LATEX_PARSE);
		}

		DWORD dvipngExitCode = ExecuteCommand(dvipngPath + " -D " + std::to_string(resolution) + " -o " + '"' + imageFile + '"' + " -bg " + backgroundColor + " -fg " + foregroundColor + " -T tight " + dviFilePath + " >" + dvipngStdout + " 2>" + dvipngStderr);

		if(dvipngExitCode != 0)
			throw flytex_exception(ERROR_DVIPNG);
	}
	catch(const flytex_exception& ex)
	{
		error = ex.get();
	}

	if(deleteTemp)
		if(!RemoveDirectoryRecursive(TMP_FOLDER))
			return ERROR_REMOVE_TEMP;

	return error;
}

Error FlyTexParser::ParseToClipboard(const std::string& expression, bool deleteTemp) const
{
	std::string tmpPngFile = MakeFullPath(TMP_FOLDER, TMP_FILENAME, "png");

	Error res = ParseToImage(expression, tmpPngFile, false);
	if(res != ERROR_OK)
	{
		Cleanup(deleteTemp);
		return res;
	}

	Gdiplus::Bitmap* png = new Gdiplus::Bitmap(Utf8ToUtf16(tmpPngFile).c_str());

	HBITMAP hBmp;
	png->GetHBITMAP(Gdiplus::Color::White, &hBmp);
	HBITMAP hClipBmp = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, 0, 0, NULL);

	delete png;
	png = nullptr;

	if(deleteTemp)
		if(!Cleanup())
			return ERROR_REMOVE_TEMP;

	if(OpenClipboard(NULL))
	{
		//EmptyClipboard();
		HANDLE ret = SetClipboardData(CF_BITMAP, hClipBmp);
		CloseClipboard();

		return (ret == NULL) ? ERROR_CLIPBOARD_PASTE : ERROR_OK;
	}
	else
	{
		return ERROR_CLIPBOARD_OPEN;
	}
}

bool FlyTexParser::Cleanup(bool act)
{
	if(act)
		return RemoveDirectoryRecursive(TMP_FOLDER);

	return true;
}
