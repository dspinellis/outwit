/*
 *
 * odbc - select data from relational databases
 *
 * (C) Copyright 1999-2003 Diomidis Spinellis
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
 * $Id: odbc.c,v 1.3 2003-12-02 08:17:36 dds Exp $
 *
 */

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

struct s_coldata {
	SQLCHAR column_name[1024];
	SQLSMALLINT column_name_len;
	SQLSMALLINT data_type;
	SQLUINTEGER column_size;
	SQLSMALLINT decimal_digits;
	SQLSMALLINT nullable;
	unsigned char *data;
	SQLINTEGER io_len;
};

extern char	*optarg;	/* Global argument pointer. */
extern int	optind;		/* Global argv index. */

void
usage(char *fname)
{
	fprintf(stderr, 
		"odbc - select data from relational databases.  $Revision: 1.3 $\n"
		"(C) Copyright 1999-2003 Diomidis D. Spinelllis.  All rights reserved.\n\n"

		"Permission to use, copy, and distribute this software and its\n"
		"documentation for any purpose and without fee is hereby granted,\n"
		"provided that the above copyright notice appear in all copies and that\n"
		"both that copyright notice and this permission notice appear in\n"
		"supporting documentation.\n\n"

		"THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED\n"
		"WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"usage: %s [-R RS] [-F FS] [-u UID] [-a AS] [-h] DSN stmt\n"
		"\tRS\tRecord separator (default is newline)\n"
		"\tFS\tField separator (default is tab)\n"
		"\tUID\tUser identifier (default is empty)\n"
		"\tAS\tAuthorization string (default is empty)\n"
		"\tDSN\tODBC Data Source Name\n"
		"\t-h\tPrint headings on first line\n"
		"\tstmt\tA valid SELECT statement\n\n", fname
	);
	exit(1);
}

/* Report an SQL error, through the status records */
void
report_error(SQLSMALLINT handletype, SQLHANDLE handle)
{
	SQLSMALLINT	i = 1;
	SQLCHAR         SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER      NativeError;
	SQLSMALLINT     MsgLen;
	RETCODE         retcode;

	for (i = 1; (retcode = SQLGetDiagRec(handletype, handle, i, SqlState, 
	    &NativeError, Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA; i++)
		fprintf(stderr, "SQL Error State: %s, Native Error Code: %lX, ODBC Error: %s", 
			(LPSTR)SqlState, NativeError, (LPSTR)Msg); 
}

int 
main(int argc, char *argv[])
{
	RETCODE         retcode;
	struct s_coldata *coldata;		// Data for every column;
	int		headings = 0;
	SQLSMALLINT	numcol, i;
	char		*field_sep = "\t";
	char		*rec_sep = "\n";
	UCHAR		*uid = "", *auth = "";
	SQLHENV         henv = SQL_NULL_HENV;
	SQLHDBC         hdbc1 = SQL_NULL_HDBC;
	SQLHSTMT        hstmt1 = SQL_NULL_HSTMT;
	SQLHDESC        hdesc = NULL;
	char		c;

	while ((c = getopt(argc, argv, "R:F:u:a:h")) != EOF)
		switch (c) {
		case 'F':
			if (!optarg)
				usage(argv[0]);
			field_sep = strdup(optarg);
			break;
		case 'R':
			if (!optarg)
				usage(argv[0]);
			rec_sep = strdup(optarg);
			break;
		case 'u':
			if (!optarg)
				usage(argv[0]);
			uid = strdup(optarg);
			break;
		case 'a':
			if (!optarg)
				usage(argv[0]);
			auth = strdup(optarg);
			break;
		case 'h':
			headings = 1;
			break;
		case '?':
			usage(argv[0]);
		}

	if (argv[optind] == NULL || argv[optind + 1] == NULL)
		usage(argv[0]);
	/*
	 * Allocate the Environment handle.  Set the Env attribute, 
	 * allocate the connection handle, connect to the database 
	 * and allocate the statement handle.
	 */
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &henv);
	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER);
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc1);
	retcode = SQLDriverConnect(hdbc1, NULL, argv[optind], SQL_NTS, 0, 0, 0, SQL_DRIVER_NOPROMPT);

	if ((retcode == SQL_SUCCESS_WITH_INFO) || (retcode == SQL_ERROR))
		report_error(SQL_HANDLE_DBC, hdbc1);

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt1);

	retcode = SQLExecDirect(hstmt1, (UCHAR *) argv[optind + 1], SQL_NTS);

	if ((retcode == SQL_SUCCESS_WITH_INFO) || (retcode == SQL_ERROR))
		report_error(SQL_HANDLE_STMT, hstmt1);
	
	if ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO)) {
		retcode = SQLNumResultCols(hstmt1, &numcol);
	
		coldata = (struct s_coldata *)malloc((numcol + 1) * sizeof(struct s_coldata));
		// Column 0 is the bookmark column
		for (i = 1; i <= numcol; i++) {
			retcode = SQLDescribeCol(hstmt1, i, 
				coldata[i].column_name, 
				sizeof(coldata[i].column_name),
				&(coldata[i].column_name_len), 
				&(coldata[i].data_type), 
				&(coldata[i].column_size), 
				&(coldata[i].decimal_digits), 
				&(coldata[i].nullable));
			coldata[i].data = (char *)malloc(coldata[i].column_size + 1);
			retcode = SQLBindCol(hstmt1, i, 
				SQL_C_CHAR, 
				coldata[i].data,
				coldata[i].column_size + 1,
				&(coldata[i].io_len));
			if (headings) {
				fputs(coldata[i].column_name, stdout);
				if (i < numcol)
					fputs(field_sep, stdout);
			}
		}
		if (headings)
			fputs(rec_sep, stdout);
	
		for (;;) {
			retcode = SQLFetch(hstmt1);
			if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
				report_error(SQL_HANDLE_STMT, hstmt1);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				for (i = 1; i <= numcol; i++) {
					if (coldata[i].io_len != SQL_NULL_DATA)
						fputs(coldata[i].data, stdout);
					if (i < numcol)
						fputs(field_sep, stdout);
				}
				fputs(rec_sep, stdout);
			} else
				break;
	
			fflush(stdout);
		}
	}
	
	/* clean up */
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt1);
	SQLDisconnect(hdbc1);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc1);
	SQLFreeHandle(SQL_HANDLE_ENV, henv);
	return (0);
}
