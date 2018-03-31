#pragma once
#ifndef _WIN32_WINDOW_H_
#define _WIN32_WINDOW_H_

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Win32Window
{
public:
	Win32Window(const unsigned int width, const unsigned int height, const std::string & title);
	~Win32Window();

	HRESULT createWindow(HINSTANCE instanceHandle, WNDPROC callBackWndProc, int nCmdVals);

	const unsigned int getWidth() { return m_width; }
	const unsigned int getHeight() { return m_height; }
	const HWND getWindowHandle() { return m_windowHandle; }

	void shutdown();

private:
	unsigned int m_width;
	unsigned int m_height;
	std::string m_title;

	HINSTANCE m_instanceHandle;
	HWND m_windowHandle;

	const char * m_ccp_winClassStr = "WIN32WINDOW\0";
};


#endif