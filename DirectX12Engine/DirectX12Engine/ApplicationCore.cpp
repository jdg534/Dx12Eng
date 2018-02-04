#include "ApplicationCore.h"


// put this string somewhere else
const char * s_c_windowClassString = "Dx12EngWindowClass\0";

LRESULT CALLBACK WindowCallBackFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


ApplicationCore::ApplicationCore()
{

}

ApplicationCore::~ApplicationCore()
{

}

HRESULT ApplicationCore::init(HINSTANCE hInst, int nCmdValues, const std::string & indexFile)
{
	// create Win32 window

	WNDCLASSEX winClassEx;
	ZeroMemory(&winClassEx, sizeof(WNDCLASSEX));

	winClassEx.cbSize = sizeof(WNDCLASSEX);

	winClassEx.style = CS_HREDRAW | CS_VREDRAW;
	winClassEx.lpfnWndProc = WindowCallBackFunc;
	winClassEx.cbClsExtra = 0;
	winClassEx.cbWndExtra = 0;
	winClassEx.hInstance = hInst;
	winClassEx.hIcon = LoadIcon(hInst, IDI_APPLICATION); // replace this with the chain from the .rc file
	winClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	winClassEx.lpszMenuName = nullptr;
	winClassEx.lpszClassName = s_c_windowClassString;
	winClassEx.hIconSm = LoadIcon(winClassEx.hInstance, (LPCTSTR)IDI_APPLICATION); // replace this with the chain from the .rc file
	if (!RegisterClassEx(&winClassEx))
		return E_FAIL;

	// Create window
	m_hInst = hInst;
	RECT WindowRect = { 0, 0, 800, 600};
	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);
	m_hWnd = CreateWindow(s_c_windowClassString, "Dx12 Engine Window\0", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, nullptr, nullptr, m_hInst,
		nullptr);
	if (!m_hWnd)
		return E_FAIL;

	ShowWindow(m_hWnd, nCmdValues);

	return S_OK; // next just get a rotating triangle on screen (need to create a Dx12 context first)


	// return E_FAIL;
}

void ApplicationCore::shutdown()
{

}

void ApplicationCore::update()
{

}

void ApplicationCore::draw()
{

}