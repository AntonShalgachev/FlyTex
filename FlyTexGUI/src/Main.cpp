#include "Main.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow)
{
	try
	{
		FlyTexApp app(hInst);
		app.Show(cmdShow);

		return app.Run();
	}
	catch(const std::exception& ex)
	{
		MessageBoxA(NULL, ex.what(), "Exception", MB_OK | MB_ICONERROR);
	}

	return EXIT_FAILURE;
}
