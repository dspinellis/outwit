/*
 *
 * winreg - Windows registry text-based access
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
 * $Id: winreg.c,v 1.4 2000-04-01 12:14:39 dds Exp $
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

/* The name and value of the root handle (HKEY_...) */
static char rootname[1024];
static HKEY rootkey;

/* Program options */
static int o_print_name = 1;
static int o_print_type = 1;
static int o_print_value = 1;
static int o_verify = 0;
static int o_print_decimal = 0;
static char field_sep = '\t';

extern char	*optarg;	/* Global argument pointer. */
extern int	optind;		/* Global argv index. */

static int line;		/* Current input line */

static void
wperror(char *s)
{
	LPVOID lpMsgBuf;

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

/*
 * Print a registry key value
 */
static void
print_value(int type, unsigned char *data, int len)
{
	int i;
	unsigned char c;

	if (o_print_type) {
		if (o_print_name)
			putchar(field_sep);
		switch (type) {
		case REG_BINARY: printf("BINARY"); break;
		case REG_DWORD: printf("DWORD"); break;
		case REG_SZ: printf("SZ"); break;
		case REG_MULTI_SZ: printf("MULTI_SZ"); break;
		case REG_EXPAND_SZ: printf("EXPAND_SZ"); break;
		case REG_NONE: printf("NONE"); break;
		case REG_LINK: printf("LINK"); break;
		case REG_RESOURCE_LIST: printf("RESOURCE_LIST"); break;
		default:
			fprintf(stderr, "Unknown registry type: 0x%x\n", type);
			break;
		}
	}
	if (o_print_value) {
		if (o_print_name || o_print_type)
			putchar(field_sep);
		switch (type) {
		case REG_BINARY:
			if (o_print_decimal) {
				switch (len) {
				case 1:
					printf("%u", *(unsigned char *)data);
					break;
				case 2:
					printf("%u", *(unsigned short *)data);
					break;
				case 4:
					printf("%u", *(unsigned int *)data);
					break;
				default:
					for (i = 0; i < len; i++)
						printf("%02x ", data[i]);
					break;
				}
				break;
			}
			/* FALLTHROUGH */
		case REG_LINK:
		case REG_RESOURCE_LIST:
			for (i = 0; i < len; i++)
				printf("%02x ", data[i]);
			break;
		case REG_DWORD:
			if (o_print_decimal)
				printf("%u", *(unsigned int *)data);
			else
				printf("%08x", *(unsigned int *)data);
			break;
		case REG_MULTI_SZ:
		case REG_EXPAND_SZ:
		case REG_SZ:
			for (i = 0; i < len - 1; i++) {
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
			break;
		case REG_NONE:
			break;
		default:
			fprintf(stderr, "Unknown registry type: 0x%x\n", type);
			break;
		}
	}
}

/*
 * Dump the values of a given key
 */
static int
print_key(HKEY key, char *basename)
{
	DWORD namelen, datalen;
	DWORD onamelen, odatalen;
	char *data, *name;
	DWORD type, i, numvalues;

	if (RegQueryInfoKey(key, NULL, NULL, NULL,
				NULL, NULL, NULL,
				&numvalues,
				&namelen, &datalen, NULL, NULL) != ERROR_SUCCESS) {
		wperror("RegQueryInfoKey");
		return (-1);
	}
	namelen++;
	datalen++;
	name = malloc(namelen);
	data = malloc(datalen);
	/*
	odatalen = datalen;
	if (RegQueryValueEx(key, NULL, NULL, &type, data, &odatalen) != ERROR_SUCCESS)
			wperror("RegQueryValueEx");
	
	*/
	if (numvalues == 0) {
		if (o_print_name)
			printf("%s\\%s\\", rootname, basename);
		print_value(REG_NONE, "", 0);
		putchar('\n');
	}
	for (i = 0; ; i++) {
		onamelen = namelen;
		odatalen = datalen;
		switch (RegEnumValue(key, i, name, &onamelen, NULL, &type, data, &odatalen)) {
		case ERROR_SUCCESS:
			if (o_print_name)
				printf("%s\\%s\\%s", rootname, basename, name);
			print_value(type, data, odatalen);
			putchar('\n');
			break;
		case ERROR_NO_MORE_ITEMS:
			free(name);
			free(data);
			return (0);
		default:
			wperror("RegEnumValue");
			free(name);
			free(data);
			return (-1);
		}
	}
	/* NOTREACHED */
}

/*
 * Recursively dump a registry node
 * Each key can have a number of subkeys and a number of values
 */
static int
print_registry(char *pkeyname)
{
	int i, err;
	HKEY key;
	char *keyname, *subkeypos;
	DWORD keynamelen;
	DWORD skeynamelen, oskeynamelen;

	if (RegOpenKeyEx(rootkey, pkeyname, 0, KEY_READ, &key) != ERROR_SUCCESS) {
		wperror("RegOpenKeyEx");
		return (-1);
	}

	print_key(key, pkeyname);
	if (RegQueryInfoKey(key, NULL, NULL, NULL,
			    NULL, &skeynamelen, NULL,
			    NULL,
			    NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
		wperror("RegQueryInfoKey");
		RegCloseKey(key);
		return (-1);
	}
	skeynamelen++;
	if (pkeyname)
		keynamelen = skeynamelen + strlen(pkeyname) + 1;	/* To hold the \ */
	else
		keynamelen = skeynamelen;
	subkeypos = keyname = malloc(keynamelen);
	if (pkeyname) {
		strcpy(keyname, pkeyname);
		strcat(keyname, "\\");
		subkeypos += strlen(pkeyname) + 1;
	}
	for (i = 0; ; i++) {
		oskeynamelen = skeynamelen;
		switch (err = RegEnumKeyEx(key, i, subkeypos, &oskeynamelen, NULL, NULL, NULL, NULL)) {
		case ERROR_SUCCESS:
			print_registry(keyname);
			break;
		case ERROR_NO_MORE_ITEMS:
			RegCloseKey(key);
			free(keyname);
			return (0);
		default:
			wperror("RegEnumKeyEx");
			RegCloseKey(key);
			free(keyname);
			return (-1);
		}
	}
	/* NOTREACHED */
}

/* A simple map */
static struct s_nameval {
	char *name;
	DWORD val;
};

struct s_nameval handles[] = {
	{"HKEY_CLASSES_ROOT", (DWORD)HKEY_CLASSES_ROOT},
	{"HKEY_CURRENT_CONFIG", (DWORD)HKEY_CURRENT_CONFIG},
	{"HKEY_CURRENT_USER", (DWORD)HKEY_CURRENT_USER},
	{"HKEY_LOCAL_MACHINE", (DWORD)HKEY_LOCAL_MACHINE},
	{"HKEY_USERS", (DWORD)HKEY_USERS},
	{"HKEY_PERFORMANCE_DATA", (DWORD)HKEY_PERFORMANCE_DATA},
	{"HKEY_DYN_DATA", (DWORD)HKEY_DYN_DATA},
};

struct s_nameval types[] = {
	{"BINARY", REG_BINARY},
	{"DWORD", REG_DWORD},
	{"SZ", REG_SZ},
	{"MULTI_SZ", REG_MULTI_SZ},
	{"EXPAND_SZ", REG_EXPAND_SZ},
	{"NONE", REG_NONE},
	{"LINK", REG_LINK},
	{"RESOURCE_LIST", REG_RESOURCE_LIST},
};

/*
 * Map a name from the map structure to its value.
 * Return true if found, false if not
 */
static int
nameval(struct s_nameval *map, int maplen, char *name, int *val)
{
	int i;

	for (i = 0; i < maplen; i++)
		if (strcmp(map[i].name, name) == 0) {
			*val = map[i].val;
			return (1);
		}
	return (0);
}

static void
usage(char *fname)
{
	fprintf(stderr, 
		"winreg - Windows registry text-based access.  $Revision: 1.4 $\n"
		"(C) Copyright 1999, 2000 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: %s [-F c] [-ntvc] [key]\n"
		"\tFS\tField separator (default is tab)\n"
		"\t-n\tDo not print key names\n"
		"\t-t\tDo not print key types\n"
		"\t-v\tDo not print key values\n"
		"\t-d\tOutput DWORD and binary data of (1, 2, 4) bytes in decimal\n"
		"\t\twarning: decimal output can not be parsed for input\n"
		"\t-c\tCheck input, do not enter keys into registry\n\n", fname
	);
	exit(1);
}

/*
 * Return the integer value of hex digit c
 */
static int
hexval(unsigned char c)
{
	static unsigned char hex[] = "0123456789abcdef";
	char *p;

	p = strchr(hex, tolower(c));
	if (p == NULL) {
		fprintf(stderr, "Line %d: Hexadecimal digit expected\n", line);
		exit(1);
	}
	return (unsigned char)(p - hex);
}

/*
 * Split a registry key name into its root name (setting rootkey)
 * returning a pointer to the rest.
 */
static char *
split_name(char *name, HKEY *rkey, char *rname)
{
	char *s, *p;

	for (s = name, p = rname; *s && *s != '\\'; s++, p++)
		*p = *s;
	*p = '\0';
	if (*s == '\\')
		s++;
	if (nameval(handles, sizeof(handles) / sizeof(struct s_nameval), rname, (DWORD *)rkey))
		return (*s ? s : NULL);
	else {
		fprintf(stderr, "Unknown root handle name [%s]\n", rname);
		exit(1);
	}
}

/*
 * Add a key and its data to the registry
 */
static void
addkey(char *name, DWORD type, char *value, int datalen)
{
	HKEY root;
	char *kname;		/* Key name */
	char *vname;		/* Value name */
	char rname[100];
	HKEY key;

	if (o_verify)
			return;
	kname = split_name(name, &root, rname);
	vname = strrchr(kname, '\\');
	if (vname) {
		*vname = '\0';
		vname++;
	}
	if (RegCreateKeyEx(root, kname, 0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS)
		wperror("RegCreateKeyEx");
	if (type != REG_NONE)
		if (RegSetValueEx(key, vname, 0, type, value, datalen) != ERROR_SUCCESS)
			wperror("RegSetValueEx");
	if (RegCloseKey(key) != ERROR_SUCCESS)
		wperror("RegCloseKey");
}

/* 
 * Swap around the bytes of data to match the Intel DWORD endianess
 */
static void
swapbytes(unsigned char *data)
{
		unsigned char tmp;

		tmp = data[0];
		data[0] = data[3];
		data[3] = tmp;
		tmp = data[2];
		data[2] = data[1];
		data[1] = tmp;
}

/*
 * Process formated input containing registry data adding keys to the
 * registry
 */
static void
input_process(void)
{
	/* Read and add keys */
	char *name;
	char type[100];
	unsigned char *data;
	int namelen, datalen;
	int nameidx, typeidx, dataidx;
	enum {SNAME = 1000, STYPE, REG_BINARY2, REG_SZ_BACK, REG_SZ_HEX, REG_SZ_HEX2} 
		state;
	int c;
	DWORD typeval;
	int hexcount;

	name = malloc(namelen = 32);
	data = malloc(datalen = 32);
	state = SNAME;
	nameidx = 0;
	line = 1;
	while ((c = getchar()) != EOF) {
		switch (state) {
		case SNAME:
			if (nameidx >= namelen)
				name = realloc(name, namelen *= 2);
			if (c == field_sep) {
				name[nameidx++] = '\0';
				state = STYPE;
				typeidx = 0;
			} else if (c == '\n') {
				fprintf(stderr, "Missing type in line %d\n", line);
				exit(1);
			} else
				name[nameidx++] = c;
			break;
		case STYPE:
			if (c == field_sep) {
				type[typeidx++] = '\0';
				dataidx = 0;
				if (!nameval(types, sizeof(types) / sizeof(struct s_nameval), type, &typeval)) {
					fprintf(stderr, "Line %d: Unrecognized type name\n", line);
					exit(1);
				}
				state = typeval;
			} else if (c == '\n') {
				fprintf(stderr, "Line %d: missing data\n", line);
				exit(1);
			} else
				type[typeidx++] = c;
			break;
		case REG_BINARY:
		case REG_DWORD:
		case REG_LINK:
		case REG_RESOURCE_LIST:
			if (c == '\n') {
				if (typeval == REG_DWORD) {
					if (dataidx != 4) {
						fprintf(stderr, "Line :%d: not a four byte DWORD\n", line);
						exit(1);
					}
					swapbytes(data);
				}
		addkey:
				addkey(name, typeval, data, dataidx);
				nameidx = 0;
				state = SNAME;
				line++;
				break;
			} else if (isspace(c))
				break;
			data[dataidx] = hexval((unsigned char)c) << 4;
			state = REG_BINARY2;
			break;
		case REG_BINARY2:
			data[dataidx++] |= hexval((unsigned char)c);
			state = REG_BINARY;
			break;
		case REG_SZ:
		case REG_MULTI_SZ:
		case REG_EXPAND_SZ:
			if (c == '\n') {
				data[dataidx] = '\0';
				goto addkey;
			} else if (c == '\\')
				state = REG_SZ_BACK;
			else
				data[dataidx++] = c;
			break;
		case REG_SZ_BACK:
			switch (c) {
			case '\\': data[dataidx++] = '\\'; break;
			case 'a': data[dataidx++] = '\a'; break;
			case 'b': data[dataidx++] = '\b'; break;
			case 'f': data[dataidx++] = '\f'; break;
			case 't': data[dataidx++] = '\t'; break;
			case 'r': data[dataidx++] = '\r'; break;
			case 'n': data[dataidx++] = '\n'; break;
			case 'v': data[dataidx++] = '\v'; break;
			case '0': data[dataidx++] = '\0'; break;
			case 'x': state = REG_SZ_HEX; hexcount = 0; break;
			}
			state = REG_SZ;
			break;
		case REG_SZ_HEX:
			data[dataidx] = hexval((unsigned char)c) << 4;
			state = REG_SZ_HEX2;
			break;
		case REG_SZ_HEX2:
			data[dataidx++] |= hexval((unsigned char)c);
			state = REG_SZ;
			break;
		case REG_NONE:
			if (c == '\n')
				goto addkey;
			else {
				fprintf(stderr, "Line %d: Unexpected data\n", line);
				exit(1);
			}
		default:
			assert(0);
		}
	}
}

int getopt(int argc, char *argv[], char *optstring);

main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "F:ntvcd")) != EOF)
		switch (c) {
		case 'F':
			if (!optarg || optarg[1])
				usage(argv[0]);
			field_sep = *optarg;
			break;
		case 'n': o_print_name = 0; break;
		case 't': o_print_type = 0; break;
		case 'v': o_print_value = 0; break;
		case 'c': o_verify = 1; break;
		case 'd': o_print_decimal = 1; break;
		case '?':
			usage(argv[0]);
		}


	if (argv[optind] != NULL) {
		if (argv[optind + 1] != NULL)
			usage(argv[0]);
		/* Dump a key */
		print_registry(split_name(argv[optind], &rootkey, rootname));
	} else
		/* Read input and add keys */
		input_process();
	return (0);
}
