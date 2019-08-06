
#include "stdafx.h"

#define COBJMACROS
#include <Objbase.h>
#include <wincodec.h>
#include <Windows.h>
#include <Winerror.h>
#include "wic.h"
#include "atlpath.h"


#pragma comment(lib, "Windowscodecs.lib")

HRESULT WriteBitmap(HBITMAP bitmap, const wchar_t* pathname) {

	HRESULT hr = S_OK;
	GUID ContainerFormat;

	CPath path(pathname);
	CString ext = path.GetExtension();
	if (ext == L".jpg")
		ContainerFormat = GUID_ContainerFormatJpeg;
	else if (ext == L".png")
		ContainerFormat = GUID_ContainerFormatPng;
	else if (ext == L".bmp")
		ContainerFormat = GUID_ContainerFormatBmp;
	else
		return E_FAIL;

	// (1) Retrieve properties from the source HBITMAP.
	BITMAP bm_info = { 0 };
	if (!GetObject(bitmap, sizeof(bm_info), &bm_info))
		hr = E_FAIL;

	CComPtr<IWICImagingFactory> pFactory;
	hr = pFactory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(hr))
		return hr;

	CComPtr<IWICBitmap> pBitmap;
	hr = pFactory->CreateBitmapFromHBITMAP(bitmap, NULL, WICBitmapIgnoreAlpha, &pBitmap);
	if (FAILED(hr))
		return hr;

	CComPtr<IWICStream> pStream;
	hr = pFactory->CreateStream(&pStream);
	if (FAILED(hr))
		return hr;

	hr = pStream->InitializeFromFilename(pathname, GENERIC_WRITE);
	if (FAILED(hr))
		return hr;

	CComPtr<IWICBitmapEncoder> pEncoder;
	hr = pFactory->CreateEncoder(ContainerFormat, NULL, &pEncoder);
	if (FAILED(hr))
		return hr;

	hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
	if (FAILED(hr))
		return hr;

	CComPtr<IWICBitmapFrameEncode> pFrame;
	hr = pEncoder->CreateNewFrame(&pFrame, NULL);
	if (FAILED(hr))
		return hr;

	hr = pFrame->Initialize(NULL);
	if (FAILED(hr))
		return hr;

	hr = pFrame->SetSize(bm_info.bmWidth, bm_info.bmHeight);
	if (FAILED(hr))
		return hr;

	GUID pixel_format = GUID_WICPixelFormat24bppBGR;
	hr = pFrame->SetPixelFormat(&pixel_format);
	if (FAILED(hr))
		return hr;

	hr = pFrame->WriteSource(pBitmap, NULL);
	if (FAILED(hr))
		return hr;

	hr = pFrame->Commit();
	if (FAILED(hr))
		return hr;

	hr = pEncoder->Commit();
	if (FAILED(hr))
		return hr;


	return hr;
}