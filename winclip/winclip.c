/*
 * Copy/Paste the Windows Clipboard
 *
 * (C) Copyright 1994-2004 Diomidis Spinellis
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
 * $Id: winclip.c,v 1.23 2005-12-05 20:49:06 dds Exp $
 *
 */

#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <wchar.h>

/* Add some formats lacking in VC5 */
#ifndef CF_DIBV5
#define CF_DIBV5 17
#endif

/* Size of an I/O chunk */
#define CHUNK 1024

/* Flags affecting Unicode output */
/* Print byte order mark */
static int bom = 0;
/* output UTF instead of native wide characters */
static int multibyte = 0;
/* True if big-endian BOM */
static int big_endian = 0;

/* Other flags */
/* Text format */
static unsigned int textfmt = CF_OEMTEXT;
/* Language identifiers */
static int langid = LANG_NEUTRAL;
static int sublangid = SUBLANG_DEFAULT;

/* Program name */
char *argv0;

static void
error(char *s)
{
	LPVOID lpMsgBuf;

	FormatMessage(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM |
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL
	);
	fprintf(stderr, "%s: %s: %s\n", argv0, s, lpMsgBuf);
	LocalFree(lpMsgBuf);
	exit(1);
}

void
unicode_fputs(const wchar_t *str, FILE *iofile)
{
	setmode(fileno(iofile), O_BINARY);

	if (multibyte) {
		int wlen = wcslen(str);
		int blen = (wlen + 1) * 6;
		char *mbs = malloc(blen);
		if (mbs == NULL) {
			CloseClipboard();
			fprintf(stderr, "%s: Unable to allocate %d bytes of memory\n", argv0, blen);
			exit(1);
		}
		if (WideCharToMultiByte(CP_UTF8, 0, str, wlen, mbs, blen, NULL, NULL) == 0) {
			CloseClipboard();
			error("Error converting wide characters into a multi byte sequence");
		}
		if (bom)
			fprintf(iofile, "\xef\xbb\xbf");
		fprintf(iofile, "%s", mbs);
	} else {
		if (bom)
			fprintf(iofile, "\xff\xfe");
		fwprintf(iofile, L"%s\n", str);
	}
}

static char *usage_string =
	"usage: %s [-v|h] [-w|u|m] [-l lang] [-s sublang] [-b] -c|-p|-i [filename]\n";

void
help(void)
{
	printf(usage_string, argv0);
	printf( "\t-v Display version and copyright information\n"
		"\t-h Display this help message\n"
		"\t-c Copy to clipboard\n"
		"\t-p Paste from clipboard\n"
		"\t-i Print the type of the clipboard's contents\n"
		"\t-u Data to be copied / pasted is in Unicode format\n"
		"\t-m Unicode data is multi-byte\n"
		"\t-b Include BOM with Unicode data\n"
		"\t-w Data is in the Windows code page (OEM code page is the default)\n"
		"\t-l Specify the language for the data to be copied\n"
		"\t-s Specify the sublanguage for the data to be copied\n"
	);
	exit(0);
}

void
usage(void)
{
	fprintf(stderr, usage_string, argv0);
	fprintf(stderr, "Run %s -h for help\n", argv0);
	exit(1);
}

void
version(void)
{
	printf("%s - copy/paste the Windows clipboard.  $Revision: 1.23 $\n\n", argv0);
	printf( "(C) Copyright 1994-2005 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"
	);
	exit(0);
}

struct s_keyval {
	char *name;
	int val;
};

#include "lang.h"

/*
 * Return a value for the passed key.
 * If the value is not found, print the valid keys and exit.
 */
int
get_val(const struct s_keyval *kv, int len, const char *key)
{
	int i, olen = 0;

	for (i = 0; i < len; i++)
		if (stricmp(kv[i].name, key) == 0)
			return (kv[i].val);
	fprintf(stderr, "%s: invalid key [%s].  Valid keys are:\n", argv0, key);
	for (i = 0; i < len; i++) {
		fprintf(stderr, "%s", kv[i].name);
		if ((olen += strlen(kv[i].name)) > 60) {
			fputc('\n', stderr);
			olen = 0;
		} else
			fputc(' ', stderr);
	}
	fputc('\n', stderr);
	exit(1);
}

/*
 * Return the name of a predefined clipboard format name.
 * Return NULL if this name is not known.
 *
 * This must be the intended implementation of the function
 * GetPredefinedClipboardFormatName, inexplicably appearing in
 * Microsoft's example.
 */
static const char *
predefined_name(int n)
{
	static char buff[256];

	if (n >= CF_GDIOBJFIRST && n <= CF_GDIOBJLAST) {
		sprintf(buff, "CF_GDIOBJ%d", n);
		return buff;
	} else if (n >= CF_PRIVATEFIRST && n <= CF_PRIVATELAST) {
		sprintf(buff, "CF_PRIVATE%d", n);
		return buff;
	}
	switch (n) {
	case CF_BITMAP:		 return "CF_BITMAP";
	case CF_DIB:		 return "CF_DIB";
	case CF_DIBV5:		 return "CF_DIBV5";
	case CF_DIF:		 return "CF_DIF";
	case CF_DSPBITMAP:	 return "CF_DSPBITMAP";
	case CF_DSPENHMETAFILE:	 return "CF_DSPENHMETAFILE";
	case CF_DSPMETAFILEPICT: return "CF_DSPMETAFILEPICT";
	case CF_DSPTEXT:	 return "CF_DSPTEXT";
	case CF_ENHMETAFILE:	 return "CF_ENHMETAFILE";
	case CF_HDROP:		 return "CF_HDROP";
	case CF_LOCALE:		 return "CF_LOCALE";
	case CF_METAFILEPICT:	 return "CF_METAFILEPICT";
	case CF_OEMTEXT:	 return "CF_OEMTEXT";
	case CF_OWNERDISPLAY:	 return "CF_OWNERDISPLAY";
	case CF_PALETTE:	 return "CF_PALETTE";
	case CF_PENDATA:	 return "CF_PENDATA";
	case CF_RIFF:		 return "CF_RIFF";
	case CF_SYLK:		 return "CF_SYLK";
	case CF_TEXT:		 return "CF_TEXT";
	case CF_WAVE:		 return "CF_WAVE";
	case CF_TIFF:		 return "CF_TIFF";
	case CF_UNICODETEXT:	 return "CF_UNICODETEXT";
	}
	return NULL;
}

/* Display the clipboard's formats */
static void
info(void)
{
	int n;

	if (!OpenClipboard(NULL))
		error("Unable to open clipboard");
	for (n = EnumClipboardFormats(0); n; n = EnumClipboardFormats(n)) {
		char buff[1024];
		LPCSTR fmtname;
		if ((fmtname = predefined_name(n)) == NULL) {
			if (GetClipboardFormatName(n, buff, sizeof(buff)) == 0)
				error("Unable to get clipboard format name");
			fmtname = buff;
		}
		printf("%s\n", fmtname);
	}
	if (GetLastError() != ERROR_SUCCESS)
		error("Error enumerating clipboard formats");
	CloseClipboard();
}

/*
 * Paste the Windows clipboard to standard output
 */
static void
paste(const char *fname, FILE *iofile)
{
	HANDLE cb_hnd;		/* Clipboard memory handle */

	if (!OpenClipboard(NULL))
		error("Unable to open clipboard");
	if (IsClipboardFormatAvailable(textfmt)) {
		/* Clipboard contains text; copy it */
		cb_hnd = GetClipboardData(textfmt);
		if (cb_hnd != NULL) {
			setmode(fileno(iofile), O_BINARY);
			if (textfmt == CF_UNICODETEXT)
				unicode_fputs(cb_hnd, iofile);
			else
				fprintf(iofile, "%s", cb_hnd);
		}
		CloseClipboard();
		exit(0);
	} else if (IsClipboardFormatAvailable(CF_HDROP)) {
		/* Clipboard contains file names; print them */
		cb_hnd = GetClipboardData(CF_HDROP);
		if (cb_hnd != NULL) {
			int nfiles, i;
			char fname[4096];

			nfiles = DragQueryFile(cb_hnd, 0xFFFFFFFF, NULL, 0);
			for (i = 0; i < nfiles; i++) {
				switch (textfmt) {
				case CF_TEXT:	/* Windows native */
					DragQueryFileA(cb_hnd, i, fname, sizeof(fname));
					fprintf(iofile, "%s\n", fname);
					break;
				case CF_UNICODETEXT:
					DragQueryFileW(cb_hnd, i, (LPWSTR)fname, sizeof(fname));
					unicode_fputs((wchar_t *)fname, iofile);
					break;
				case CF_OEMTEXT:
					DragQueryFileA(cb_hnd, i, fname, sizeof(fname));
					CharToOemBuff(fname, fname, strlen(fname));
					fprintf(iofile, "%s\n", fname);
					break;
				}
			}
		}
		CloseClipboard();
		exit(0);
	} else if (IsClipboardFormatAvailable(CF_BITMAP)) {
		/* Clipboard contains a bitmap; dump it */
		cb_hnd = GetClipboardData(CF_BITMAP);
		if (cb_hnd != NULL) {
			BITMAP bmp;
			HDC hdc;
			int x, y;
			COLORREF cref;
			HGDIOBJ oldobj;

			if (!GetObject(cb_hnd, sizeof(BITMAP), &bmp))
				error("Unable to get bitmap dimensions");
			setmode(fileno(iofile), O_BINARY );
			fprintf(iofile, "P6\n%d %d\n255\n", bmp.bmWidth, bmp.bmHeight);
			if ((hdc = CreateCompatibleDC(NULL)) == NULL)
				error("Unable to create a compatible device context");
			if ((oldobj = SelectObject(hdc, cb_hnd)) == NULL)
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
		exit(1);
	}
}

/*
 * Copy our input to the Windows Clipboard
 */
static void
copy(const char *fname, FILE *iofile)
{
	char *b, *bp;		/* Buffer, pointer to buffer insertion point */
	HANDLE cb_hnd;		/* Clipboard memory handle */
	HANDLE lc_hnd;		/* Locale memory handle */
	size_t bsiz, remsiz;	/* Buffer, size, remaining size */
	size_t total;		/* Total read */
	int n;
	char *clipdata;		/* Pointer to the clipboard data */
	LCID *plcid;		/* Locale id pointer */

	if (!OpenClipboard(NULL))
		error("Unable to open the clipboard");
	if (!EmptyClipboard()) {
		CloseClipboard();
		error("Unable to empty the clipboard");
	}

	/* Read stdin into buffer */
	setmode(fileno(iofile), O_BINARY);
	bp = b = malloc(remsiz = bsiz = CHUNK);
	if (b == NULL) {
		CloseClipboard();
		fprintf(stderr, "%s: Unable to allocate %d bytes of memory\n", argv0, bsiz);
		exit(1);
	}
	total = 0;
	while ((n = fread(bp, 1, remsiz, iofile)) > 0) {
		remsiz -= n;
		total += n;
		if (remsiz < CHUNK) {
			b = realloc(b, bsiz *= 2);
			if (b == NULL) {
				CloseClipboard();
				fprintf(stderr, "%s: Unable to allocate %d bytes of memory\n", argv0, bsiz);
				exit(1);
			}
			remsiz = bsiz - total;
			bp = b + total;
		}
	}
	if (bom) {
		if (memcmp(b, "\xef\xbb\xbf", 3) == 0) {
			/* UTF-8 */
			textfmt = CF_UNICODETEXT;
			multibyte = 1;
			big_endian = 0;
		} else if (memcmp(b, "\xff\xfe", 2) == 0) {
			/* UTF-16 little endian (native) */
			textfmt = CF_UNICODETEXT;
			multibyte = 0;
			big_endian = 0;
		} else if (memcmp(b, "\xfe\xff", 2) == 0) {
			/* UTF-16 big endian */
			textfmt = CF_UNICODETEXT;
			multibyte = 0;
			big_endian = 1;
		}
	}
	if (textfmt == CF_UNICODETEXT && multibyte) {
		int b2siz = (total + 1) * 2;
		char *b2 = malloc(b2siz);
		int ret;

		if (b2 == NULL) {
			fprintf(stderr, "%s: Unable to allocate %d bytes of memory\n", argv0, b2siz);
			CloseClipboard();
			exit(1);
		}
		if ((ret = MultiByteToWideChar(CP_UTF8, 0, b, total, (LPWSTR)b2, b2siz)) == 0) {
			CloseClipboard();
			error("Error converting a multi byte sequence into wide characters");
		}
		total = ret * 2;
		b = b2;
	}
	/* Convert big to little endian */
	if (big_endian) {
		char tmp;
		unsigned i;

		for (i = 0; i < total; i++) {
			tmp = b[i];
			b[i] = b[i + 1];
			b[i + 1] = tmp;
		}
	}
	cb_hnd = GlobalAlloc(GMEM_MOVEABLE, total + 1);
	if (cb_hnd == NULL) {
		CloseClipboard();
		error("Unable to allocate clipboard memory");
	}
	clipdata = (char *)GlobalLock(cb_hnd);
	memcpy(clipdata, b, total);
	clipdata[total] = '\0';
	clipdata[total + 1] = '\0';
	GlobalUnlock(cb_hnd);
	SetClipboardData(textfmt, cb_hnd);
	/* Now set the locale */
	lc_hnd = GlobalAlloc(GHND, sizeof(LCID));
	if (lc_hnd == NULL) {
		CloseClipboard();
		error("Unable to allocate clipboard memory");
	}
	plcid = (LCID*)GlobalLock(lc_hnd);
	*plcid = MAKELCID(MAKELANGID(langid, sublangid), SORT_DEFAULT);
	GlobalUnlock(lc_hnd);
	SetClipboardData(CF_LOCALE, lc_hnd);
	CloseClipboard();
	if (ferror(iofile)) {
		perror(fname);
		exit(1);
	}
}


int getopt(int argc, char *argv[], char *optstring);
extern char	*optarg;	/* Global argument pointer. */
extern int	optind;		/* Global argv index. */

main(int argc, char *argv[])
{
	enum {O_NONE, O_COPY, O_PASTE, O_INFO} operation = O_NONE;
	int c;
	FILE *iofile;		/* File to use for I/O */
	char *fname;		/* and its file name */

	argv0 = argv[0];
	while ((c = getopt(argc, argv, "vhuwcpimbl:s:")) != EOF)
		switch (c) {
		case 'b':
			bom = 1;
			break;
		case 'm':
			if (textfmt != CF_OEMTEXT)
				usage();
			textfmt = CF_UNICODETEXT;
			multibyte = 1;
			break;
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
		case 'i':
			if (operation != O_NONE)
				usage();
			operation = O_INFO;
			break;
		case 'l':
			if (!optarg)
				usage();
			langid = get_val(lang, sizeof(lang) / sizeof(*lang), optarg);
			break;
		case 's':
			if (!optarg)
				usage();
			sublangid = get_val(sublang, sizeof(sublang) / sizeof(*sublang), optarg);
			break;
		case 'v':
			version();
			break;
		case 'h':
			help();
			break;
		case '?':
			usage();
		}
	if (argc - optind > 1)
		usage();

	switch (operation) {
	case O_INFO:
		info();
		break;
	case O_PASTE:
		if (argc - optind == 1) {
			if ((iofile = fopen(fname = argv[optind], "w")) == NULL) {
				perror(argv[optind]);
				return (1);
			}
		} else {
			iofile = stdout;
			fname = "standard output";
		}
		paste(fname, iofile);
		break;
	case O_COPY:
		if (argc - optind == 1) {
			if ((iofile = fopen(fname = argv[optind], "r")) == NULL) {
				perror(argv[optind]);
				return (1);
			}
		} else {
			iofile = stdin;
			fname = "standard input";
		}
		copy(fname, iofile);
		break;
	case O_NONE:
		usage();
		break;
	}
	return (0);
}
