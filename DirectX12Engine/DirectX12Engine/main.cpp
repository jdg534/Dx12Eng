// define lean and mean flags before Windows.h is included
#define WIN32_LEAN_AND_MEAN
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

	int runResults = applicationCore->run();

	applicationCore->shutdown();
	delete applicationCore;
	applicationCore = nullptr;
	return runResults;
}