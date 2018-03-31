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
	void waitForLastFrame();
	void populateDxCmdList();


	std::chrono::steady_clock::time_point m_timeAtStartOfTheFrame,
		m_timeAtEndOfTheFrame;
	float m_deltaTimeForFrame;

	// Windows hinstance
	HINSTANCE m_hInst;
	HWND m_hWnd; // window handle

	// Dx12 structs
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
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	UINT m_rtvDescriptorSize;

	bool m_useWarpDevice;

	// current frame index, based off the Dx12 Win32 sample
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	float m_aspectRatio;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// tempory code, figure out a good way to replace this
	static inline void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		*ppAdapter = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*ppAdapter = adapter.Detach();
	}
};

#endif