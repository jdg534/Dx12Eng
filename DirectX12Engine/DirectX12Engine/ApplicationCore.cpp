#include "ApplicationCore.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h" // taken from sample code
#include <dxgi1_6.h>

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 colour;
	Vertex(DirectX::XMFLOAT3 & IN_pos, DirectX::XMFLOAT4 & IN_colour)
		: pos(IN_pos)
		, colour(IN_colour)
	{		

	}
};

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
	, m_windowPtr(nullptr)
{
	m_useWarpDevice = false;
}

ApplicationCore::~ApplicationCore()
{

}

HRESULT ApplicationCore::init(HINSTANCE hInst, int nCmdValues, const std::string & indexFile)
{
	// create Win32 window

	m_windowPtr = new Win32Window(800, 600, "Dx12 Engine");

	if (FAILED(m_windowPtr->createWindow(hInst, WindowCallBackFunc, nCmdValues)))
	{
		return E_FAIL;
	} 

	const HWND windowHandle = m_windowPtr->getWindowHandle();

	// start of stuff to put in initDirectx() 

	// viewport and sicssor rect might not need to be defined after the window, both show be moved into the renderer class on refactor
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 800.0f, 600.0f); // might need to define the depth and draw dist
	m_scissorRect = CD3DX12_RECT(0, 0, 800, 600);

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
		MessageBoxA(windowHandle, "CreateDXGIFactory2() failed", "CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)); failed", MB_OK);
		return E_FAIL;
	}

	if (m_useWarpDevice)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;

		directxInitResults = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "factory->EnumWarpAdapter() failed", "factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter); failed", MB_OK);
			return E_FAIL;
		}

		directxInitResults = D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_dx12Device)
		);

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "D3D12CreateDevice() failed", "D3D_FEATURE_LEVEL_11_0 with WARP failed", MB_OK);
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
			MessageBoxA(windowHandle, "D3D12CreateDevice() failed", "D3D_FEATURE_LEVEL_11_0 with hardware failed", MB_OK);
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
		MessageBoxA(windowHandle, "Failed to create command queue", "CreateCommandQueue() failed", MB_OK);
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
	
	m_aspectRatio = (float) swapChainDesc.Width / (float) swapChainDesc.Height;


	
	// assuming this struct would get used to create a projection matrix

	directxInitResults = factory->CreateSwapChainForHwnd(
		m_dx12CommandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);

	if (FAILED(directxInitResults))
	{
		MessageBoxA(windowHandle, "Swap chain creation failed", "CreateSwapChainForHwnd() failed", MB_OK);
		return E_FAIL;
	}


	directxInitResults = swapChain.As(&m_swapChain);

	if (FAILED(directxInitResults))
	{
		MessageBoxA(windowHandle, "Swap chain creation failed", "swapChain.As(&m_swapChain);", MB_OK);
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
			MessageBoxA(windowHandle, "Failed to create heap descriptor", "Failed to create heap descriptor for the render target view", MB_OK);
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
		MessageBoxA(windowHandle, "Failed to create command allocator", "m_dx12Device->CreateCommandAllocator() failed", MB_OK);
		return E_FAIL;
	}

	// end of stuff to put in initiDirectX()


	// get a simple rotating triangle on screen

	// start of code to put in initRenderer()
	// the additional scope is just to follow the sample
	// create root siganture
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
		ZeroMemory(&rootSigDesc, sizeof(CD3DX12_ROOT_SIGNATURE_DESC));

		rootSigDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Microsoft::WRL::ComPtr<ID3DBlob> sig;
		Microsoft::WRL::ComPtr<ID3DBlob> err;

		directxInitResults = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err);

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to create root signature", "D3D12SerializeRootSignature() failed", MB_OK);
			return E_FAIL;
		}

		directxInitResults = m_dx12Device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_dx12RootSig));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to create root signature", "m_dx12Device->CreateRootSignature() failed", MB_OK);
			return E_FAIL;
		}
	}

	// initialise the input assembler and associated shader
	{
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob;

#ifdef _DEBUG
		UINT shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UNIT shaderCompileFlags = 0;
#endif

		directxInitResults = D3DCompileFromFile(L"DefaultShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", shaderCompileFlags, 0, &vsBlob, nullptr);

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to compile Vertex Shader", "Failed to compile Vertex Shader", MB_OK);
			return E_FAIL;
		}

		directxInitResults = D3DCompileFromFile(L"DefaultShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", shaderCompileFlags, 0, &psBlob, nullptr);
		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to compile pixel Shader", "Failed to compile pixel Shader", MB_OK);
			return E_FAIL;
		}

		D3D12_INPUT_ELEMENT_DESC inputElementDesc [] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
		psoDesc.pRootSignature = m_dx12RootSig.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		directxInitResults = m_dx12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to create pipeline stage", "m_dx12Device->CreateGraphicsPipelineState() failed", MB_OK);
			return E_FAIL;
		}
	}

	directxInitResults = m_dx12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_dx12CmdAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList));

	if (FAILED(directxInitResults))
	{
		MessageBoxA(windowHandle, "Failed to create command list", "m_dx12Device->CreateCommandList() failed", MB_OK);
		return E_FAIL;
	}

	directxInitResults = m_commandList->Close();
	if (FAILED(directxInitResults))
	{
		MessageBoxA(windowHandle, "Failed to clear the command list", "m_commandList->Close() failed", MB_OK);
		return E_FAIL;
	}

	{
		// Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			Vertex(DirectX::XMFLOAT3( 0.0f, 0.25f * m_aspectRatio, 0.0f ),DirectX::XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f )),
			Vertex(DirectX::XMFLOAT3(0.25f, -0.25f * m_aspectRatio, 0.0f ),DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f )),
			Vertex(DirectX::XMFLOAT3(-0.25f, -0.25f * m_aspectRatio, 0.0f ),DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f ))
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// 
		directxInitResults=  m_dx12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to clear the command list", "m_commandList->Close() failed", MB_OK);
			return E_FAIL;
		}

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.

		directxInitResults = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to clear the command list", "m_commandList->Close() failed", MB_OK);
			return E_FAIL;
		}

		// populate the vertex buffer via the mapping
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		
		// remove the mapping as the copy has taken place
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		// taken from Dx12 sample

		directxInitResults = m_dx12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		if (FAILED(directxInitResults))
		{
			MessageBoxA(windowHandle, "Failed to create fence", "m_dx12Device->CreateFence() failed", MB_OK);
			return E_FAIL;
		}
		
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			 directxInitResults = HRESULT_FROM_WIN32(GetLastError());

			 MessageBoxA(windowHandle, "Failed to create fence event handle", "CreateEvent() return nullptr", MB_OK);
			 return E_FAIL;
		}

		// Wait for the command list to execute; we are reusing the same command 
		// due to following sample code only need to deal with one command list.
		waitForLastFrame();
	}

	// end of code to put in initRenderer()

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
	waitForLastFrame();

	// free / release / delete all Dx12 structs
	CloseHandle(m_fenceEvent);

	/* (is addition to the code in the sample) free:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxDeviceAdapter;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_dx12RootSig;
	Microsoft::WRL::ComPtr<ID3D12Device> m_dx12Device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_dx12CommandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_renderTargetviewDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[2];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_dx12CmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	*/

	m_vertexBuffer.~ComPtr(); // this should free any associated resorces
	m_dxDeviceAdapter.~ComPtr();
	m_dx12RootSig.~ComPtr();
	m_dx12Device.~ComPtr();
	m_dx12CommandQueue.~ComPtr();
	m_swapChain.~ComPtr();
	m_renderTargetviewDescHeap.~ComPtr();
	m_renderTargets[0].~ComPtr();
	m_renderTargets[1].~ComPtr();
	m_dx12CmdAllocator.~ComPtr();
	m_pipelineState.~ComPtr();
	m_commandList.~ComPtr();

	m_windowPtr->shutdown();

	delete m_windowPtr;
}

void ApplicationCore::update()
{

}

void ApplicationCore::draw()
{
	// initial code from the sample

	populateDxCmdList();

	ID3D12CommandList* ppCmdLists[] = { m_commandList.Get() };

	// array of size 1, can have multiple command lists?
	m_dx12CommandQueue->ExecuteCommandLists(1, ppCmdLists);


	// present the frame
	if (FAILED(m_swapChain->Present(1, 0)))
	{
		throw "m_swapChain->present() failed";
	}

	waitForLastFrame();
}

void ApplicationCore::waitForLastFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	HRESULT signalResult = m_dx12CommandQueue->Signal(m_fence.Get(), fence);

	if (FAILED(signalResult))
	{
		throw "m_dx12CommandQueue->Signal() failed";
	}

	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		HRESULT setEventOnCompletionRes = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		
		if (FAILED(setEventOnCompletionRes))
		{
			throw "m_fence->SetEventOnCompletion() failed";
		}
		
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void ApplicationCore::populateDxCmdList()
{
	// clear the command allocator
	HRESULT hRes = m_dx12CmdAllocator->Reset();

	if (FAILED(hRes))
	{
		throw "m_dx12CmdAllocator->Reset failed";
	}

	// reset the command list for repopulation
	hRes = m_commandList->Reset(m_dx12CmdAllocator.Get(), m_pipelineState.Get());

	if (FAILED(hRes))
	{
		throw "Failed to reset the command list!";
	}



	// set the state
	m_commandList->SetGraphicsRootSignature(m_dx12RootSig.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// inform that the current render target is the back buffer
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_renderTargetviewDescHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// start defining commands
	const float clearClr[] = { 0.0f, 0.4f , 0.2f ,1.0f};
	m_commandList->ClearRenderTargetView(rtvHandle, clearClr, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0); // just does that contents of the current vertex buffer, once moving to full 3D rendered scene look into creating a index buffer

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// close the command list
	hRes = m_commandList->Close();

	if (FAILED(hRes))
	{
		throw "Failed the close the command list";
	}
}