/*
 * Copy/Paste the Windows Clipboard
 *
 * (C) Copyright 1994-2002 Diomidis Spinellis
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
 * $Id: winclip.c,v 1.13 2002-01-03 13:01:10 dds Exp $
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
		"winclip - copy/Paste the Windows Clipboard.  $Revision: 1.13 $\n"
		"(C) Copyright 1994-2002 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: winclip [-w|u] -c|-p [filename]\n");
	exit(1);
}

// I/O chunk
#define CHUNK 1024

int getopt(int argc, char *argv[], char *optstring);
extern char	*optarg;	/* Global argument pointer. */
extern int	optind;		/* Global argv index. */

main(int argc, char *argv[])
{
	HANDLE hglb;
	char *b, *bp;		/* Buffer, pointer to buffer insertion point */
	size_t bsiz, remsiz;	/* Buffer, size, remaining size */
	size_t total;		/* Total read */
	int n;
	enum {O_NONE, O_COPY, O_PASTE} operation = O_NONE;
	unsigned int textfmt = CF_OEMTEXT;
	int c;
	FILE *iofile;		/* File to use for I/O */
	char *fname;		/* and its file name */

	while ((c = getopt(argc, argv, "uwcp")) != EOF)
		switch (c) {
		case 'u':
			if (textfmt != CF_OEMTEXT)
				usage();
			textfmt = CF_UNICODETEXT;
			break;
		case 'w':
			if (textfmt != CF_OEMTEXT)
				usage();
			textfmt = CF_TEXT;
			break;
		case 'c':
			if (operation != O_NONE)
				usage();
			operation = O_COPY;
			break;
		case 'p':
			if (operation != O_NONE)
				usage();
			operation = O_PASTE;
			break;
		case '?':
			usage();
		}
	if (argc - optind > 1)
		usage();

	switch (operation) {
	case O_PASTE:
		/*
		 * Paste the Windows clipboard to standard output
		 */
		if (!OpenClipboard(NULL))
			error("Unable to open clipboard");
		if (argc - optind == 1) {
			if ((iofile = fopen(fname = argv[optind], "w")) == NULL) {
				perror(argv[optind]);
				return (1);
			}
		} else {
			iofile = stdout;
			fname = "standard output";
		}
		if (IsClipboardFormatAvailable(textfmt)) {
			/* Clipboard contains text; copy it */
			hglb = GetClipboardData(textfmt);
			if (hglb != NULL) { 
				setmode(fileno(iofile), O_BINARY);
				fwprintf(iofile, L"%s", hglb);
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
					switch (textfmt) {
					case CF_TEXT:	/* Windows native */
						DragQueryFileA(hglb, i, fname, sizeof(fname));
						fprintf(iofile, "%s\n", fname);
						break;
					case CF_UNICODETEXT:
						DragQueryFileW(hglb, i, fname, sizeof(fname));
						setmode(fileno(iofile), O_BINARY);
						fwprintf(iofile, L"%s\n", fname);
						break;
					case CF_OEMTEXT:
						DragQueryFileA(hglb, i, fname, sizeof(fname));
						CharToOemBuff(fname, fname, strlen(fname));
						fprintf(iofile, "%s\n", fname);
						break;
					}
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
				setmode(fileno(iofile), O_BINARY );
				fprintf(iofile, "P6\n%d %d\n255\n", bmp.bmWidth, bmp.bmHeight);
				if ((hdc = CreateCompatibleDC(NULL)) == NULL)
					error("Unable to create a compatible device context");
				if ((oldobj = SelectObject(hdc, hglb)) == NULL)
					error("Unable to select object");
				for (y = 0; y < bmp.bmHeight; y++)
					for (x = 0; x < bmp.bmWidth; x++) {
						cref = GetPixel(hdc, x, y);
						putc(GetRValue(cref), iofile);
						putc(GetGValue(cref), iofile);
						putc(GetBValue(cref), iofile);
					}
				SelectObject(hdc, oldobj);
				DeleteDC(hdc);
			}
			CloseClipboard(); 
		} else {
			CloseClipboard(); 
			error("The clipboard does not contain text or files");
		}
		if (ferror(iofile)) {
			perror(fname);
			return (1);
		}
		break;
	case O_COPY:
		/*
		 * Copy our input to the Windows Clipboard
		 */
		if (argc - optind == 1) {
			if ((iofile = fopen(fname = argv[optind], "r")) == NULL) {
				perror(argv[optind]);
				return (1);
			}
		} else {
			iofile = stdin;
			fname = "standard input";
		}
		if (!OpenClipboard(NULL))
			error("Unable to open the clipboard");
		if (!EmptyClipboard()) {
			CloseClipboard(); 
			error("Unable to empty the clipboard");
		}

		// Read stdin into buffer
		setmode(fileno(iofile), O_BINARY);
		bp = b = malloc(remsiz = bsiz = CHUNK);
		if (b == NULL) {
			CloseClipboard(); 
			fprintf(stderr, "winclip: Unable to allocate %d bytes of memory\n", bsiz);
			return (1);
		}
		total = 0;
		while ((n = fread(bp, 1, remsiz, iofile)) > 0) {
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
		SetClipboardData(textfmt, hglb); 
		CloseClipboard(); 
		if (ferror(iofile)) {
			perror(fname);
			return (1);
		}
		break;
	case O_NONE:
		usage();
		break;
	}
	return (0);
}
