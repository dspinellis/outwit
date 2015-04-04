/*
 *
 * choosecolor -- print the color selected from the Windows color selection dialog box
 *
 * (C) Copyright 1999-2006 Diomidis Spinellis
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

main(int argc, char *argv[])
{
	static CHOOSECOLOR c;
	static COLORREF custom[16];
	BOOL ret;

	c.lStructSize = sizeof(c);
	c.hwndOwner = NULL;
	c.lpCustColors = NULL;
	c.Flags = CC_ANYCOLOR | CC_FULLOPEN;
	if (argc == 4) {
		c.rgbResult =  RGB(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
		c.Flags |= CC_RGBINIT;
	}
	c.lpCustColors = custom;
	if (ChooseColor(&c) == 0)
		return (1);	/* Error, or cancel */
	printf("%d %d %d\n",
		GetRValue(c.rgbResult),
		GetGValue(c.rgbResult),
		GetBValue(c.rgbResult));
	return (0);
}
