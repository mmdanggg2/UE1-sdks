/*=============================================================================
	OpenGL_DXGI_Interop.cpp

	Low-latency DXGI rendering support

	Revision history:
	* Created by Stijn Volckaert
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

// stijn: GL on DXGI support
#if UTGLR_SUPPORT_DXGI_INTEROP
#pragma comment (lib, "d3d11.lib")
#include <dxgi1_3.h>
#include <VersionHelpers.h>

void FOpenGLBase::CreateDXGISwapchain()
{
	if (!RenDev->UseLowLatencySwapchain || RenDev->D3DDevice || !SUPPORTS_WGL_NV_DX_interop || !SUPPORTS_WGL_NV_DX_interop2)
		return;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;// RenDev->RefreshRate;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.SampleDesc.Count = RenDev->UsingFBOMultisampling() ? RenDev->NumAASamples : 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.OutputWindow = static_cast<HWND>(RenDev->Viewport->GetWindow());
	SwapChainDesc.Windowed = TRUE; // can be changed to fullscreen through IDXGISwapChain::SetFullscreenState
	SwapChainDesc.SwapEffect = IsWindows10OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = 0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE, 
		NULL, 
		/*D3D11_CREATE_DEVICE_DEBUG*/D3D11_CREATE_DEVICE_SINGLETHREADED,
		NULL, 
		0,
		D3D11_SDK_VERSION, 
		&SwapChainDesc,
		&RenDev->DXGISwapChain, 
		&RenDev->D3DDevice, 
		NULL, 
		&RenDev->D3DContext);

	if (hr == S_OK)
		RenDev->D3DDeviceInteropHandle = FOpenGLBase::wglDXOpenDeviceNV(RenDev->D3DDevice);

	if (hr != S_OK || RenDev->D3DDeviceInteropHandle == NULL)
	{
		debugf(TEXT("OpenGL: Failed to create DXGI Device - UseLowLatencySwapchain disabled"));
		RenDev->D3DDevice = nullptr;
		RenDev->DXGISwapChain = nullptr;
		RenDev->D3DContext = nullptr;
		RenDev->UseLowLatencySwapchain = FALSE;
	}
}

void FOpenGLBase::SetDXGIFullscreenState(UBOOL Fullscreen)
{
	if (!RenDev->D3DDevice)
		return;

	IDXGISwapChain_SetFullscreenState(RenDev->DXGISwapChain, Fullscreen, nullptr);
}

void FOpenGLBase::DestroyDXGISwapchain()
{
	if (!RenDev->D3DDevice)
		return;

	ID3D11DeviceContext_ClearState(RenDev->D3DContext);

	wglDXUnregisterObjectNV(RenDev->D3DDeviceInteropHandle, RenDev->D3DColorViewInteropHandle);
	wglDXUnregisterObjectNV(RenDev->D3DDeviceInteropHandle, RenDev->D3DDepthStencilViewInteropHandle);

	wglDXCloseDeviceNV(RenDev->D3DDeviceInteropHandle);

	ID3D11RenderTargetView_Release(RenDev->D3DColorView);
	ID3D11DepthStencilView_Release(RenDev->D3DDepthStencilView);
	ID3D11DeviceContext_Release(RenDev->D3DContext);
	ID3D11Device_Release(RenDev->D3DDevice);
	IDXGISwapChain_Release(RenDev->DXGISwapChain);

	RenDev->DXGISwapChain = nullptr;
	RenDev->D3DContext = nullptr;

	RenDev->D3DDevice = nullptr;
	RenDev->D3DDeviceInteropHandle = NULL;

	RenDev->D3DColorView = nullptr;
	RenDev->D3DColorViewInteropHandle = NULL;

	RenDev->D3DDepthStencilView = nullptr;
	RenDev->D3DDepthStencilViewInteropHandle = NULL;
}

bool FOpenGLBase::UpdateDXGISwapchain()
{
	if (!RenDev->D3DDevice)
		return false;

	HRESULT hr;

	if (RenDev->D3DColorView)
	{
		wglDXUnregisterObjectNV(RenDev->D3DDeviceInteropHandle, RenDev->D3DColorViewInteropHandle);
		wglDXUnregisterObjectNV(RenDev->D3DDeviceInteropHandle, RenDev->D3DDepthStencilViewInteropHandle);

		ID3D11DeviceContext_OMSetRenderTargets(RenDev->D3DContext, 0, NULL, NULL);
		ID3D11RenderTargetView_Release(RenDev->D3DColorView);
		ID3D11DepthStencilView_Release(RenDev->D3DDepthStencilView);

		hr = IDXGISwapChain_ResizeBuffers(RenDev->DXGISwapChain, 1, RenDev->RequestedFramebufferWidth, RenDev->RequestedFramebufferHeight, DXGI_FORMAT_UNKNOWN, 0);
		check(SUCCEEDED(hr) && "Couldn't resize DXGI Swapchain buffers");
	}

	ID3D11Texture2D* ColorBuffer;
	hr = IDXGISwapChain_GetBuffer(RenDev->DXGISwapChain, 0, *(_GUID*)(&IID_ID3D11Texture2D), (void**)&ColorBuffer);
	check(SUCCEEDED(hr) && "Couldn't get DXGI Swapchain color buffer handle");

	hr = ID3D11Device_CreateRenderTargetView(RenDev->D3DDevice, (ID3D11Resource*)ColorBuffer, NULL, &RenDev->D3DColorView);
	check(SUCCEEDED(hr) && "Couldn't create DXGI ColorView");

	ID3D11Texture2D_Release(ColorBuffer);

	D3D11_TEXTURE2D_DESC DepthStencilTextureDesc = {};
	DepthStencilTextureDesc.Width = RenDev->RequestedFramebufferWidth;
	DepthStencilTextureDesc.Height = RenDev->RequestedFramebufferHeight;
	DepthStencilTextureDesc.MipLevels = 1;
	DepthStencilTextureDesc.ArraySize = 1;
	DepthStencilTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilTextureDesc.SampleDesc.Count = RenDev->UsingFBOMultisampling() ? RenDev->NumAASamples : 1;
	DepthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* DepthStencilBuffer;
	hr = ID3D11Device_CreateTexture2D(RenDev->D3DDevice, &DepthStencilTextureDesc, NULL, &DepthStencilBuffer);
	check(SUCCEEDED(hr) && "Couldn't create DXGI DepthStencil buffer");

	hr = ID3D11Device_CreateDepthStencilView(RenDev->D3DDevice, (ID3D11Resource*)DepthStencilBuffer, NULL, &RenDev->D3DDepthStencilView);
	check(SUCCEEDED(hr) && "Couldn't create DXGI DepthStencilView");

	if (RenDev->UsingFBOMultisampling())
	{
		RenDev->D3DColorViewInteropHandle = wglDXRegisterObjectNV(RenDev->D3DDeviceInteropHandle, ColorBuffer, RenDev->MainFramebuffer_Color_MSAA, GL_RENDERBUFFER, WGL_ACCESS_READ_WRITE_NV);
		RenDev->D3DDepthStencilViewInteropHandle = wglDXRegisterObjectNV(RenDev->D3DDeviceInteropHandle, DepthStencilBuffer, RenDev->MainFramebuffer_Depth_MSAA, GL_RENDERBUFFER, WGL_ACCESS_READ_WRITE_NV);
	}
	else
	{
		RenDev->D3DColorViewInteropHandle = wglDXRegisterObjectNV(RenDev->D3DDeviceInteropHandle, ColorBuffer, RenDev->MainFramebuffer_Texture.Texture, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
		RenDev->D3DDepthStencilViewInteropHandle = wglDXRegisterObjectNV(RenDev->D3DDeviceInteropHandle, DepthStencilBuffer, RenDev->MainFramebuffer_Depth, GL_RENDERBUFFER, WGL_ACCESS_READ_WRITE_NV);
	}
	check(RenDev->D3DColorViewInteropHandle && "Couldn't register DXGI ColorView");
	check(RenDev->D3DDepthStencilViewInteropHandle && "Couldn't register DXGI DepthStencilView");	

	ID3D11Texture2D_Release(DepthStencilBuffer);

	D3D11_VIEWPORT ViewportDesc = {};
	ViewportDesc.TopLeftX = 0.f;
	ViewportDesc.TopLeftY = 0.f;
	ViewportDesc.Width = (float)RenDev->RequestedFramebufferWidth;
	ViewportDesc.Height = (float)RenDev->RequestedFramebufferHeight;
	ViewportDesc.MinDepth = 0.f;
	ViewportDesc.MaxDepth = 1.f;
	ID3D11DeviceContext_RSSetViewports(RenDev->D3DContext, 1, &ViewportDesc);

	return true;
}

void FOpenGLBase::LockDXGIFramebuffer()
{
	if (!RenDev->D3DDevice || !RenDev->D3DColorView || !RenDev->D3DDepthStencilView)
		return;

	/*HRESULT hr;
	ID3D11Texture2D* ColorBuffer;
	hr = IDXGISwapChain_GetBuffer(RenDev->DXGISwapChain, 0, *(_GUID*)(&IID_ID3D11Texture2D), (void**)&ColorBuffer);
	check(SUCCEEDED(hr) && "Couldn't get DXGI Swapchain color buffer handle");

	hr = ID3D11Device_CreateRenderTargetView(RenDev->D3DDevice, (ID3D11Resource*)ColorBuffer, NULL, &RenDev->D3DColorView);
	check(SUCCEEDED(hr) && "Couldn't create DXGI ColorView");

	ID3D11Texture2D_Release(ColorBuffer);

	RenDev->D3DColorViewInteropHandle = wglDXRegisterObjectNV(
		RenDev->D3DDeviceInteropHandle, 
		ColorBuffer, 
		RenDev->UsingFBOMultisampling() ? RenDev->MainFramebuffer_Color_MSAA : RenDev->MainFramebuffer_Texture.Texture, 
		RenDev->UsingFBOMultisampling() ? GL_RENDERBUFFER : GL_TEXTURE_2D, 
		WGL_ACCESS_READ_WRITE_NV);
	*/

	FLOAT Black[] = { 0.f, 0.f, 0.f, 1.f };
	ID3D11DeviceContext_OMSetRenderTargets(RenDev->D3DContext, 1, &RenDev->D3DColorView, RenDev->D3DDepthStencilView);
	ID3D11DeviceContext_ClearRenderTargetView(RenDev->D3DContext, RenDev->D3DColorView, Black);
	ID3D11DeviceContext_ClearDepthStencilView(RenDev->D3DContext, RenDev->D3DDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0, 0);

	HANDLE InteropHandles[] = { RenDev->D3DColorViewInteropHandle, RenDev->D3DDepthStencilViewInteropHandle };
	wglDXLockObjectsNV(RenDev->D3DDeviceInteropHandle, 2, InteropHandles);

}

bool FOpenGLBase::UnlockDXGIFramebuffer()
{
	if (!RenDev->D3DDevice || !RenDev->D3DColorView || !RenDev->D3DDepthStencilView)
		return false;

	HANDLE InteropHandles[] = { RenDev->D3DColorViewInteropHandle, RenDev->D3DDepthStencilViewInteropHandle };
	wglDXUnlockObjectsNV(RenDev->D3DDeviceInteropHandle, 2, InteropHandles);

	// stijn: idk what's going on, but this doesn't work.
	// SwapChain->Present only = black screen
	// SwapChain->Present + SwapBuffers = screen flickering
	// SwapBuffers only = works
	//check(SUCCEEDED(IDXGISwapChain_Present(RenDev->DXGISwapChain, 0, 0)));

	//ID3D11RenderTargetView_Release(RenDev->D3DColorView);
	return true;
}

#endif