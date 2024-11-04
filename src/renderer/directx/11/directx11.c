#include "directx11.h"
#ifdef PLATFORM_WINDOWS

/* https://www.3dgep.com/introduction-to-directx-11/  */

#include "win32/win32.h"
#include "core/ymemory.h"
#include "core/assert.h"

static D11Context gD11Ctx = {0};

D3D_FEATURE_LEVEL featureLevels[] = 
{
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
	D3D_FEATURE_LEVEL_9_2,
	D3D_FEATURE_LEVEL_9_1
};

int
D11Init(OsState* pOsState, YMB BOOL vSync)
{
	InternalState *pState = (InternalState *)pOsState->pInternalState;
	RECT clientRect;
	GetClientRect(pState->hWindow, &clientRect);

	uint32_t clientWidth = clientRect.right - clientRect.left;
	uint32_t clientHeight = clientRect.bottom - clientRect.top;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	yZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = pState->hWindow;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed = TRUE;

	uint32_t createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	IDXGIAdapter *pAdapter = NULL;
	HMODULE hmSoftware = NULL;
	/*
	 * NOTE: This will be the feature level that 
	 * is used to create our device and swap chain.
	 */
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
			pAdapter, D3D_DRIVER_TYPE_HARDWARE, hmSoftware,
			createDeviceFlags, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, 
			&swapChainDesc, &gD11Ctx.pSwapchain, &gD11Ctx.pDevice, &featureLevel, &gD11Ctx.pDeviceContext);

	if (hr == E_INVALIDARG)
	{
		D11_CHECK(D3D11CreateDeviceAndSwapChain(
					pAdapter, D3D_DRIVER_TYPE_HARDWARE, hmSoftware, createDeviceFlags, &featureLevels[1], 
					_countof(featureLevels) - 1, D3D11_SDK_VERSION, &swapChainDesc, &gD11Ctx.pSwapchain,
					&gD11Ctx.pDevice, &featureLevel, &gD11Ctx.pDeviceContext));
	}

	/* NOTE:Next initialize the back buffer of the swap chain and associate it to a render target view. */
	ID3D11Texture2D* backBuffer;
	D11_CHECK(gD11Ctx.pSwapchain->lpVtbl->GetBuffer(
				gD11Ctx.pSwapchain, 0, &IID_ID3D11Texture2D, (LPVOID*)&backBuffer));

	D11_CHECK(gD11Ctx.pDevice->lpVtbl->CreateRenderTargetView(
				gD11Ctx.pDevice, (ID3D11Resource *)backBuffer, NULL, &gD11Ctx.pRenderTargetView));

	backBuffer->lpVtbl->Release(backBuffer);

    D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
    yZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	/* NOTE: Create the depth buffer for use with the depth/stencil view. */
    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Width = clientWidth;
    depthStencilBufferDesc.Height = clientHeight;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D11_CHECK(gD11Ctx.pDevice->lpVtbl->CreateTexture2D(
				gD11Ctx.pDevice, &depthStencilBufferDesc, NULL, &gD11Ctx.pDepthStencilBuffer));

    D11_CHECK(gD11Ctx.pDevice->lpVtbl->CreateDepthStencilView(
				gD11Ctx.pDevice, (ID3D11Resource*) gD11Ctx.pDepthStencilBuffer, NULL, &gD11Ctx.pDepthStencilView));

	/* NOTE: Setup depth/stencil state. */
    D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
    yZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	/* NOTE: Should set to FALSE for 2D */
    depthStencilStateDesc.DepthEnable = TRUE;
    depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDesc.StencilEnable = FALSE;

    D11_CHECK(gD11Ctx.pDevice->lpVtbl->CreateDepthStencilState(
				gD11Ctx.pDevice, &depthStencilStateDesc, &gD11Ctx.pDepthStencilState));

	/* NOTE: Setup rasterizer state. */
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory( &rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC) );

    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    /* NOTE: Create the rasterizer state object. */
    D11_CHECK(gD11Ctx.pDevice->lpVtbl->CreateRasterizerState(
				gD11Ctx.pDevice, &rasterizerDesc, &gD11Ctx.pRasterizerState));

    /* NOTE: Initialize the viewport to occupy the entire client area. */
	gD11Ctx.viewport.Width = clientWidth;
	gD11Ctx.viewport.Height = clientHeight;
	gD11Ctx.viewport.TopLeftX = 0.0f;
	gD11Ctx.viewport.TopLeftY = 0.0f;
	gD11Ctx.viewport.MinDepth = 0.0f;
	gD11Ctx.viewport.MaxDepth = 1.0f;

	return TRUE;
}

b8 D11ResizeImpl(YMB OsState* pOsState, YMB uint32_t width, YMB uint32_t height)
{
	return TRUE;
}

b8 D11DrawImpl(YMB OsState* pOsState, YMB void* pCtx)
{
	/* YINFO("Dx11 drawing."); */
	return TRUE;
}

void
D11Shutdown(YMB void* pCtx)
{
}

char *
GetErrorMessage(HRESULT hr) 
{
    LPVOID lpMsgBuf;
    DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    DWORD dwChars = FormatMessageA(
        dwFlags,
        NULL,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf,
        0, NULL);

    if (dwChars == 0) {
        return "Unknown error";
    }
	return lpMsgBuf;
}

b8
PrintHresult(HRESULT hr)
{
	char *pStr = GetErrorMessage(hr);
	YFATAL("%s", pStr);
	LocalFree(pStr);
	return FALSE;
}
#endif // PLATFORM_WINDOWS
