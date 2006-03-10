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
 * $Id: imageload.cpp,v 1.2 2006-03-10 12:48:12 dds Exp $
 *
 */

#include <stdio.h>
#include <windows.h>
#include <olectl.h>
#include <ole2.h>

extern "C" HBITMAP
image_load(FILE * fp)
{
	// Allocate memory and load the image
	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	HGLOBAL memory = GlobalAlloc(GPTR, fsize);
	if (!memory)
		return NULL;
	fread((void *)memory, 1, fsize, fp);

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
