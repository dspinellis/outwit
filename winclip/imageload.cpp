/*
 * Load an image from a file using OLE
 *
 * (C) Copyright 2006 Diomidis Spinellis
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
 * $Id: imageload.cpp,v 1.4 2006-03-10 18:34:00 dds Exp $
 *
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <windows.h>
#include <olectl.h>
#include <ole2.h>

extern "C" HBITMAP
image_load(FILE * fp)
{
	HGLOBAL memory;
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
		// Load the image with OLE magic
		LPSTREAM stream = NULL;
		if (CreateStreamOnHGlobal(memory, TRUE, &stream) != S_OK) {
			GlobalFree(memory);
			return NULL;
		}
		IPicture *picture;
		if (OleLoadPicture(stream, 0, 0, IID_IPicture, (void **)&picture) != S_OK) {
			stream->Release();
			GlobalFree(memory);
			return NULL;
		}

		// Free resources; copy the image to an allocated bitmap
		stream->Release();
		GlobalFree(memory);
		HBITMAP	bitmap = 0;
		picture->get_Handle((unsigned int *)&bitmap);
		HBITMAP	hBB = (HBITMAP)CopyImage(bitmap, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
		picture->Release();
		return hBB;
	}
}
