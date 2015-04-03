/*
 * Load an image from a file using the Windows codecs
 *
 * (C) Copyright 2006-2015 Diomidis Spinellis
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment(lib, "WindowsCodecs.lib")
#include <atlbase.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <windows.h>
#include <atlbase.h>

// Return a string representing the specified Windows error
static string
windowsError(int err)
{
	LPVOID lpMsgBuf;

	if (!FormatMessage(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM |
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    err,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL
	)) {
		ostringstream s;
		// See http://msdn.microsoft.com/en-us/library/windows/desktop/ee719669%28v=vs.85%29.aspx
		s << "Unknown error " << err << " 0x" << hex << err;
		return s.str();
	}

	string r((char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return r;
}

static HGLOBAL memory = 0;

// Exit with an error if the specified Windows API function fails
#define CHECK(x) do { \
	int ret; \
	if ((ret = (x)) != S_OK) { \
		fprintf(stderr, "Cannot process image file: " #x ": %s", windowsError(ret).c_str()); \
		if (memory) \
			GlobalFree(memory); \
		exit(1); \
	} \
} while(0)


// Convert stream data into a bitmap and return it
static HBITMAP
ImageReadStream(IStream *stream)
{

	int ret;

	switch (ret = CoInitializeEx(NULL, COINIT_MULTITHREADED)) {
	case S_OK:
	case S_FALSE:	// Already initialized
		break;
	default:
		CHECK(("CoInitialize", 0));
	}

	CComPtr<IWICImagingFactory> factory;
	CHECK(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));

	CComPtr<IWICBitmapDecoder> decoder;
	CHECK(factory->CreateDecoderFromStream(stream, 0,  WICDecodeMetadataCacheOnDemand, &decoder));

	CComPtr<IWICBitmapFrameDecode> frame;
	CHECK(decoder->GetFrame(0, &frame));
	UINT width = 0;
	UINT height = 0;
	CHECK(frame->GetSize(&width, &height));

	CComPtr<IWICFormatConverter> format_converter;
	CHECK(factory->CreateFormatConverter(&format_converter));

	// Initialize the format converter.
	CHECK(format_converter->Initialize(
		frame,                  	 // Input source to convert
		GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
		WICBitmapDitherTypeNone,         // Specified dither pattern
		NULL,                            // Specify a particular palette
		0.f,                             // Alpha threshold
		WICBitmapPaletteTypeCustom       // Palette translation type
	));


	/*
	 * Convert the passed IWICBitmapSource into a Windows bitmap
	 * and return it.
	 */
	HBITMAP res = 0;

	unsigned int stride = (width * 32 + 7) / 8;
	unsigned int buffer_size = stride * height;

	BYTE* buffer = (BYTE*)malloc(buffer_size);
	CHECK(format_converter->CopyPixels(0, stride, buffer_size, buffer));


	BITMAPINFOHEADER bh;

	bh.biSize = sizeof(bh);
	bh.biWidth = width;
	bh.biHeight = -(int)height;	// Top-down DIB
	bh.biPlanes = 1;
	bh.biBitCount = 32;
	bh.biCompression = BI_RGB;
	bh.biSizeImage =
	bh.biXPelsPerMeter =
	bh.biYPelsPerMeter =
	bh.biClrUsed =
	bh.biClrImportant = 0;

	BITMAPINFO bi;

	bi.bmiHeader = bh;

	res = CreateDIBitmap(::GetDC(NULL), &bh, CBM_INIT, buffer, &bi, DIB_RGB_COLORS);

	free(buffer);

	return res;
}

extern "C" HBITMAP
image_load(FILE * fp)
{
	long fsize, n;
	struct stat sb;

	// Read it as a binary file
	if (_setmode(_fileno(fp), _O_BINARY) == -1)
		return NULL;

	// Allocate memory and load the image
	if (fstat(fileno(fp), &sb) == -1 || (sb.st_mode & _S_IFREG) == 0) {
		// Non-seekable stream; read by adjusting memory
		long n, currsize = 4096, currptr = 0;
		if ((memory = GlobalAlloc(GPTR, currsize)) == NULL)
			return NULL;
		const int BLOCK = 4096;
		while ((n = fread((char *)memory + currptr, 1, BLOCK, fp)) == BLOCK) {
			currptr += BLOCK;
			if (currptr >= currsize) {
				currsize *= 2;
				if ((memory = GlobalReAlloc(memory, currsize, GMEM_MOVEABLE)) == NULL)
					return NULL;
			}
		}
		if (ferror(fp))
			return NULL;
		fsize = currptr + n;
	} else {
		fsize = sb.st_size;
		if ((memory = GlobalAlloc(GPTR, fsize)) == NULL)
			return NULL;
		if ((n = fread(memory, 1, fsize, fp)) != fsize)
			return NULL;
	}

	int x, y, maxval, nchars;
	if (sscanf((const char *)memory, "P6\n%d %d\n%d\n%n", &x, &y, &maxval, &nchars) == 3 && maxval == 255) {
		// Convert PPM into a Windows bitmap
		BITMAPINFO info;
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biWidth = x;
		info.bmiHeader.biHeight = 0 - y;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 24;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biSizeImage = 0;
		info.bmiHeader.biXPelsPerMeter = 2834;
		info.bmiHeader.biXPelsPerMeter = 2834;
		info.bmiHeader.biClrUsed = 0;
		info.bmiHeader.biClrImportant = 0;
		// Swap the data to Windows format
		int swap;
		char *data = (char *)memory + nchars;
		int len = 3 * x * y;
		for (int i = 0; i < len; i += 3) {
			swap = data[i + 1];
			data[i + 1] = data[i + 0];
			data[i + 0] = swap;
		}
		printf("Bitmap %d %d %d %d\n", x, y, nchars, len);
		HBITMAP hb = CreateDIBitmap(GetDC(NULL), &info.bmiHeader, CBM_INIT, data, &info, 0);
		printf("%x\n", hb);
		return hb;
	} else {
		// Load the image with Windows codecs
		LPSTREAM stream = NULL;
		CHECK(CreateStreamOnHGlobal(memory, TRUE, &stream));
		HBITMAP	bitmap = ImageReadStream(stream);
		// Free resources; copy the image to an allocated bitmap
		GlobalFree(memory);
		HBITMAP	hBB = (HBITMAP)CopyImage(bitmap, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
		return hBB;
	}
}
