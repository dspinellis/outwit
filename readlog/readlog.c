/*
 *
 * readlog - Windows event log text-based access
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
 * $Id: readlog.c,v 1.15 2006-09-04 16:43:28 dds Exp $
 *
 */

#include <time.h>
#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
 * Program options
 */
static char *time_fmt = "%b %d %H:%M:%S";
static char *server = NULL;
static int o_direction = EVENTLOG_FORWARDS_READ;
static int o_id = 0;
static int o_user = 1;
static int o_computer = 1;
static int o_source = 1;
static int o_type = 1;
static int o_category = 1;
static int o_ascii = 0;
static int o_byte = 0;
static int o_dword = 0;
static int o_newline = 0;
static int o_recno = 0;

static void
usage(char *fname)
{
	fprintf(stderr,
		"readlog - Windows event log text-based access.  $Revision: 1.15 $\n"
		"(C) Copyright 2002 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: %s [-t fmt] [-v srv] [-riuwsycabdn] [source ...]\n"
		"\t-t fmt\tTime format string (strftime)\n"
		"\t-v srv\tServer name\n"
		"\t-r\tPrint entries in reverse order\n"
		"\t-i\tOutput decimal event id\n"
		"\t-N\tOutput decimal record number\n"
		"\t-u\tDo not print user information\n"
		"\t-w\tDo not print workstation name\n"
		"\t-s\tDo not print event source\n"
		"\t-y\tDo not print event type\n"
		"\t-c\tDo not print event category\n"
		"\t-a\tOutput event-specific data as ASCII\n"
		"\t-b\tOutput event-specific data as hex bytes\n"
		"\t-d\tOutput event-specific data as hex doublewords\n"
		"\t-n\tFormat event using newline separators\n"
		"\tDefault source is System;\n"
		"\tsource can be System, Application, Security or a custom name.\n"
		, fname
	);
	exit(1);
}

extern char	*optarg;	/* Global argument pointer. */
extern int	optind;		/* Global argv index. */
int getopt(int argc, char *argv[], char *optstring);
static void print_log(char *source);

main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "t:v:riucsyabdnN")) != EOF)
		switch (c) {
		case 't':
			if (!optarg)
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
		case 'w': o_computer = 0; break;
		case 'c': o_category = 0; break;
		case 's': o_source = 0; break;
		case 'y': o_type = 0; break;
		case 'a': o_ascii = 1; break;
		case 'b': o_byte = 1; break;
		case 'd': o_dword = 1; break;
		case 'n': o_newline = 1; break;
		case 'i': o_id = 1; break;
		case 'N': o_recno = 1; break;
		case '?':
			usage(argv[0]);
		}

	if (argv[optind] != NULL)
		while (argv[optind])
			print_log(argv[optind++]);
	else
		print_log("System");
	return (0);
}


static char *
evtype(int type)
{
	switch (type) {
	case EVENTLOG_ERROR_TYPE: return "Error";
	case EVENTLOG_WARNING_TYPE: return "Warning";
	case EVENTLOG_INFORMATION_TYPE: return "Information";
	case EVENTLOG_AUDIT_SUCCESS: return "Success";
	case EVENTLOG_AUDIT_FAILURE: return "Failure";
	}
	return "UNKNOWN";
}

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
	fprintf(stderr, "%s: %s", s, lpMsgBuf);
	LocalFree(lpMsgBuf);
}

static void
print_time(EVENTLOGRECORD *pelr)
{
	char buff[512];
	struct tm *t;
	static time_t y2038 = (time_t)0x7fffffff;

	if (!*time_fmt)
		return;
	// Under VC 5 -ve time_ts have localtime return NULL
	t = localtime((time_t *)(&(pelr->TimeGenerated)));
	if (!t)
		t = localtime(&y2038);
	strftime(buff, sizeof(buff), time_fmt, t);
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

static char *
get_message(char *msgfile, DWORD id, char *argv[])
{
	HINSTANCE mh;
	char *outmsg, *p;

	for (;;) {
		p = strchr(msgfile, ';');
		if (p)
			*p = 0;
		if ((mh = LoadLibraryEx(msgfile, NULL, LOAD_LIBRARY_AS_DATAFILE)) == NULL)
			break;
		if (FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
		    FORMAT_MESSAGE_FROM_HMODULE |
		    FORMAT_MESSAGE_ARGUMENT_ARRAY,
		    mh, id, 0, (LPTSTR)&outmsg, 1024, argv)) {
			FreeLibrary(mh);
			return (outmsg);
		}
		if (p && GetLastError() == ERROR_MR_MID_NOT_FOUND)
			msgfile = p + 1;
		else {
			wperror("FormatMessage", GetLastError());
			break;
		}
	}
	wperror(msgfile, GetLastError());
	p = LocalAlloc(LPTR, 20);
	sprintf(p, "%u", id);
	return p;
}

/*
 * Get a registry entry for name from rh expanding the environment strings
 * into name.
 * Return ERROR_SUCCESS if ok, otherwise the error code.
 */
static int
get_regentry(HANDLE rh, char *name, char *result)
{
	DWORD type;
	DWORD datalen;
	char tmpstr[1024];
	int ret;

	datalen = sizeof(tmpstr);
	if ((ret = RegQueryValueEx(rh, name, NULL, &type, tmpstr, &datalen)) != ERROR_SUCCESS)
		return ret;
	if (ExpandEnvironmentStrings(tmpstr, result, sizeof(tmpstr)) == 0)
		return GetLastError();
	return ERROR_SUCCESS;
}

static void
print_oneline(char *outmsg)
{
	char *p;

	for (p = outmsg; *p; p++)
		switch (*p) {
		case '\r':
			break;
		case '\n':
			if (p[1])
				putchar(' ');
			break;
		default: putchar(*p);
		}
}

static void
print_data(unsigned char *data, unsigned len)
{
	int i;
	unsigned c;

	if (o_ascii) {
		putchar('[');
		for (i = 0; i < len; i++) {
			c = data[i];
			if (c == '\\')
				printf("\\\\");
			else if (isprint(c))
				putchar(c);
			else
				switch (c) {
				case '\a': printf("\\a"); break;
				case '\b': printf("\\b"); break;
				case '\f': printf("\\f"); break;
				case '\t': printf("\\t"); break;
				case '\r': printf("\\r"); break;
				case '\n': printf("\\n"); break;
				case '\v': printf("\\v"); break;
				case '\0': printf("\\0"); break;
				default: printf("\\x%02x", c); break;
				}
		}
		putchar(']');
		if (o_newline)
			putchar('\n');
	}
	if (o_byte) {
		putchar('[');
		for (i = 0; i < len; i++) {
			printf("%02x", data[i]);
			if (i < len - 1)
				putchar(' ');
		}
		putchar(']');
		if (o_newline)
			putchar('\n');
	}
	if (o_dword) {
		putchar('[');
		for (i = 0; i < len / 4; i++) {
			printf("%08x", ((DWORD *)data)[i]);
			if (i < len / 4 - 1)
				putchar(' ');
		}
		putchar(']');
		if (o_newline)
			putchar('\n');
	}
}

static void
print_msg(EVENTLOGRECORD *pelr, char *source)
{
	char key[1024];
	char msgfile[1024];
	char paramfile[1024];
	int ret;
	HKEY rh;
	char *outmsg, *msg;
	char *argv[1024];
	int i;
	char *p;
	int regok;

	print_time(pelr);
	/* Computer name */
	printf("%s ", (char *)pelr + strlen((char *)pelr + sizeof(EVENTLOGRECORD)) + sizeof(EVENTLOGRECORD) + 1);
	/* Access the registry message files */
	sprintf(key, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\%s\\%s", source, (LPSTR)((LPBYTE) pelr + sizeof(EVENTLOGRECORD)));
	*msgfile = 0;
	if ((ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &rh)) != ERROR_SUCCESS) {
		wperror(key, ret);
		regok = 0;
	} else
		regok = 1;
	if (regok && (ret = get_regentry(rh, "EventMessageFile", msgfile)) != ERROR_SUCCESS) {
		fprintf(stderr, "%s\\", key);
		wperror("EventMessageFile", ret);
		RegCloseKey(rh);
	}
	if (o_recno)
		printf("%d: ", pelr->RecordNumber);
	/* Event source */
	if (o_source)
		printf("%s: ", (LPSTR) ((LPBYTE) pelr + sizeof(EVENTLOGRECORD)));
	/* Category */
	if (o_category) {
		char *category;
		char catfile[1024];

		if (!regok || pelr->EventCategory == 0)
			printf("-: ");
		else if ((ret = get_regentry(rh, "CategoryMessageFile", catfile)) == ERROR_SUCCESS) {
			category = get_message(catfile, pelr->EventCategory, NULL);
			print_oneline(category);
			printf(": ");
			LocalFree(category);
		} else {
			#ifdef WARNINGS
			fprintf(stderr, "%s\\", key);
			wperror("CategoryMessageFile", ret);
			#endif
			printf("UNKWNOWN_%d: ", pelr->EventCategory);
		}
	}
	/* Event type */
	if (o_type)
		printf("%s: ", evtype(pelr->EventType));
	print_uname(pelr);
	if (o_id)
		printf("%d: ", pelr->EventID & 0xffff);
	if (!regok){
		printf("Code %d (no error message available)", pelr->EventID & 0xffff);
		print_data((LPBYTE)pelr + pelr->DataOffset, pelr->DataLength);
		putchar('\n');
		return;
	}
	for (i = 0, p = (char *)((LPBYTE) pelr + pelr->StringOffset); i < pelr->NumStrings; p += strlen(argv[i]) + 1, i++)
		argv[i] = p;
	outmsg = get_message(msgfile, pelr->EventID, argv);
	if (get_regentry(rh, "ParameterMessageFile", paramfile) == ERROR_SUCCESS) {
		char *src, *dst;

		msg = malloc(strlen(outmsg) + 1);
		for (src = outmsg, dst = msg; *src;)
			if (src[0] == '%' && src[1] == '%' && isdigit(src[2])) {
				int code;
				char *param;

				code = strtoul(src + 2, &src, 10);
				param = get_message(paramfile, code, NULL);
				*dst = 0;
				msg = realloc(msg, strlen(msg) + strlen(outmsg) + strlen(param) + 1);
				strcat(msg, param);
				dst = msg + strlen(msg);
			} else
				*dst++ = *src++;
		*dst = 0;
	} else
		msg = outmsg;
	RegCloseKey(rh);
	if (o_newline)
		printf("\n%s", msg);
	else
		print_oneline(msg);
	print_data((LPBYTE)pelr + pelr->DataOffset, pelr->DataLength);
	putchar('\n');
	LocalFree(outmsg);
	if (msg != outmsg)
		free(msg);
}

static void
print_log(char *source)
{
	HANDLE          h;
	EVENTLOGRECORD *pevlr;
	BYTE            *bBuffer;
	DWORD           dwRead,
	                dwNeeded,
	                cRecords,
			dwSize,
	                dwThisRecord = 0;

	h = OpenEventLog(server, source);
	if (h == NULL) {
		wperror("Unable to open event log", GetLastError());
		exit(1);
	}

	bBuffer = malloc(dwSize = 8192);
again:
	pevlr = (EVENTLOGRECORD *)bBuffer;
	while (ReadEventLog(h,	// event log handle
			    o_direction |	// reads forward
			    EVENTLOG_SEQUENTIAL_READ,	// sequential read
			    0,	// ignored for sequential reads
			    pevlr,	// pointer to buffer
			    dwSize,	// size of buffer
			    &dwRead,	// number of bytes read
			    &dwNeeded))	// bytes in next record
	{
		while (dwRead > 0) {
			print_msg(pevlr, source);
			dwRead -= pevlr->Length;
			pevlr =
			    (EVENTLOGRECORD *) ((LPBYTE) pevlr +
						pevlr->Length);
		}
		pevlr = (EVENTLOGRECORD *)bBuffer;
	}
	switch (GetLastError()) {
	case ERROR_INSUFFICIENT_BUFFER:
		bBuffer = realloc(bBuffer, dwSize = dwNeeded);
		if (bBuffer == NULL) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		goto again;
	case ERROR_HANDLE_EOF:
		break;
	default:
		wperror("ReadEventLog", GetLastError());
	}
	CloseEventLog(h);
}
