#ifndef _APPLICATION_CORE_H_
#define _APPLICATION_CORE_H_

#include <Windows.h>

#include <chrono>
#include <string>
#include <cfloat>

#include "resource.h"

class ApplicationCore
{
public:
	ApplicationCore();
	~ApplicationCore();


	HRESULT init(HINSTANCE hInst, int nCmdValues, const std::string & indexFile);

	void shutdown();

	void update();
	void draw();

private:
	std::chrono::steady_clock::time_point m_timeAtStartOfTheFrame,
		m_timeAtEndOfTheFrame;
	float m_deltaTimeForFrame;

	// Windows hinstance
	HINSTANCE m_hInst;
	HWND m_hWnd; // window handle

};

#endif