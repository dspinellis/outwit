/*
 * Print the properties of an OLE structured storage document
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
 *
 * $Id: docprop.cpp,v 1.5 2000-04-01 12:39:18 dds Exp $
 *
 */

#include <stdio.h>
#include <windows.h>
#include <ole2.h>

/* Document properties are stored in this linked list */
struct s_nameval {
	char *name;		/* Property name */
	char *val;		/* Property value */
	struct s_nameval *next;	/* Next in list */
} *nv;

/* Property identifiersi to retrieve from documents */
struct pidsiStruct {
	char           *name;
	long            pidsi;
};

// FMTID_SummaryInformation
struct pidsiStruct summary_pidsi[] = {
/* Array of PIDSI 's you are interested in. */
	{ "Title", PIDSI_TITLE }              , /* VT_LPSTR */
	{ "Subject", PIDSI_SUBJECT }          , /* ... */
	{ "Author", PIDSI_AUTHOR }            ,
	{ "Keywords", PIDSI_KEYWORDS }        ,
	{ "Comments", PIDSI_COMMENTS }        ,
	{ "Template", PIDSI_TEMPLATE }        ,
	{ "LastAuthor", PIDSI_LASTAUTHOR }    ,
	{ "Revision Number", PIDSI_REVNUMBER },
	{ "Edit Time", PIDSI_EDITTIME }       , /* VT_FILENAME(UTC)  */
	{ "Last printed", PIDSI_LASTPRINTED } , /* ... */
	{ "Created", PIDSI_CREATE_DTM }       ,
	{ "Last Saved", PIDSI_LASTSAVE_DTM }  ,
	{ "Page Count", PIDSI_PAGECOUNT }     , /* VT_I4 */
	{ "Word Count", PIDSI_WORDCOUNT }     , /* ... */
	{ "Char Count", PIDSI_CHARCOUNT }     ,
	{ "Thumpnail", PIDSI_THUMBNAIL }      , /* VT_CF */
	{ "AppName", PIDSI_APPNAME }          , /* VT_LPSTR */
	{ "Doc Security", PIDSI_DOC_SECURITY }, /* VT_I4 */
	{ 0, 0 }
};

/* The constants are not (yet?) defined in MSVC */
// FMTID_DocSummaryInformation
struct pidsiStruct docsummary_pidsi[] = {
	{ "Category", 0x00000002 /* PIDDSI_CATEGORY */ },		//  VT_LPSTR 
	{ "PresentationTarget", 0x00000003 /* PIDDSI_PRESFORMAT */ },	//  VT_LPSTR 
	{ "Bytes", 0x00000004 /* PIDDSI_BYTECOUNT */ },			//  VT_I4 
	{ "Lines", 0x00000005 /* PIDDSI_LINECOUNT */ },			//  VT_I4 
	{ "Paragraphs", 0x00000006 /* PIDDSI_PARCOUNT */ },		//  VT_I4 
	{ "Slides", 0x00000007 /* PIDDSI_SLIDECOUNT */ },		//  VT_I4 
	{ "Notes", 0x00000008 /* PIDDSI_NOTECOUNT */ },			//  VT_I4 
	{ "HiddenSlides", 0x00000009 /* PIDDSI_HIDDENCOUNT */ },		//  VT_I4 
	{ "MMClips", 0x0000000a /* PIDDSI_MMCLIPCOUNT */ },		//  VT_I4 
	{ "ScaleCrop", 0x0000000b /* PIDDSI_SCALE */ },			//  VT_BOOL 
	{ "HeadingPairs", 0x0000000c /* PIDDSI_HEADINGPAIR */ },		//  VT_VARIANT | VT_VECTOR 
	{ "TitlesofParts", 0x0000000d /* PIDDSI_DOCPARTS */ },		//  VT_VECTOR | (VT_LPSTR) 
	{ "Manager", 0x0000000e /* PIDDSI_MANAGER */ },			//  VT_LPSTR 
	{ "Company", 0x0000000f /* PIDDSI_COMPANY */ },			//  VT_LPSTR 
	{ "LinksUpToDate", 0x00000010 /* PIDDSI_LINKSDIRTY */ },		//VT_BOOL
	{ 0, 0 }
};


/*
 * Return a malloced copy of a PROPVARIANT type or NULL
 */
char *
propvariant_string(PROPVARIANT * pPropVar, bool relative_time)
{
	char buff[4096];

	if (pPropVar->vt & VT_ARRAY)
		return (NULL);
	if (pPropVar->vt & VT_BYREF)
		return (NULL);
	/* Switch types. */
	switch (pPropVar->vt) {
	case VT_EMPTY:
		return (NULL);
	case VT_NULL:
		return (NULL);
	case VT_BLOB:
		return (NULL);
	case VT_BOOL:
	       return (strdup(pPropVar->boolVal ? "TRUE" : "FALSE"));
	case VT_I2:
		return (strdup((sprintf(buff, "%d", (int) pPropVar->iVal), buff)));
	case VT_I4:
		return (strdup((sprintf(buff, "%d", (int) pPropVar->lVal), buff)));
	case VT_R4:
		return (strdup((sprintf(buff, "%.2lf", (double) pPropVar->fltVal), buff)));
	case VT_R8:
		return (strdup((sprintf(buff, "%.2lf", (double) pPropVar->dblVal), buff)));
	case VT_BSTR:
		/* OLE Automation string. */
		{
			/* Translate into ASCII. */
			char           *pbstr = (char *) pPropVar->bstrVal;
			int             i = wcstombs(
			 buff, pPropVar->bstrVal, *((DWORD *) (pbstr - 4)));
			buff[i] = 0;
			return (strdup(buff));
		}
	case VT_LPSTR:
		return (strdup(pPropVar->pszVal));
	case VT_FILETIME:
		{
			FILETIME        lft;
			FileTimeToLocalFileTime(&pPropVar->filetime, &lft);

			if (relative_time) {
				ULARGE_INTEGER t;

				t.LowPart = lft.dwLowDateTime;
				t.HighPart = lft.dwHighDateTime;
				sprintf(buff, "%d", t.QuadPart / 10000000);
				// lst.wYear -= 1601;
				// lst.wMonth--;
				// lst.wDay--;
			} else {
				SYSTEMTIME      lst;

				FileTimeToSystemTime(&lft, &lst);
				sprintf(buff, "%4d/%02d/%02d %02d:%02d:%02d",
					lst.wYear, lst.wMonth, lst.wDay, 
					lst.wHour, lst.wMinute, lst.wSecond);
			}
			return (strdup(buff));
		}
	case VT_CF:
		/* Clipboard format. */
		return (NULL);
	default:
		return (NULL);
	}
}

/*
 * Append built-in properties for the given format identifier from
 * the set specified to the nv linked list
 */ 
void
get_builtin_props(IPropertySetStorage * pPropSetStg, REFFMTID fmid, struct pidsiStruct pidsiArr[])
{
	IPropertyStorage *pPropStg = NULL;
	HRESULT         hr;
	struct s_nameval *nvp;

	/* Open summary information, getting an IpropertyStorage. */
	hr = pPropSetStg->Open(fmid, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
	if (FAILED(hr))
		return;

	int nPidsi = 0;
	for (nPidsi = 0; pidsiArr[nPidsi].name; nPidsi++)
			;

	PROPSPEC * pPropSpec = new PROPSPEC[nPidsi];
	PROPVARIANT    *pPropVar = new PROPVARIANT[nPidsi];

	for (int i = 0; i < nPidsi; i++) {
		ZeroMemory(&pPropSpec[i], sizeof(PROPSPEC));
		pPropSpec[i].ulKind = PRSPEC_PROPID;
		pPropSpec[i].propid = pidsiArr[i].pidsi;
	}

	hr = pPropStg->ReadMultiple(nPidsi, pPropSpec, pPropVar);

	if (!FAILED(hr))
		for (i = 0; i < nPidsi; i++) {
			nvp = new s_nameval;
			nvp->name = strdup(pidsiArr[i].name);
			nvp->val = propvariant_string(pPropVar + i, pidsiArr[i].pidsi == PIDSI_EDITTIME);
			nvp->next = nv;
			nv = nvp;
		}

	delete[] pPropVar;
	delete[] pPropSpec;
	pPropStg->Release();
}

/*
 * Append custom properties to the nv linked list
 */ 
void
get_custom_props(IPropertySetStorage * pPropSetStg)
{
	IPropertyStorage *pPropStg = NULL;
	HRESULT         hr;
	IEnumSTATPROPSTG *pEnumProp;

	hr = pPropSetStg->Open(FMTID_UserDefinedProperties,
			       STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
	if (FAILED(hr))
		return;

	hr = pPropStg->Enum(&pEnumProp);
	if (FAILED(hr)) {
		pPropStg->Release();
		return;
	}
	/* Enumerate properties. */
	STATPROPSTG     sps;
	ULONG           fetched;
	PROPSPEC        propSpec[1];
	PROPVARIANT     propVar[1];
	while (pEnumProp->Next(1, &sps, &fetched) == S_OK) {
		ZeroMemory(&propSpec[0], sizeof(PROPSPEC));
		propSpec[0].ulKind = PRSPEC_PROPID;
		propSpec[0].propid = sps.propid;

		hr = pPropStg->ReadMultiple(1, &propSpec[0], &propVar[0]);
		if (!FAILED(hr)) {
			/* Translate Prop name into ASCII. */
			char            dbcs[1024];
			char           *pbstr = (char *) sps.lpwstrName;
			int             i = wcstombs(dbcs, sps.lpwstrName,
						  *((DWORD *) (pbstr - 4)));
			dbcs[i] = 0;

			struct s_nameval *nvp = new s_nameval;
			nvp->name = strdup(dbcs);
			nvp->val = propvariant_string(propVar, FALSE);
			nvp->next = nv;
			nv = nvp;
		}
	}

	pEnumProp->Release();
	pPropStg->Release();
}

/*
 * Append custom and build-in properties of a compound document
 * to the nv list
 */
void
get_props(char *filename)
{
	WCHAR           wcFilename[1024];
	int             i = mbstowcs(wcFilename, filename, strlen(filename));
	wcFilename[i] = 0;

	IStorage       *pStorage = NULL;
	IPropertySetStorage *pPropSetStg = NULL;
	HRESULT         hr;

	hr =::StgOpenStorage(wcFilename, NULL,
		       STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);

	if (FAILED(hr))
		return;

	hr = pStorage->QueryInterface(IID_IPropertySetStorage, (void **) &pPropSetStg);
	if (FAILED(hr)) {
		pStorage->Release();
		return;
	}

	get_builtin_props(pPropSetStg, FMTID_SummaryInformation, summary_pidsi);
	get_builtin_props(pPropSetStg, FMTID_DocSummaryInformation, docsummary_pidsi);
	get_custom_props(pPropSetStg);

	pPropSetStg->Release();
	pStorage->Release();
}

static void
usage(char *s)
{
	int i;

	fprintf(stderr, 
		"docprop - print the properties of an OLE structured storage document.  $Revision: 1.5 $\n"
		"(C) Copyright 1999, 2000 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"Usage: %s [-f format] filename ...\n"
		"\tFormat is a string with embedded property names in braces\n"
		"\t(e.g. {variable}) and C escape codes.\n"
		"\tThe following (and all user-defined) property names can be used:\n", s);
	fprintf(stderr, "\t{Filename}");
	for (i = 0; summary_pidsi[i].name; i++)
		fprintf(stderr, ", {%s}", summary_pidsi[i].name);
	for (i = 0; docsummary_pidsi[i].name; i++)
		fprintf(stderr, ", {%s}", docsummary_pidsi[i].name);
	fputc('\n', stderr);
	exit(1);
}

void
print_all(void)
{
	struct s_nameval *nvp;

	for (nvp = nv; nvp; nvp = nvp->next) {
		printf("%s:", nvp->name);
		if (nvp->val)
			printf("%s", nvp->val);
		putchar('\n');
	}
}

/* Output format */
char *fmt = NULL;

/*
 * Output the format string performing variable subsitutions
 */
void
print_fmt(void)
{
	char *s;
	char varname[1024];
	int varidx;
	enum e_state {NORMAL, BACKSLASH, VARNAME} state = NORMAL;
	struct s_nameval *nvp;


	for (s = fmt; *s; s++)
		switch (state) {
		case NORMAL:
			if (*s == '\\')
				state = BACKSLASH;
			else if (*s == '{') {
				state = VARNAME;
				varidx = 0;
			} else
				putchar(*s);
			break;
		case BACKSLASH:
			switch (*s) {
			case 'a': putchar('\a'); break;
			case 'b': putchar('\b'); break;
			case 'f': putchar('\f'); break;
			case 't': putchar('\t'); break;
			case 'r': putchar('\r'); break;
			case 'n': putchar('\n'); break;
			case 'v': putchar('\v'); break;
			default: putchar(*s); break;
			}
			state = NORMAL;
			break;
		case VARNAME:
			if (*s == '}') {
				varname[varidx] = '\0';
				for (nvp = nv; nvp; nvp = nvp->next)
					if (strcmp(nvp->name, varname) == 0) {
						if (nvp->val)
							printf("%s", nvp->val);
						break;
					}
				if (!nvp) {
					fprintf(stderr, "Unknown variable name %s\n", varname);
					exit(1);
				}
				state = NORMAL;
			} else
				varname[varidx++] = *s;
			break;
		}
}


void
free_nv(void)
{
	struct s_nameval *nvp, *next;

	for (nvp = nv; nvp; nvp = next) {
		delete nvp->name;
		if (nvp->val)
			delete nvp->val;
		next = nvp->next;
		delete nvp;
	}
	nv = NULL;
}

int
main(int argc, char **argv)
{
	int optind = 1;
	int i;
	struct s_nameval *nvp;

	if (argc < 2)
		usage(argv[0]);

	if (strcmp(argv[1], "-f") == 0) {
		if (argc < 4)
			usage(argv[0]);
		fmt = argv[2];
		optind = 3;
	}

	if (*argv[optind] == '-')
		usage(argv[0]);

	for (i = optind; i < argc; i++) {
		get_props(argv[i]);
		if (!nv) {
			fprintf(stderr, "Unable to get document properties for %s\n", argv[i]);
			continue;
		}
		nvp = new s_nameval;
		nvp->name = strdup("Filename");
		nvp->val = strdup(argv[i]);
		nvp->next = nv;
		nv = nvp;
		if (!fmt)
			print_all();
		else
			print_fmt();
		free_nv();
	}
	return (0);
}
