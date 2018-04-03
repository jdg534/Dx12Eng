#include "ApplicationCore.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h" // taken from sample code
#include <dxgi1_6.h>

#include <DirectXMath.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <vector>

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
	: m_windowPtr(nullptr)
	, m_rendererPtr(nullptr)
{
	
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

	m_rendererPtr = new Dx12Renderer(800, 600);

	if (FAILED(m_rendererPtr->init(windowHandle)))
	{
		return E_FAIL;
	}

	m_aspectRatio = 800.0f / 600.0f;
	
	// populate the vertex buffer, (deal with the geomatry struct)

	{
		HRESULT vertexBufferCreation = E_FAIL;

		Assimp::Importer importer;

		const aiScene * testScene = importer.ReadFile("TestCube.obj",
			//aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			// aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType |
			aiProcess_GenNormals |
			// aiProcess_FlipWindingOrder|
			aiProcess_GenUVCoords |
			aiProcess_MakeLeftHanded);

		assert(testScene);

		std::vector<Vertex> vertexInput;

		assert(testScene->mNumMeshes == 1);

		vertexInput.resize(testScene->mMeshes[0]->mNumVertices);

		// down scale the vertex data, want it to be visable on screen

		const float scaleVerticesBy = 0.25f;

		for (size_t i = 0; i < testScene->mMeshes[0]->mNumVertices; ++i)
		{
			vertexInput[i].m_position.x = testScene->mMeshes[0]->mVertices[i].x * scaleVerticesBy;
			vertexInput[i].m_position.y = testScene->mMeshes[0]->mVertices[i].y * scaleVerticesBy;
			vertexInput[i].m_position.z = testScene->mMeshes[0]->mVertices[i].z * scaleVerticesBy;

			// assign colour based on i
			if (i % 3 == 0) // i is a multiple of 3
			{
				vertexInput[i].m_colour = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f); // blue
			}
			else if (i % 2 == 0) // i is a multiple of 2
			{
				vertexInput[i].m_colour = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f); // green
			}
			else
			{
				vertexInput[i].m_colour = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // red
			}
		}

		const UINT vertexBufferSize = sizeof(Vertex) * testScene->mMeshes[0]->mNumVertices;

		const Microsoft::WRL::ComPtr<ID3D12Device> devicePtr = m_rendererPtr->getDevicePtr();
		
		vertexBufferCreation = devicePtr->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_geomatry.m_vertexBuffer));

		if (FAILED(vertexBufferCreation))
		{
			MessageBoxA(windowHandle, "Failed to clear the command list", "m_commandList->Close() failed", MB_OK);
			return E_FAIL;
		}

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.

		vertexBufferCreation = m_geomatry.m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

		if (FAILED(vertexBufferCreation))
		{
			MessageBoxA(windowHandle, "Failed to clear the command list", "m_commandList->Close() failed", MB_OK);
			return E_FAIL;
		}

		// populate the vertex buffer via the mapping
		memcpy(pVertexDataBegin, &vertexInput[0], vertexBufferSize);

		// remove the mapping as the copy has taken place
		m_geomatry.m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_geomatry.m_vertexBufferView.BufferLocation = m_geomatry.m_vertexBuffer->GetGPUVirtualAddress();
		m_geomatry.m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_geomatry.m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}


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
	m_geomatry.m_vertexBuffer.~ComPtr(); // this should free any associated resorces
	
	m_rendererPtr->shutdown();
	delete m_rendererPtr;

	m_windowPtr->shutdown();

	delete m_windowPtr;
}

void ApplicationCore::update()
{

}

void ApplicationCore::draw()
{
	// initial code from the sample
	m_rendererPtr->createInitialDrawingCommands();


	populateDxCmdList(); // this will need to be moved into the renderer class, 
	// just deal with initial commands, 

	m_rendererPtr->finishDrawing();
}



void ApplicationCore::populateDxCmdList()
{
	m_rendererPtr->appendDrawingCommands(m_geomatry);
}