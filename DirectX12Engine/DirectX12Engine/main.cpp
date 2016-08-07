#include <Windows.h>
#include "ApplicationCore.h"



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLn, int nCmdsToShow)
{
	UNREFERENCED_PARAMETER(hPrevInst);
	UNREFERENCED_PARAMETER(lpCmdLn);

	ApplicationCore * applicationCore = new ApplicationCore();

	if (FAILED(applicationCore->init(hInst, nCmdsToShow, "index.txt")))
	{
		return -1;
	}

	MSG windowsMessage = {0};

	while (windowsMessage.message != WM_QUIT)
	{
		if (PeekMessage(&windowsMessage, NULL, 0,0, PM_REMOVE))
		{
			bool messageProcessed = false;

			// logic for processing win32 user input
			// if needed
		}
		else
		{
			applicationCore->update();
			applicationCore->draw();
		}
	}

	applicationCore->shutdown();
	delete applicationCore;
	applicationCore = nullptr;
	return (int)windowsMessage.wParam;
}