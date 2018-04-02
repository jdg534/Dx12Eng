#ifndef _APPLICATION_CORE_H_
#define _APPLICATION_CORE_H_

#include <Windows.h>
#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_6.h>

// include the Dx12 helper header
#include "d3dx12.h"

#include <chrono>
#include <string>
#include <cfloat>

#include "resource.h"

#include "Win32Window.h"
#include "Dx12Renderer.h"

#include "Geomatry.h"

class ApplicationCore
{
public:
	ApplicationCore();
	~ApplicationCore();


	HRESULT init(HINSTANCE hInst, int nCmdValues, const std::string & indexFile);
	int run();
	void shutdown();
	
	

private:

	void update();
	void draw();
	void populateDxCmdList();


	std::chrono::steady_clock::time_point m_timeAtStartOfTheFrame,
		m_timeAtEndOfTheFrame;
	float m_deltaTimeForFrame;

	Win32Window * m_windowPtr;
	Dx12Renderer * m_rendererPtr;

	Geomatry m_geomatry;

	float m_aspectRatio;
};

#endif