/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * utils.c
 * Copyright (C) 2007 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <execinfo.h>
#include <stdlib.h>
#include <glib/gstrfuncs.h>
#include <glib/gstring.h>
#include "utils.h"


static gboolean __hg_stacktrace_feature = TRUE;

/*
 * private functions
 */

/*
 * public functions
 */
gchar *
hg_get_stacktrace(void)
{
	void *traces[256];
	int size, i;
	char **strings;
	GString *retval = g_string_new(NULL);

	if (__hg_stacktrace_feature == FALSE)
		return g_strdup("");

	size = backtrace(traces, 256);
	strings = backtrace_symbols(traces, size);

	for (i = 0; i < size; i++) {
		g_string_append(retval, strings[i]);
		g_string_append_c(retval, '\n');
	}
	free(strings);

	return g_string_free(retval, FALSE);
}

void
hg_use_stacktrace(gboolean flag)
{
#ifdef DEBUG
	__hg_stacktrace_feature = flag;
#else
	g_warning("The stacktrace feature are entirely disabled at the build time.");
#endif
}

#ifdef DEBUG
gboolean
hg_is_stacktrace_enabled(void)
{
	return __hg_stacktrace_feature;
}
#endif
