/*
 *
 * readlog - Windows log text-based access
 *
 * (C) Copyright 2002 Diomidis Spinellis
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
 * $Id: readlog.c,v 1.1 2002-07-09 18:57:20 dds Exp $
 *
 */

#include <time.h>
#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

/*
 * Program options
 */
static char *time_fmt = "%b %d %H:%M:%S";
static char *server = NULL;
static char *source = "System";
static int o_direction = EVENTLOG_FORWARDS_READ;
static int o_user = 1;
static int o_computer = 1;
static int o_source = 1;
static int o_type = 1;
static int o_ascii = 0;
static int o_hex = 0;
static int o_newline = 0;

static void
usage(char *fname)
{
	fprintf(stderr, 
		"readlog - Windows event log text-based access.  $Revision: 1.1 $\n"
		"(C) Copyright 2002 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: %s [-t fmt] [-v server] [-rucsyahn] [source]\n"
		"\t-t fmt\tTime format string (strftime)\n"
		"\t-v server\tServer name\n"
		"\t-r\tPrint entries in reverse order\n"
		"\t-u\tDo not print user information\n"
		"\t-c\tDo not print computer name\n"
		"\t-s\tDo not print event source\n"
		"\t-y\tDo not print event type\n"
		"\t-a\tOutput event-specific data as ASCII\n"
		"\t-h\tOutput event-specific data in hex format\n"
		"\t-n\tFormat event using newline separators\n"
		"\t\tDefault source is System,\n"
		"\t\tsource can be System, Application, Security or a custom name\n"
		, fname
	);
	exit(1);
}

extern char	*optarg;	/* Global argument pointer. */
extern int	optind;		/* Global argv index. */
int getopt(int argc, char *argv[], char *optstring);
static void print_log(void);

main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "t:v:rucsyahn")) != EOF)
		switch (c) {
		case 't':
			if (!optarg || optarg[1])
				usage(argv[0]);
			time_fmt = optarg;
			break;
		case 'v':
			if (!optarg || optarg[1])
				usage(argv[0]);
			server = optarg;
			break;
		case 'r': o_direction = EVENTLOG_BACKWARDS_READ; break;
		case 'u': o_user = 0; break;
		case 'c': o_computer = 0; break;
		case 's': o_source = 0; break;
		case 'y': o_type = 0; break;
		case 'a': o_ascii = 1; break;
		case 'h': o_hex = 1; break;
		case 'n': o_newline = 1; break;
		case '?':
			usage(argv[0]);
		}


	if (argv[optind] != NULL) {
		if (argv[optind + 1] != NULL)
			usage(argv[0]);
		/* Dump a key */
		source = argv[optind];
	}
	print_log();
	return (0);
}



static char *
evtype(int type)
{
	switch (type) {
	case EVENTLOG_ERROR_TYPE: return "error";
	case EVENTLOG_WARNING_TYPE: return "warning";
	case EVENTLOG_INFORMATION_TYPE: return "information";
	case EVENTLOG_AUDIT_SUCCESS: return "success";
	case EVENTLOG_AUDIT_FAILURE: return "failure";
	}
}

#define BUFFER_SIZE 8192

static void
wperror(char *s, LONG err)
{
	LPVOID lpMsgBuf;

	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    err,		// GetLastError() does not seem to work reliably
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);
	fprintf(stderr, "%s: %s\n", s, lpMsgBuf);
	LocalFree(lpMsgBuf);
}

static void
print_time(EVENTLOGRECORD *pelr)
{
	char buff[512];

	if (!*time_fmt)
		return;
	strftime(buff, sizeof(buff), time_fmt, localtime((time_t *)(&(pelr->TimeGenerated))));
	printf("%s ", buff);
}

void
print_uname(EVENTLOGRECORD *pelr)
{
	PSID            lpSid;
	char            szName[256];
	char            szDomain[256];
	SID_NAME_USE    snu;
	DWORD           dwLen;
	DWORD           cbName = 256;
	DWORD           cbDomain = 256;

	if (!o_user || pelr->UserSidLength == 0)
		return;
	// Point to the SID. 
	lpSid = (PSID) ((LPBYTE) pelr + pelr->UserSidOffset);
	if (LookupAccountSid(NULL, lpSid, szName, &cbName, szDomain, &cbDomain, &snu))
		printf("%s\\%s: ", szDomain, szName);
	else
		wperror("LookupAccountSid", GetLastError());
}

static void
print_msg(EVENTLOGRECORD *pelr)
{
	char key[1024];
	char tmpstr[1024];
	char msgfile[1024];
	DWORD type;
	DWORD datalen;
	int ret;
	HANDLE rh;
	HINSTANCE mh;
	char *outmsg;
	char *argv[1024];
	int i;
	char *p;

	print_time(pelr);
	/* Computer name */
	printf("%s ", (char *)pelr + strlen((char *)pelr + sizeof(EVENTLOGRECORD)) + sizeof(EVENTLOGRECORD) + 1);
	/* Access the registry message files */
	sprintf(key, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\%s\\%s", source, (LPSTR)((LPBYTE) pelr + sizeof(EVENTLOGRECORD)));
	if ((ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &rh)) != ERROR_SUCCESS) {
		wperror(key, ret);
		return;
	}
	datalen = sizeof(tmpstr);
	if ((ret = RegQueryValueEx(rh, "EventMessageFile", NULL, &type, tmpstr, &datalen)) != ERROR_SUCCESS) {
		wperror("EventMessageFile", ret);
		RegCloseKey(rh);
		return;
	}
	RegCloseKey(rh);
	if (ExpandEnvironmentStrings(tmpstr, msgfile, sizeof(msgfile)) == 0)
		wperror("ExpandEnvironmentStrings", ret);
	/* Event source */
	if (o_source)
		printf("%s: ", (LPSTR) ((LPBYTE) pelr + sizeof(EVENTLOGRECORD)));
	/* Event type */
	if (o_type)
		printf("%s: ", evtype(pelr->EventType));
	/* Category is seldom used: printf("CAT: %d: ", pelr->EventCategory); */
	print_uname(pelr);
	/*
	 * Some message file specs use a ; separated path.
	 * This is undocumented and we do not support it.
	 */
	if ((mh = LoadLibraryEx(msgfile, NULL, LOAD_LIBRARY_AS_DATAFILE)) == NULL) {
		wperror(msgfile, GetLastError());
		return;
	}
	for (i = 0, p = (char *)((LPBYTE) pelr + pelr->StringOffset); i < pelr->NumStrings; p += strlen(argv[i]) + 1, i++)
		argv[i] = p;
	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_ARGUMENT_ARRAY,
		mh, pelr->EventID, 0, (LPTSTR)&outmsg, 1024, argv) == 0) {
		wperror("FormatMessage", GetLastError());
		return;
	}
	if (o_newline)
		printf("\n%s", outmsg);
	else
		for (p = outmsg; *p; p++)
			switch (*p) {
			case '\r': break;
			case '\n': putchar(' '); break;
			default: putchar(*p);
			}
	putchar('\n');
	LocalFree(outmsg);
	FreeLibrary(mh);	/* TODO: cache libraries */
}

static void
print_log(void)
{
	HANDLE          h;
	EVENTLOGRECORD *pevlr;
	BYTE            bBuffer[BUFFER_SIZE];
	DWORD           dwRead,
	                dwNeeded,
	                cRecords,
	                dwThisRecord = 0;

	h = OpenEventLog(server, source);
	if (h == NULL) {
		wperror("Unable to open event log", GetLastError());
		exit(1);
	}

	pevlr = (EVENTLOGRECORD *)&bBuffer;
	// Opening the event log positions the file pointer for this 
	// handle at the beginning of the log. Read the records 
	// sequentially until there are no more. 
	while (ReadEventLog(h,	// event log handle 
			    o_direction |	// reads forward 
			    EVENTLOG_SEQUENTIAL_READ,	// sequential read 
			    0,	// ignored for sequential reads 
			    pevlr,	// pointer to buffer 
			    BUFFER_SIZE,	// size of buffer 
			    &dwRead,	// number of bytes read 
			    &dwNeeded))	// bytes in next record 
	{
		while (dwRead > 0) {
			print_msg(pevlr);
			dwRead -= pevlr->Length;
			pevlr =
			    (EVENTLOGRECORD *) ((LPBYTE) pevlr +
						pevlr->Length);
		}
		pevlr = (EVENTLOGRECORD *)&bBuffer;
	}
	CloseEventLog(h);
}
