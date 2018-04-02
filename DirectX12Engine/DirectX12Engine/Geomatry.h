#pragma once
#ifndef _GEOMATRY_H_
#define _GEOMATRY_H_

#include <wrl.h>

#include <DirectXMath.h>
#include <d3d12.h>


// this should just define structs for representing geomatry
struct Vertex
{
	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT4 m_colour;

	Vertex(DirectX::XMFLOAT3 & IN_pos, DirectX::XMFLOAT4 & IN_colour)
		: m_position(IN_pos)
		, m_colour(IN_colour)
	{

	}
};


struct Geomatry
{
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	// Microsoft::WRL::ComPtr<ID3D12Resource> m_index; // add this later

	// this struct will change 
	UINT m_numVertices;
};


#endif // _GEOMATRY_H_