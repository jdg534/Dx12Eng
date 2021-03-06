#include "Dx12Renderer.h"

#include <d3dcompiler.h>

Dx12Renderer::Dx12Renderer(const UINT width, const UINT height)
	: m_width(width)
	, m_height(height)
	, m_useWarpDevice(false)
	, m_dxDeviceAdapter(nullptr)
	, m_dx12RootSig(nullptr)
	, m_dx12Device(nullptr)
	, m_dx12CommandQueue(nullptr)
	, m_swapChain(nullptr)
	, m_renderTargetviewDescHeap(nullptr)
	, m_dx12CmdAllocator(nullptr)
	, m_pipelineState(nullptr)
	, m_commandList(nullptr)
	, m_fence(nullptr)

{
	m_renderTargets[0] = nullptr;
	m_renderTargets[1] = nullptr;
}

Dx12Renderer::~Dx12Renderer()
{

}

HRESULT Dx12Renderer::init(const HWND windowHandle)
{
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

	m_aspectRatio = (float)swapChainDesc.Width / (float)swapChainDesc.Height;



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

		D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
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

	// need to get access to the Dx12 Device object

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

	return S_OK;
}

void Dx12Renderer::shutdown()
{
	waitForLastFrame();

	CloseHandle(m_fenceEvent);

	m_dxDeviceAdapter.~ComPtr();
	m_dx12RootSig.~ComPtr();
	m_dx12Device.~ComPtr();
	m_dx12CommandQueue.~ComPtr();
	m_swapChain.~ComPtr();
	m_renderTargetviewDescHeap.~ComPtr();
	m_dx12CmdAllocator.~ComPtr();
	m_pipelineState.~ComPtr();
	m_commandList.~ComPtr();
	m_fence.~ComPtr();
	m_renderTargets[0].~ComPtr();
	m_renderTargets[1].~ComPtr();
}

void Dx12Renderer::waitForLastFrame()
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

void Dx12Renderer::createInitialDrawingCommands()
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
	const float clearClr[] = { 0.0f, 0.4f , 0.2f ,1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearClr, 0, nullptr);
}

void Dx12Renderer::appendDrawingCommands(const Geomatry & toDraw)
{
	// append stuff to the command list
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // this could likely be moved to createInitialDrawingCommands
	m_commandList->IASetVertexBuffers(0, 1, &toDraw.m_vertexBufferView);
	m_commandList->DrawInstanced(toDraw.m_numVertices, 1, 0, 0);
}

void Dx12Renderer::finishDrawing()
{
	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// close the command list
	HRESULT hRes = m_commandList->Close();

	if (FAILED(hRes))
	{
		throw "Failed the close the command list";
	}


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