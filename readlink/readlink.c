/*
 *
 * readlink -- resolve shell shortcuts
 *
 * $Id: readlink.c,v 1.2 2000-04-01 12:36:45 dds Exp $
 *
 * (C) Copyright 1999, 2000 Diomidis Spinellis
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

#include <windows.h>
#include <shlobj.h>
#include <shlguid.h>
#include <errno.h>
#include <stdio.h>


int 
readlink(const LPCSTR lpszLinkFile, LPSTR lpszPath, int bufsiz)
{
	HRESULT         hres;
	IShellLink     *psl;
	char            szGotPath[MAX_PATH];
	char            szDescription[MAX_PATH];
	WIN32_FIND_DATA wfd;
	IPersistFile   *ppf;
	WORD            wsz[MAX_PATH];

	errno = EINVAL;

	if (strcmp(lpszLinkFile + strlen(lpszLinkFile) - 4, ".lnk") != 0)
		return (-1);

	// Initialize the COM library
	hres = CoInitialize(NULL);
	if (!SUCCEEDED(hres)) {
		CoUninitialize();
		return (-1);
	}

	//Get a pointer to the IShellLink interface.
	hres = CoCreateInstance(&CLSID_ShellLink, NULL,
		       CLSCTX_INPROC_SERVER, &IID_IShellLink, &psl);
	if (!SUCCEEDED(hres)) {
		CoUninitialize();
		return (-1);
	}

	//Get a pointer to the IPersistFile interface.
	hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);
	if (!SUCCEEDED(hres)) {
		psl->lpVtbl->Release(psl);
		CoUninitialize();
		return (-1);
	}

	//Ensure that the string is Unicode.
	MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, wsz, MAX_PATH);

	//Load the shortcut.
	hres = ppf->lpVtbl->Load(ppf, wsz, STGM_READ);
	if (!SUCCEEDED(hres))
		goto release_fail;

	//Resolve the link.
	hres = psl->lpVtbl->Resolve(psl, (HWND)0, SLR_NO_UI | SLR_ANY_MATCH);
	if (!SUCCEEDED(hres))
		goto release_fail;

	//Get the path to the link target.
	hres = psl->lpVtbl->GetPath(psl, szGotPath,
		MAX_PATH, (WIN32_FIND_DATA *) & wfd, SLGP_SHORTPATH);
	if (!SUCCEEDED(hres))
		goto release_fail;
		
	// Get the description of the target.
	hres = psl->lpVtbl->GetDescription(psl, szDescription, MAX_PATH);
	if (!SUCCEEDED(hres))
		goto release_fail;

	if (strlen(szGotPath) + 1 > bufsiz)
		szGotPath[bufsiz - 1] = 0;
	lstrcpy(lpszPath, szGotPath);

	//Release the pointer to the IPersistFile interface.
	ppf->lpVtbl->Release(ppf);
	//Release the pointer to the IShellLink interface.
	psl->lpVtbl->Release(psl);
	//Finish with the COM library
	CoUninitialize();
	//Malformed link files are sometimes silently resolved returning
	//an empty path
	return (*lpszPath ? 0 : -1);

release_fail:
	// Release all resource and return fail
	ppf->lpVtbl->Release(ppf);
	psl->lpVtbl->Release(psl);
	CoUninitialize();
	return (-1);
}

#ifdef TEST
main(int argc, char *argv[])
{
	static char     path[1024];
	int             r;

	r = readlink(argv[1], path, sizeof(path));
	printf("r = %d, link=[%s], path=[%s], \n", r, argv[1], path);
}
#endif /* TEST */

#ifdef APP
main(int argc, char *argv[])
{
	static char     path[1024];
	int             r;

	if (argc != 2 || *argv[1] == '-') {
		fprintf(stderr, 
		"readlink - resolve shell shortcuts.  $Revision: 1.2 $\n"
		"(C) Copyright 1999, 2000 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: %s link-file\n", argv[0]);
		return (1);
	}
	if ((r = readlink(argv[1], path, sizeof(path))) == -1) {
		return (1);
	} else {
		printf("%s\n", path);
		return (0);
	}
}
#endif /* APP */
