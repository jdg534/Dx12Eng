#include "Win32Window.h"

#include "resource.h"

Win32Window::Win32Window(const unsigned int width, const unsigned int height, const std::string & title)
	: m_width(width)
	, m_height(height)
	, m_title(title)
{

}

Win32Window::~Win32Window()
{

}

HRESULT Win32Window::createWindow(HINSTANCE instanceHandle, WNDPROC callBackWndProc, int nCmdVals)
{
	m_instanceHandle = instanceHandle;

	WNDCLASSEX winClassEx;
	ZeroMemory(&winClassEx, sizeof(WNDCLASSEX));

	winClassEx.cbSize = sizeof(WNDCLASSEX);

	winClassEx.style = CS_HREDRAW | CS_VREDRAW;
	winClassEx.lpfnWndProc = callBackWndProc;
	winClassEx.cbClsExtra = 0;
	winClassEx.cbWndExtra = 0;
	winClassEx.hInstance = m_instanceHandle;

	HICON iconForWindow = NULL;

	iconForWindow = LoadIcon(m_instanceHandle, "FreeLinkNonComercial.ico");

	if (!iconForWindow)
	{
		iconForWindow = LoadIconA(m_instanceHandle, MAKEINTRESOURCEA(CHAIN_ICON));
	}

	winClassEx.hIcon = iconForWindow;

	winClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	winClassEx.lpszMenuName = nullptr;
	winClassEx.lpszClassName = m_ccp_winClassStr;

	winClassEx.hIconSm = iconForWindow;

	if (!RegisterClassEx(&winClassEx))
	return E_FAIL;

	RECT WindowRect = { 0, 0, m_width, m_height };
	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);
	m_windowHandle = CreateWindow(m_ccp_winClassStr, m_title.c_str(), WS_OVERLAPPEDWINDOW,CW_USEDEFAULT, CW_USEDEFAULT, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, nullptr, nullptr, m_instanceHandle,
		nullptr);
	if (!m_windowHandle)
	{
		return E_FAIL;
	}
	

	ShowWindow(m_windowHandle, nCmdVals);

	return S_OK;
}

void Win32Window::shutdown()
{
	DestroyWindow(m_windowHandle);
}