/*
 * Copyright 2013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnssync, http://www.netsurf-browser.org/
 *
 * Released under the Expat MIT License (see COPYING),
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "util.h"

int nssync__saprintf(char **str_out, const char *format, ...)
{
	va_list ap;
	int slen = 0;
	char *str = NULL;

	va_start(ap, format);
	slen = vsnprintf(str, slen, format, ap);
	va_end(ap);

	str = malloc(slen + 1);
	if (str == NULL) {
		return -1;
	}

	va_start(ap, format);
	slen = vsnprintf(str, slen + 1, format, ap);
	va_end(ap);

	if (slen < 0) {
		free(str);
	} else {
		*str_out = str;
	}

	return slen;
}
