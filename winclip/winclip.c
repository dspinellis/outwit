/*
 * Copy/Paste the Windows Clipboard
 *
 * (C) Copyright 1998 Diomidis Spinellis
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
 * $Id: winclip.c,v 1.1 1999-06-02 07:02:32 dds Exp $
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


// I/O chunk
#define CHUNK 1024

main(int argc, char *argv[])
{
	HANDLE hglb;
	LPTSTR lptstr;
	char *b, *bp;		/* Buffer, pointer to buffer insertion point */
	size_t bsiz, remsiz;	/* Buffer, size, remaining size */
	size_t total;		/* Total read */
	int n;

	if (argc > 2 || (argc == 2 && *argv[1] == '-')) {
		fprintf(stderr, "$Id: winclip.c,v 1.1 1999-06-02 07:02:32 dds Exp $\n"
				"Copyright 1998 Diomidis Spinellis.  "
				"May be freely copied without modification.\n\n"
				"usage: winclip [filename]\n");
		return (1);
	}

	if (argc == 2) {
		/* Redirect stdin */
		close(0);
		if (open(argv[1], _O_BINARY | _O_RDONLY) < 0) {
			perror(argv[1]);
			return (1);
		}
	}

	if (isatty(0)) {
		/*
		 * We are the first process in the pipeline.
		 * Copy the Windows clipboard to standard output
		 */
		if (!OpenClipboard(NULL))
			error("Unable to open clipboard");
		if (!IsClipboardFormatAvailable(CF_OEMTEXT)) {
			CloseClipboard(); 
			error("The clipboard does not contain text");
		}
		hglb = GetClipboardData(CF_OEMTEXT); 
		if (hglb != NULL) { 
			lptstr = GlobalLock(hglb); 
			if (lptstr != NULL) {
				setmode(1, O_BINARY);
				printf("%s", lptstr);
			}
			GlobalUnlock(hglb); 
		}
		CloseClipboard(); 
		return (0);
	} else if (isatty(1)) {
		/*
		 * We are the last process in the pipeline.
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
			error("Unable to allocate clipboard memory");
			CloseClipboard(); 
			return (1);
		}
		lptstr = GlobalLock(hglb);
		memcpy(lptstr, b, total);
		lptstr[total] = '\0';
		GlobalUnlock(hglb); 
		SetClipboardData(CF_OEMTEXT, hglb); 
		CloseClipboard(); 
		return (0);
	} else {
		fprintf(stderr, "winclip must be first or last in a pipeline\n");
		return (1);
	}
	return (0);
}
