#include "stdafx.h"
#include "Hooks.h"
#include "Helpers.h"
#include "CCheat.h"

tD3D11CreateQuery Hooks::oCreateQuery = NULL;
tD3D11Present Hooks::oPresent = NULL;
tD3D11DrawIndexed Hooks::oDrawIndexed = NULL;

HRESULT GenerateShader(ID3D11Device* pD3DDevice, ID3D11PixelShader** pShader, float r, float g, float b)
{
	char szCast[] = "struct VS_OUT"
		"{"
		" float4 Position : SV_Position;"
		" float4 Color : COLOR0;"
		"};"

		"float4 main( VS_OUT input ) : SV_Target"
		"{"
		" float4 fake;"
		" fake.a = 1.0f;"
		" fake.r = %f;"
		" fake.g = %f;"
		" fake.b = %f;"
		" return fake;"
		"}";
	ID3D10Blob* pBlob;
	char szPixelShader[1000];

	sprintf(szPixelShader, szCast, r, g, b);

	ID3DBlob* d3dErrorMsgBlob;

	HRESULT hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", NULL, NULL, "main", "ps_4_0", NULL, NULL, &pBlob, &d3dErrorMsgBlob);

	if (FAILED(hr))
		return hr;

	hr = pD3DDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pShader);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

ID3D11RasterizerState * rwState;
ID3D11RasterizerState * rsState;

enum eDepthState
{
	ENABLED,
	DISABLED,
	READ_NO_WRITE,
	NO_READ_NO_WRITE,
	_DEPTH_COUNT
};

ID3D11PixelShader* psCrimson = NULL;
ID3D11PixelShader* psYellow = NULL;
ID3D11ShaderResourceView* ShaderResourceView;

ID3D11DepthStencilState* myDepthStencilStates[static_cast<int>(eDepthState::_DEPTH_COUNT)];

void SetDepthStencilState(eDepthState aState)
{
	CCheat::pContext->OMSetDepthStencilState(myDepthStencilStates[aState], 1);
}

HRESULT __stdcall Hooks::hkD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool bOnce = false;

	if (!bOnce)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&CCheat::pDevice)))
		{
			pSwapChain->GetDevice(__uuidof(CCheat::pDevice), (void**)&CCheat::pDevice);
			CCheat::pDevice->GetImmediateContext(&CCheat::pContext);
		}

		D3D11_DEPTH_STENCIL_DESC  stencilDesc;
		stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		stencilDesc.StencilEnable = true;
		stencilDesc.StencilReadMask = 0xFF;
		stencilDesc.StencilWriteMask = 0xFF;
		stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		stencilDesc.DepthEnable = true;
		stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		CCheat::pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::ENABLED)]);

		stencilDesc.DepthEnable = false;
		stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		CCheat::pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::DISABLED)]);

		stencilDesc.DepthEnable = false;
		stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		stencilDesc.StencilEnable = false;
		stencilDesc.StencilReadMask = UINT8(0xFF);
		stencilDesc.StencilWriteMask = 0x0;
		CCheat::pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::NO_READ_NO_WRITE)]);

		stencilDesc.DepthEnable = true;
		stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		stencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
		stencilDesc.StencilEnable = false;
		stencilDesc.StencilReadMask = UINT8(0xFF);
		stencilDesc.StencilWriteMask = 0x0;

		stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
		stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
		stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

		stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
		stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
		stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
		stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
		CCheat::pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::READ_NO_WRITE)]);

		D3D11_RASTERIZER_DESC rwDesc;
		CCheat::pContext->RSGetState(&rwState); 
		rwState->GetDesc(&rwDesc); 
		rwDesc.FillMode = D3D11_FILL_WIREFRAME;
		rwDesc.CullMode = D3D11_CULL_NONE;
		CCheat::pDevice->CreateRasterizerState(&rwDesc, &rwState);

		D3D11_RASTERIZER_DESC rsDesc;
		CCheat::pContext->RSGetState(&rsState); 
		rsState->GetDesc(&rsDesc); 
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		CCheat::pDevice->CreateRasterizerState(&rsDesc, &rsState);


		Helpers::Log("D3D11Present initialised");
		bOnce = true;
	}

	if (!psCrimson)
		GenerateShader(CCheat::pDevice, &psCrimson, 1.f, 0.2f, 0.2f);

	if (!psYellow)
		GenerateShader(CCheat::pDevice, &psYellow, 1.f, 0.6f, 0);

	return Hooks::oPresent(pSwapChain, SyncInterval, Flags);
}
void __stdcall Hooks::hkD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	UINT Stride;
	ID3D11Buffer *veBuffer;
	UINT veBufferOffset = 0;
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);

	if (Stride == 36)
	{
		SetDepthStencilState(DISABLED);
		pContext->PSSetShader(psYellow, NULL, NULL);
		
		Hooks::oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
		
		SetDepthStencilState(ENABLED);
		pContext->PSSetShader(psCrimson, NULL, NULL);
		
		pContext->PSSetShaderResources(0, 1, &ShaderResourceView);
	}
		

	return Hooks::oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}
void __stdcall Hooks::hkD3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery)
{
	// YOU GOT ANY MORE OF THAT OCCLUSION???+
	if (pQueryDesc->Query == D3D11_QUERY_OCCLUSION)
	{
		D3D11_QUERY_DESC oqueryDesc = CD3D11_QUERY_DESC();
		(&oqueryDesc)->MiscFlags = pQueryDesc->MiscFlags;
		(&oqueryDesc)->Query = D3D11_QUERY_TIMESTAMP;

		return Hooks::oCreateQuery(pDevice, &oqueryDesc, ppQuery);
	}

	return Hooks::oCreateQuery(pDevice, pQueryDesc, ppQuery);
}