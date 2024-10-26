#ifndef YDIRECTX11_H
#define YDIRECTX11_H

#ifdef PLATFORM_WINDOWS
 #include "mydefines.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <initguid.h>

#include "os.h"

// DEFINE_GUID(IID_ID3D11Texture2D, 0x6f15aaf2, 0xd208, 0x4e89, 0x9a, 0xb4, 0x48, 0x95, 0x98, 0xb3, 0x17, 0x5b);

typedef struct D11VertexBuffer
{
	ID3D11InputLayout*	pInputLayout;
	ID3D11Buffer* pHandle;
	ID3D11Buffer* pIndexBuffer;
}D11VertexBuffer;

typedef struct D11ShaderData
{
	ID3D11VertexShader* pVertexShader;
	ID3D11PixelShader* pPixelShader;
}D11ShaderData;

typedef struct D11Context
{
	ID3D11Device* pDevice;
	ID3D11DeviceContext* pDeviceContext;
	IDXGISwapChain* pSwapchain;
	ID3D11RenderTargetView* pRenderTargetView;
	ID3D11DepthStencilView* pDepthStencilView;
	ID3D11Texture2D* pDepthStencilBuffer;
	ID3D11DepthStencilState* pDepthStencilState;
	ID3D11RasterizerState* pRasterizerState;
	D3D11_VIEWPORT viewport;
}D11Context;

#define D11_CHECK(expr) \
	do { \
		HRESULT hr = (expr); \
		if (FAILED(hr)) \
		{ \
			return PrintHresult(hr); \
		} \
	} while (0);


YND int D11Init(
		OsState*							pOsState,
		YMB BOOL							vSync);

b8 D11ResizeImpl(
		OsState*							pOsState,
		uint32_t							width,
		uint32_t							height);

b8 D11DrawImpl(
		YMB OsState*						pOsState);

b8 PrintHresult(
		HRESULT								hr);

char *GetErrorMessage(
		HRESULT								hr);

void D11Shutdown(void);

#endif // PLATFORM_WINDOWS
#endif //YDIRECTX12_H
