#include "stdafx.h"
#include "Hooks.h"
#include "Helpers.h"
#include "CCheat.h"

tD3D11CreateQuery Hooks::oCreateQuery = NULL;
tD3D11Present Hooks::oPresent = NULL;
tD3D11DrawIndexed Hooks::oDrawIndexed = NULL;

// PASTED
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
		" fake.a = 1.f;"
		" fake.r = %f;"
		" fake.g = %f;"
		" fake.b = %f;"
		" return fake;"
		"}";
	ID3D10Blob* pBlob;
	char szPixelShader[250];

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

ID3D11PixelShader* psRed = NULL;
ID3D11PixelShader* psOrange = NULL;

ID3D11DepthStencilState* g_depthEnabled;
ID3D11DepthStencilState* g_depthDisabled;

HRESULT __stdcall Hooks::hkD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool bOnce = false;

	if (!bOnce)
	{
		bOnce = true;

		if (pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&CCheat::pDevice))
		{
			pSwapChain->GetDevice(__uuidof(CCheat::pDevice), (void**)&CCheat::pDevice);
			CCheat::pDevice->GetImmediateContext(&CCheat::pContext);
		}


		GenerateShader(CCheat::pDevice, &psRed, 1.f, 0.f, 0.f);
		GenerateShader(CCheat::pDevice, &psOrange, 1.f, 0.3f, 0);

		ID3D11DepthStencilState* sState;
		CCheat::pContext->OMGetDepthStencilState(&sState, NULL);

		D3D11_DEPTH_STENCIL_DESC sDesc;
		sState->GetDesc(&sDesc);

		sDesc.DepthEnable = true;
		CCheat::pDevice->CreateDepthStencilState(&sDesc, &g_depthEnabled);

		sDesc.DepthEnable = false;
		CCheat::pDevice->CreateDepthStencilState(&sDesc, &g_depthDisabled);

		Helpers::Log("D3D11Present initialised");
	}

	return Hooks::oPresent(pSwapChain, SyncInterval, Flags);
}

void __stdcall Hooks::hkD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	// GET STRIDE
	UINT Stride;
	ID3D11Buffer *veBuffer;
	UINT veBufferOffset = 0;
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);

	if (Stride == 36) // PLAYER AND CARS
	{
		CCheat::pContext->OMSetDepthStencilState(g_depthDisabled, 1); // DEPTH DISABLED
		pContext->PSSetShader(psOrange, NULL, NULL);
		
		Hooks::oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
		
		CCheat::pContext->OMSetDepthStencilState(g_depthEnabled, 1); // DEPTH ENABLED
		pContext->PSSetShader(psRed, NULL, NULL);
	}
		

	return Hooks::oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}
void __stdcall Hooks::hkD3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery)
{
	// DISABLE OCCLUSION, WHICH PREVENTS RENDERING PLAYER MODEL THROUGH CERTAIN OBJECTS
	if (pQueryDesc->Query == D3D11_QUERY_OCCLUSION)
	{
		D3D11_QUERY_DESC oqueryDesc = CD3D11_QUERY_DESC();
		(&oqueryDesc)->MiscFlags = pQueryDesc->MiscFlags;
		(&oqueryDesc)->Query = D3D11_QUERY_TIMESTAMP;

		return Hooks::oCreateQuery(pDevice, &oqueryDesc, ppQuery);
	}

	return Hooks::oCreateQuery(pDevice, pQueryDesc, ppQuery);
}
