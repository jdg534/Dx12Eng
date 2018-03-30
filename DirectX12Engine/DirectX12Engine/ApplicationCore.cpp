#include "ApplicationCore.h"

#include <d3d12.h>
#include "d3dx12.h" // taken from sample code
#include <dxgi1_6.h>

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
	: m_dxDeviceAdapter(nullptr)
	, m_dx12RootSig(nullptr)
	, m_dx12Device(nullptr)
	, m_dx12CommandQueue(nullptr)
	, m_swapChain(nullptr)
	, m_renderTargetviewDescHeap(nullptr)
{
	m_useWarpDevice = false;
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
	
	HICON iconForWindow = NULL;
		
	iconForWindow = LoadIcon(hInst, "FreeLinkNonComercial.ico");

	if (!iconForWindow)
	{
		iconForWindow = LoadIconA(hInst, MAKEINTRESOURCEA(CHAIN_ICON));
	}
	
	winClassEx.hIcon = iconForWindow;
	
	winClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	winClassEx.lpszMenuName = nullptr;
	winClassEx.lpszClassName = s_c_windowClassString;
	
	winClassEx.hIconSm = iconForWindow;
	
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

	// start of stuff to put in initDirectx() 


	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	HRESULT directxInitResults;

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	directxInitResults = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

	if (FAILED(directxInitResults))
	{
		MessageBoxA(m_hWnd, "CreateDXGIFactory2() failed", "CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)); failed", MB_OK);
		return E_FAIL;
	}

	if (m_useWarpDevice)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;

		directxInitResults = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(m_hWnd, "factory->EnumWarpAdapter() failed", "factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter); failed", MB_OK);
			return E_FAIL;
		}

		directxInitResults = D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_dx12Device)
		);

		if (FAILED(directxInitResults))
		{
			MessageBoxA(m_hWnd, "D3D12CreateDevice() failed", "D3D_FEATURE_LEVEL_11_0 with WARP failed", MB_OK);
			return E_FAIL;
		}
	}
	else
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
		
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		directxInitResults = D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_dx12Device)
		);

		if (FAILED(directxInitResults))
		{
			MessageBoxA(m_hWnd, "D3D12CreateDevice() failed", "D3D_FEATURE_LEVEL_11_0 with hardware failed", MB_OK);
			return E_FAIL;
		}
	}

	// define the command queue
	D3D12_COMMAND_QUEUE_DESC d3d12CmdQueueDesc;
	ZeroMemory(&d3d12CmdQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3d12CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3d12CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	directxInitResults = m_dx12Device->CreateCommandQueue(&d3d12CmdQueueDesc, IID_PPV_ARGS(&m_dx12CommandQueue));

	if (FAILED(directxInitResults))
	{
		MessageBoxA(m_hWnd, "Failed to create command queue", "CreateCommandQueue() failed", MB_OK);
		return E_FAIL;
	}

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2; // play with this number, double buffer?
	swapChainDesc.Width = 800;
	swapChainDesc.Height = 600; // make this value readable from file
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	// re check everythign below his line!
	// DON'T use UWP example!

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	


	directxInitResults = factory->CreateSwapChainForHwnd(
		m_dx12CommandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);

	if (FAILED(directxInitResults))
	{
		MessageBoxA(m_hWnd, "Swap chain creation failed", "CreateSwapChainForHwnd() failed", MB_OK);
		return E_FAIL;
	}


	directxInitResults = swapChain.As(&m_swapChain);

	if (FAILED(directxInitResults))
	{
		MessageBoxA(m_hWnd, "Swap chain creation failed", "swapChain.As(&m_swapChain);", MB_OK);
		return E_FAIL;
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	{
		// the Dx12 sample added a scope, this might not be needed.
		// going for geting stuff on screen then refactoring

		// create descriptor heap, some manual memory management?
		D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
		ZeroMemory(&renderTargetViewHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
		renderTargetViewHeapDesc.NumDescriptors = 2; // 2 frames, double buffer
		renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		directxInitResults = m_dx12Device->CreateDescriptorHeap(&renderTargetViewHeapDesc, IID_PPV_ARGS(&m_renderTargetviewDescHeap));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(m_hWnd, "Failed to create heap descriptor", "Failed to create heap descriptor for the render target view", MB_OK);
			return E_FAIL;
		}

		m_rtvDescriptorSize = m_dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	{
		// add the descriptor handle, need to use D3DX12
		CD3DX12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle(m_renderTargetviewDescHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < 2; ++i)
		{
			m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])); // fix this HERE!!!
			m_dx12Device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, renderTargetViewHandle);
			renderTargetViewHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	directxInitResults = m_dx12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_dx12CmdAllocator));

	if (FAILED(directxInitResults))
	{
		MessageBoxA(m_hWnd, "Failed to create command allocator", "m_dx12Device->CreateCommandAllocator() failed", MB_OK);
		return E_FAIL;
	}

	// end of stuff to put in initiDirectX()


	// PICK UP HERE!
	// get a simple rotating triangle on screen



	return S_OK; // next just get a rotating triangle on screen (need to create a Dx12 context first)
}

int ApplicationCore::run()
{
	MSG windowsMessage = { 0 };

	while (windowsMessage.message != WM_QUIT)
	{
		if (PeekMessage(&windowsMessage, NULL, 0, 0, PM_REMOVE))
		{
			bool messageProcessed = false;

			// logic for processing win32 user input
			// if needed
			if (windowsMessage.message == WM_QUIT)
			{
				messageProcessed = true;
				break;
			}

			if (!messageProcessed)
			{
				TranslateMessage(&windowsMessage);
				DispatchMessage(&windowsMessage);
			}
		}
		else
		{
			update();
			draw();
		}
	}

	return static_cast<int>(windowsMessage.wParam);
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

