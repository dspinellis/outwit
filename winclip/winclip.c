/*
 * Copy/Paste the Windows Clipboard
 *
 * (C) Copyright 1994-1999 Diomidis Spinellis
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
 * $Id: winclip.c,v 1.8 2000-04-01 12:32:26 dds Exp $
 *
 */

#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>

static void
error(char *s)
{
	LPVOID lpMsgBuf;
	char buff[1024];

	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);
	fprintf(stderr, "%s: %s\n", s, lpMsgBuf);
	LocalFree(lpMsgBuf);
	exit(1);
}

void
usage(void)
{
	fprintf(stderr, 
		"winclip - copy/Paste the Windows Clipboard.  $Revision: 1.8 $\n"
		"(C) Copyright 1994, 2000 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: winclip -c|-p\n");
	exit(1);
}

// I/O chunk
#define CHUNK 1024

main(int argc, char *argv[])
{
	HANDLE hglb;
	char *b, *bp;		/* Buffer, pointer to buffer insertion point */
	size_t bsiz, remsiz;	/* Buffer, size, remaining size */
	size_t total;		/* Total read */
	int n;

	if (argc != 2)
		usage();


	if (strcmp(argv[1], "-p") == 0) {
		/*
		 * Paste the Windows clipboard to standard output
		 */
		if (!OpenClipboard(NULL))
			error("Unable to open clipboard");
		if (IsClipboardFormatAvailable(CF_OEMTEXT)) {
			/* Clipboard contains OEM text; copy it */
			hglb = GetClipboardData(CF_OEMTEXT);
			if (hglb != NULL) { 
				setmode(1, O_BINARY);
				printf("%s", hglb);
			}
			CloseClipboard(); 
			return (0);
		} else if (IsClipboardFormatAvailable(CF_HDROP)) {
			/* Clipboard contains file names; print them */
			hglb = GetClipboardData(CF_HDROP);
			if (hglb != NULL) { 
				int nfiles, i;
				char fname[4096];

				nfiles = DragQueryFile(hglb, 0xFFFFFFFF, NULL, 0);
				for (i = 0; i < nfiles; i++) {
					DragQueryFile(hglb, i, fname, sizeof(fname));
					printf("%s\n", fname);
				}
			}
			CloseClipboard(); 
			return (0);
		} else if (IsClipboardFormatAvailable(CF_BITMAP)) {
			/* Clipboard contains a bitmap; dump it */
			hglb = GetClipboardData(CF_BITMAP);
			if (hglb != NULL) { 
				SIZE dim;
				BITMAP bmp;
				HDC hdc;
				int x, y;
				COLORREF cref;
				HGDIOBJ oldobj;

				if (!GetObject(hglb, sizeof(BITMAP), &bmp))
					error("Unable to get bitmap dimensions");
				_setmode(_fileno(stdout), _O_BINARY );
				printf("P6\n%d %d\n255\n", bmp.bmWidth, bmp.bmHeight);
				if ((hdc = CreateCompatibleDC(NULL)) == NULL)
					error("Unable to create a compatible device context");
				if ((oldobj = SelectObject(hdc, hglb)) == NULL)
					error("Unable to select object");
				for (y = 0; y < bmp.bmHeight; y++)
					for (x = 0; x < bmp.bmWidth; x++) {
						cref = GetPixel(hdc, x, y);
						putchar(GetRValue(cref));
						putchar(GetGValue(cref));
						putchar(GetBValue(cref));
					}
				SelectObject(hdc, oldobj);
				DeleteDC(hdc);
			}
			CloseClipboard(); 
			return (0);
		} else {
			CloseClipboard(); 
			error("The clipboard does not contain text or files");
		}
	} else if (strcmp(argv[1], "-c") == 0) {
		/*
		 * Copy our input to the Windows Clipboard
		 */
		if (!OpenClipboard(NULL))
			error("Unable to open the clipboard");
		if (!EmptyClipboard()) {
			CloseClipboard(); 
			error("Unable to empty the clipboard");
		}

		// Read stdin into buffer
		setmode(0, O_BINARY);
		bp = b = malloc(remsiz = bsiz = CHUNK);
		if (b == NULL) {
			CloseClipboard(); 
			fprintf(stderr, "winclip: Unable to allocate %d bytes of memory\n", bsiz);
			return (1);
		}
		total = 0;
		while ((n = read(0, bp, remsiz)) > 0) {
			remsiz -= n;
			total += n;
			if (remsiz < CHUNK) {
				b = realloc(b, bsiz *= 2);
				if (b == NULL) {
					CloseClipboard(); 
					fprintf(stderr, "winclip: Unable to allocate %d bytes of memory\n", bsiz);
					return (1);
				}
				remsiz = bsiz - total;
				bp = b + total;
			}
		}
		hglb = GlobalAlloc(GMEM_DDESHARE, total + 1);
		if (hglb == NULL) { 
			CloseClipboard(); 
			error("Unable to allocate clipboard memory");
		}
		memcpy(hglb, b, total);
		((char *)hglb)[total] = '\0';
		SetClipboardData(CF_OEMTEXT, hglb); 
		CloseClipboard(); 
		return (0);
	} else
		usage();
	return (0);
}
