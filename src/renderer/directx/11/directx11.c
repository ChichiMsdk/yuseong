#include "directx11.h"

#include "win32/win32.h"
#include "core/ymemory.h"
#include "core/assert.h"

b8 PrintHresult(HRESULT hr);

#define D11_CHECK(expr) \
	do { \
		HRESULT hr = (expr); \
		if (FAILED(hr)) \
		{ \
			return PrintHresult(hr); \
		} \
	} while (0);

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
		D11_CHECK(
				D3D11CreateDeviceAndSwapChain(
				pAdapter, D3D_DRIVER_TYPE_HARDWARE, hmSoftware,
				createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1, D3D11_SDK_VERSION, 
				&swapChainDesc, &gD11Ctx.pSwapchain, &gD11Ctx.pDevice, &featureLevel, &gD11Ctx.pDeviceContext));
	}
	// Next initialize the back buffer of the swap chain and associate it to a 
	// render target view.
	ID3D11Texture2D* backBuffer;
	D11_CHECK(gD11Ctx.pSwapchain->lpVtbl->GetBuffer( gD11Ctx.pSwapchain, 0, &IID_ID3D11Texture2D, (LPVOID*)&backBuffer));

	D11_CHECK(gD11Ctx.pDevice->lpVtbl->CreateRenderTargetView(
				gD11Ctx.pDevice, (ID3D11Resource *)backBuffer, NULL, &gD11Ctx.pRenderTargetView));

	backBuffer->lpVtbl->Release(backBuffer);
	return TRUE;
}

b8 D11ResizeImpl(YMB OsState* pOsState, YMB uint32_t width, YMB uint32_t height)
{
	return TRUE;
}

b8 D11DrawImpl(YMB OsState* pOsState)
{
	/* YINFO("Dx11 drawing."); */
	return TRUE;
}

void
D11Shutdown(void)
{
}
