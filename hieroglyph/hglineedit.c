/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglineedit.c
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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

#include <stdlib.h>
#include <string.h>
#ifdef USE_LIBEDIT
#include <editline/readline.h>
#endif
#include "hglineedit.h"


/*
 * Private Functions
 */

/*
 * Public Functions
 */
gchar *
hg_line_edit_get_line(HgFileObject *stdin,
		      const gchar  *prompt,
		      gboolean      history)
{
	g_return_val_if_fail (stdin != NULL, NULL);

#ifdef USE_LIBEDIT
	G_STMT_START {
		gchar *retval;

		if (prompt == NULL)
			retval = readline("");
		else
			retval = readline(prompt);
		if (retval == NULL)
			return NULL;
		if (history)
			add_history(retval);

		return retval;
	} G_STMT_END;
#else
#error FIXME: implement me!
#endif /* USE_LIBEDIT */
}

gchar *
hg_line_edit_get_statement(HgFileObject *stdin, const gchar *prompt)
{
	gchar *line, *retval, *p;
	gint array_nest = 0, string_nest = 0;
	size_t len, i, total_len = 0;

	g_return_val_if_fail (stdin != NULL, NULL);

	retval = g_strdup("");
	while (1) {
		line = hg_line_edit_get_line(stdin, prompt, FALSE);
		if (line == NULL)
			return retval;
		len = strlen(line);
		for (i = 0; i < len; i++) {
			if (line[i] == '(')
				string_nest++;
			else if (line[i] == ')')
				string_nest--;
			if (string_nest < 0)
				string_nest = 0;
			if (string_nest > 0)
				continue;
			if (line[i] == '{')
				array_nest++;
			else if (line[i] == '}')
				array_nest--;
			if (array_nest < 0)
				array_nest = 0;
		}
		if (total_len > 0) {
			p = g_strconcat(retval, "\n", line, NULL);
		} else {
			p = g_strconcat(retval, line, NULL);
		}
		total_len += len;
		g_free(retval);
		free(line);
		retval = p;
		if (string_nest == 0 && array_nest == 0)
			break;
	}
#ifdef USE_LIBEDIT
	if (total_len > 0)
		add_history(retval);
#else
#error FIXME: implement me!
#endif /* USE_LINEEDIT */

	return retval;
}

gboolean
hg_line_edit_load_history(const gchar *filename)
{
	g_return_val_if_fail (filename != NULL, FALSE);

#ifdef USE_LIBEDIT
	read_history(filename);
#else
#error FIXME: implement me!
#endif

	return TRUE;
}

gboolean
hg_line_edit_save_history(const gchar *filename)
{
	g_return_val_if_fail (filename != NULL, FALSE);

#ifdef USE_LIBEDIT
	write_history(filename);
#else
#error FIXME: implement me!
#endif

	return TRUE;
}
