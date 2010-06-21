/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgerror.c
 * Copyright (C) 2007-2010 Akira TAGOH
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
#include "config.h"
#endif

#include <execinfo.h>
#include <stdlib.h>
#include "hgerror.h"


static gboolean __hg_stacktrace_feature = TRUE;

/**
 * hg_get_stacktrace:
 *
 * FIXME
 *
 * Returns: FIXME
 */
gchar *
hg_get_stacktrace(void)
{
	void *traces[256];
	GPtrArray *array;
	GString *retval;
	int size, i;
	char **strings;

	if (__hg_stacktrace_feature == FALSE)
		return g_strdup("");

	array = g_ptr_array_new();
	retval = g_string_new(NULL);
	do {
		size = backtrace(traces, 256);
		for (i = 0; i < size; i++)
			g_ptr_array_add(array, traces[i]);
	} while (size == 256);
	strings = backtrace_symbols(array->pdata, array->len);

	for (i = 1; i < array->len; i++) {
		g_string_append_printf(retval, "  %d. ", i);
		g_string_append(retval, strings[i]);
		g_string_append_c(retval, '\n');
	}
	free(strings);
	g_ptr_array_free(array, TRUE);

	return g_string_free(retval, FALSE);
}

/**
 * hg_use_stacktrace:
 * @flag: if %TRUE the stacktrace feature is enabled.
 *
 * FIXME
 */
void
hg_use_stacktrace(gboolean flag)
{
#ifdef HG_DEBUG
	__hg_stacktrace_feature = flag;
#else
	g_warning("The stacktrace feature are entirely disabled at the build time.");
#endif
}

/**
 * hg_is_stacktrace_enabled:
 *
 * FIXME
 *
 * Returns: %TRUE if the stacktrace feature is enabled. otherwise %FALSE.
 */
gboolean
hg_is_stacktrace_enabled(void)
{
#ifdef HG_DEBUG
	return __hg_stacktrace_feature;
#else
	return FALSE;
#endif
}
