
#include <stdio.h>
#include <windows.h>
#include <ole2.h>


/* Dumps simple PROPVARIANT values. */
void
DumpPropVariant(PROPVARIANT * pPropVar)
{
	/* Don 't iterate arrays, just inform as an array. */
	if (pPropVar->vt & VT_ARRAY) {
		printf("(Array)\n");
		return;
	}
	/* Don 't handle byref for simplicity, just inform byref. */
	if (pPropVar->vt & VT_BYREF) {
		printf("(ByRef)\n");
		return;
	}
	/* Switch types. */
	switch (pPropVar->vt) {
	case VT_EMPTY:
		printf("(VT_EMPTY)\n");
		break;
	case VT_NULL:
		printf("(VT_NULL)\n");
		break;
	case VT_BLOB:
		printf("(VT_BLOB)\n");
		break;
	case VT_BOOL:
		printf("%s (VT_BOOL)\n",
		       pPropVar->boolVal ? "TRUE/YES" : "FALSE/NO");
		break;
	case VT_I2:
		/* 2 - byte signed int. */
		printf("%d (VT_I2)\n", (int) pPropVar->iVal);
		break;
	case VT_I4:
		/* 4 - byte signed int. */
		printf("%d (VT_I4)\n", (int) pPropVar->lVal);
		break;
	case VT_R4:
		/* 4 - byte real. */
		printf("%.2lf (VT_R4)\n", (double) pPropVar->fltVal);
		break;
	case VT_R8:
		/* 8 - byte real. */
		printf("%.2lf (VT_R8)\n", (double) pPropVar->dblVal);
		break;
	case VT_BSTR:
		/* OLE Automation string. */
		{
			/* Translate into ASCII. */
			char            dbcs[1024];
			char           *pbstr = (char *) pPropVar->bstrVal;
			int             i = wcstombs(
			 dbcs, pPropVar->bstrVal, *((DWORD *) (pbstr - 4)));
			dbcs[i] = 0;
			printf("%s (VT_BSTR)\n", dbcs);
		}
		break;
	case VT_LPSTR:
		/* Null - terminated string. */
		{
			printf("%s (VT_LPSTR)\n", pPropVar->pszVal);
		}
		break;
	case VT_FILETIME:
		{
			char           *dayPre[] =
			{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

			FILETIME        lft;
			FileTimeToLocalFileTime(&pPropVar->filetime, &lft);
			SYSTEMTIME      lst;
			FileTimeToSystemTime(&lft, &lst);

			printf("%02d:%02d.%02d %s, %s %02d/%02d/%d (VT_FILETIME)\n",
			 1 + (lst.wHour - 1) % 12, lst.wMinute, lst.wSecond,
			       (lst.wHour >= 12) ? "pm" : "am",
			       dayPre[lst.wDayOfWeek % 7],
			       lst.wMonth, lst.wDay, lst.wYear);
		}
		break;
	case VT_CF:
		/* Clipboard format. */
		printf("(Clipboard format)\n");

		break;
	default:
		/* Unhandled type, consult wtypes.h 's VARENUM structure. */
		printf("(Unhandled type: 0x%08lx)\n", pPropVar->vt);
		break;
	}
}

/* Dump 's built-in properties of a property storage. */
void
DumpBuiltInProps(IPropertySetStorage * pPropSetStg)
{
	printf("\n==================================================\n");
	printf("BuiltInProperties Properties...\n");
	printf("==================================================\n");

	IPropertyStorage *pPropStg = NULL;
	HRESULT         hr;

	/* Open summary information, getting an IpropertyStorage. */
	hr = pPropSetStg->Open(FMTID_SummaryInformation,
			       STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
	/* hr = pPropSetStg->Open(FMTID_UserDefinedProperties, */
	/* STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg); */
	if (FAILED(hr)) {
		printf("No Summary-Information.\n");
		return;
	}
	/* Array of PIDSI 's you are interested in. */
	struct pidsiStruct {
		char           *name;
		long            pidsi;
	}               pidsiArr[] = {
		{ "Title", PIDSI_TITLE }              , /* VT_LPSTR */
		{ "Subject", PIDSI_SUBJECT }              , /* ... */
		{ "Author", PIDSI_AUTHOR }              ,
		{ "Keywords", PIDSI_KEYWORDS }              ,
		{ "Comments", PIDSI_COMMENTS }              ,
		{ "Template", PIDSI_TEMPLATE }              ,
		{ "LastAuthor", PIDSI_LASTAUTHOR }              ,
		{ "Revision Number", PIDSI_REVNUMBER }              ,
		{ "Edit Time", PIDSI_EDITTIME }              , /* VT_FILENAME(UTC)  */
		{ "Last printed", PIDSI_LASTPRINTED }              , /* ... */
		{ "Created", PIDSI_CREATE_DTM }              ,
		{ "Last Saved", PIDSI_LASTSAVE_DTM }              ,
		{ "Page Count", PIDSI_PAGECOUNT }              , /* VT_I4 */
		{ "Word Count", PIDSI_WORDCOUNT }              , /* ... */
		{ "Char Count", PIDSI_CHARCOUNT }              ,
		{ "Thumpnail", PIDSI_THUMBNAIL }              , /* VT_CF */
		{ "AppName", PIDSI_APPNAME }              , /* VT_LPSTR */
		{ "Doc Security", PIDSI_DOC_SECURITY }              , /* VT_I4 */
		{ 0, 0 }
	};
	/* Count elements in pidsiArr. */
	int             nPidsi = 0;
	for (nPidsi = 0; pidsiArr[nPidsi].name; nPidsi++);

	/* Initialize PROPSPEC for the 	properties you  want. */
	                PROPSPEC * pPropSpec = new PROPSPEC[nPidsi];
	PROPVARIANT    *pPropVar = new PROPVARIANT[nPidsi];

	for (int i = 0; i < nPidsi; i++) {
		ZeroMemory(&pPropSpec[i], sizeof(PROPSPEC));
		pPropSpec[i].ulKind = PRSPEC_PROPID;
		pPropSpec[i].propid = pidsiArr[i].pidsi;
	}

	/* Read properties. */
	hr = pPropStg->ReadMultiple(nPidsi, pPropSpec, pPropVar);

	if (FAILED(hr)) {
		printf("IPropertyStg::ReadMultiple() failed w/error %08lx\n",
		       hr);
	} else {
		/* Dump properties. */
		for (i = 0; i < nPidsi; i++) {
			printf("%16s: ", pidsiArr[i].name);
			DumpPropVariant(pPropVar + i);
		}
	}

	/* De - allocate memory. */
	delete[] pPropVar;
	delete[] pPropSpec;

	/* Release obtained interface. */
	pPropStg->Release();

}

/* Dump 's custom properties of a property storage. */
void
DumpCustomProps(IPropertySetStorage * pPropSetStg)
{
	printf("\n==================================================\n");
	printf("Custom Properties...\n");
	printf("==================================================\n");

	IPropertyStorage *pPropStg = NULL;
	HRESULT         hr;
	IEnumSTATPROPSTG *pEnumProp;

	/* Open User - Defined - Properties, getting an IpropertyStorage. */
	hr = pPropSetStg->Open(FMTID_UserDefinedProperties,
			       STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
	if (FAILED(hr)) {
		printf("No User Defined Properties.\n");
		return;
	}
	/* Get property enumerator. */
	hr = pPropStg->Enum(&pEnumProp);
	if (FAILED(hr)) {
		pPropStg->Release();
		printf("Couldn't enumerate custom properties.\n");
		return;
	}
	/* Enumerate properties. */
	STATPROPSTG     sps;
	ULONG           fetched;
	PROPSPEC        propSpec[1];
	PROPVARIANT     propVar[1];
	while (pEnumProp->Next(1, &sps, &fetched) == S_OK) {
		/* Build a PROPSPEC for this property.*/
			ZeroMemory(&propSpec[0], sizeof(PROPSPEC));
		propSpec[0].ulKind = PRSPEC_PROPID;
		propSpec[0].propid = sps.propid;

		/* Read this property. */
		hr = pPropStg->ReadMultiple(1, &propSpec[0], &propVar[0]);
		if (!FAILED(hr)) {
			/* Translate Prop name into ASCII. */
			char            dbcs[1024];
			char           *pbstr = (char *) sps.lpwstrName;
			int             i = wcstombs(dbcs, sps.lpwstrName,
						  *((DWORD *) (pbstr - 4)));
			dbcs[i] = 0;

			/* Dump this property. */
			printf("%16s: ", dbcs);
			DumpPropVariant(&propVar[0]);
		}
	}

	/* Release obtained interface. */
	pEnumProp->Release();
	pPropStg->Release();

}

/* Dump 's custom and built-in properties of a compound document. */
void
DumpProps(char *filename)
{
	/* Translate filename to Unicode. */
	WCHAR           wcFilename[1024];
	int             i = mbstowcs(wcFilename, filename, strlen(filename));
	wcFilename[i] = 0;

	IStorage       *pStorage = NULL;
	IPropertySetStorage *pPropSetStg = NULL;
	HRESULT         hr;

	/* Open the document as an OLE compound document. */
	hr =::StgOpenStorage(wcFilename, NULL,
		       STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);

	if (FAILED(hr)) {
		if (hr == STG_E_FILENOTFOUND)
			printf("File not found.");
		else if (hr == STG_E_FILEALREADYEXISTS)
			printf("Not a compound file.");
		else
			printf("StgOpenStorage() failed w/error %08lx", hr);
		return;
	}
	/* Obtain the IPropertySetStorage interface. */
	hr = pStorage->QueryInterface(
			   IID_IPropertySetStorage, (void **) &pPropSetStg);
	if (FAILED(hr)) {
		printf("QI for IPropertySetStorage failed w/error %08lx", hr);
		pStorage->Release();
		return;
	}
	/* Dump properties. */
	DumpBuiltInProps(pPropSetStg);
	DumpCustomProps(pPropSetStg);

	/* Release obtained interfaces. */
	pPropSetStg->Release();
	pStorage->Release();
}

/* Program entry - point. */
void
main(int argc, char **argv)
{
	/* Validate arguments. */
	if (argc != 2) {
		printf("- OLE Document Property Viewer\n");
		printf("- Usage: %s filename", argv[0]);
		return;
	}
	/* Pass filename to the subroutine. */
	DumpProps(argv[1]);
}
