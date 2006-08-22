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
#include <errno.h>
#include "hglineedit.h"
#include "hgfile.h"
#include "hgmem.h"


static void   _hg_line_edit_real_set_flags       (gpointer           data,
						  guint              flags);
static void   _hg_line_edit_real_relocate        (gpointer           data,
						  HgMemRelocateInfo *info);
static gchar *_hg_line_edit__default_get_line    (HgLineEdit        *lineedit,
						  const gchar       *prompt);
static void   _hg_line_edit__default_add_history (HgLineEdit        *lineedit,
						  const gchar       *strings);
static void   _hg_line_edit__default_load_history(HgLineEdit        *lineedit,
						  const gchar       *filename);
static void   _hg_line_edit__default_save_history(HgLineEdit        *lineedit,
						  const gchar       *filename);


struct _HieroGlyphLineEdit {
	HgObject                object;
	const HgLineEditVTable *vtable;
	HgFileObject           *stdin;
	HgFileObject           *stdout;
};

static HgObjectVTable __hg_line_edit_vtable = {
	.free      = NULL,
	.set_flags = _hg_line_edit_real_set_flags,
	.relocate  = _hg_line_edit_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};
static const HgLineEditVTable __hg_line_edit_default_vtable = {
	.get_line     = _hg_line_edit__default_get_line,
	.add_history  = _hg_line_edit__default_add_history,
	.load_history = _hg_line_edit__default_load_history,
	.save_history = _hg_line_edit__default_save_history,
};

/*
 * Private Functions
 */
static void
_hg_line_edit_real_set_flags(gpointer data,
			     guint    flags)
{
	HgLineEdit *lineedit = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(lineedit->stdin, obj);
	if (obj == NULL) {
		g_warning("[BUG] Invalid object %p to be marked: HgLineEdit", lineedit->stdin);
	} else {
		hg_mem_add_flags__inline(obj, flags, TRUE);
	}
	hg_mem_get_object__inline(lineedit->stdout, obj);
	if (obj == NULL) {
		g_warning("[BUG] Invalid object %p to be marked: HgLineEdit", lineedit->stdout);
	} else {
		hg_mem_add_flags__inline(obj, flags, TRUE);
	}
}

static void
_hg_line_edit_real_relocate(gpointer           data,
			    HgMemRelocateInfo *info)
{
	HgLineEdit *lineedit = data;

	if ((gsize)lineedit->stdin >= info->start &&
	    (gsize)lineedit->stdin <= info->end) {
		lineedit->stdin = (HgFileObject *)((gsize)lineedit->stdin + info->diff);
	}
	if ((gsize)lineedit->stdout >= info->start &&
	    (gsize)lineedit->stdout <= info->end) {
		lineedit->stdout = (HgFileObject *)((gsize)lineedit->stdout + info->diff);
	}
}

static gchar *
_hg_line_edit__default_get_line(HgLineEdit  *lineedit,
				const gchar *prompt)
{
	gchar c;
	gboolean cr = FALSE;
	GString *buf = g_string_new(NULL);
	gint error;

	if (prompt) {
		hg_file_object_write(lineedit->stdout, prompt, sizeof (gchar), strlen(prompt));
		hg_file_object_flush(lineedit->stdout);
	}
	while (1) {
		c = hg_file_object_getc(lineedit->stdin);
		error = hg_file_object_get_error(lineedit->stdin);
		if (error == EAGAIN) {
			sleep(1);
			continue;
		} else if (error != 0) {
			g_print("FIXME: errno %d: %s\n", errno, strerror(errno));
			break;
		}
		if (hg_file_object_is_eof(lineedit->stdin)) {
			g_string_free(buf, TRUE);
			return NULL;
		}
		if (c == '\r') {
			cr = TRUE;
			continue;
		} else if (c == '\n') {
			break;
		} else {
			if (cr) {
				hg_file_object_ungetc(lineedit->stdin, c);
				break;
			}
			g_string_append_c(buf, c);
		}
	}

	return g_string_free(buf, FALSE);
}

static void
_hg_line_edit__default_add_history(HgLineEdit  *lineedit,
				   const gchar *strings)
{
}

static void
_hg_line_edit__default_load_history(HgLineEdit  *lineedit,
				    const gchar *filename)
{
}

static void
_hg_line_edit__default_save_history(HgLineEdit  *lineedit,
				    const gchar *filename)
{
}

/*
 * Public Functions
 */
HgLineEdit *
hg_line_edit_new(HgMemPool              *pool,
		 const HgLineEditVTable *vtable,
		 HgFileObject           *stdin,
		 HgFileObject           *stdout)
{
	HgLineEdit *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (stdin != NULL, NULL);
	g_return_val_if_fail (stdout != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgLineEdit), HG_FL_HGOBJECT);
	if (retval == NULL)
		return NULL;
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_line_edit_vtable);

	if (vtable == NULL) {
		retval->vtable = &__hg_line_edit_default_vtable;
	} else {
		retval->vtable = vtable;
	}
	retval->stdin = stdin;
	retval->stdout = stdout;

	return retval;
}

gchar *
hg_line_edit_get_line(HgLineEdit  *lineedit,
		      const gchar *prompt,
		      gboolean     history)
{
	gchar *retval;
	const HgLineEditVTable *vtable;

	g_return_val_if_fail (lineedit != NULL, NULL);
	g_return_val_if_fail (lineedit->vtable != NULL, NULL);
	g_return_val_if_fail (lineedit->vtable->get_line != NULL, NULL);
	g_return_val_if_fail (lineedit->vtable->add_history != NULL, NULL);

	if (HG_FILE_GET_FILE_TYPE (lineedit->stdin) != HG_FILE_TYPE_STDIN) {
		vtable = &__hg_line_edit_default_vtable;
	} else {
		vtable = lineedit->vtable;
	}
	retval = vtable->get_line(lineedit, prompt);
	if (retval != NULL && history)
		vtable->add_history(lineedit, retval);

	return retval;
}

gchar *
hg_line_edit_get_statement(HgLineEdit *lineedit,
			   const gchar *prompt)
{
	gchar *line, *retval, *p;
	gint array_nest = 0, string_nest = 0;
	size_t len, i;
	const HgLineEditVTable *vtable;

	g_return_val_if_fail (lineedit != NULL, NULL);
	g_return_val_if_fail (lineedit->vtable != NULL, NULL);
	g_return_val_if_fail (lineedit->vtable->add_history != NULL, NULL);

	retval = g_strdup("");
	line = hg_line_edit_get_line(lineedit, prompt, FALSE);
	if (line == NULL)
		return retval;
	while (1) {
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
		p = g_strconcat(retval, line, "\n", NULL);
		g_free(retval);
		free(line);
		retval = p;
		if (string_nest == 0 && array_nest == 0)
			break;
		line = hg_line_edit_get_line(lineedit, "", FALSE);
		if (line == NULL)
			return retval;
	}
	p = g_strdup(retval);
	g_strchomp(p);
	len = strlen(p);
	if (len > 0) {
		if (HG_FILE_GET_FILE_TYPE (lineedit->stdin) != HG_FILE_TYPE_STDIN) {
			vtable = &__hg_line_edit_default_vtable;
		} else {
			vtable = lineedit->vtable;
		}
		vtable->add_history(lineedit, p);
	}
	g_free(p);

	return retval;
}

gboolean
hg_line_edit_load_history(HgLineEdit  *lineedit,
			  const gchar *filename)
{
	g_return_val_if_fail (lineedit != NULL, FALSE);
	g_return_val_if_fail (lineedit->vtable != NULL, FALSE);
	g_return_val_if_fail (lineedit->vtable->load_history != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	lineedit->vtable->load_history(lineedit, filename);

	return TRUE;
}

gboolean
hg_line_edit_save_history(HgLineEdit  *lineedit,
			  const gchar *filename)
{
	g_return_val_if_fail (lineedit != NULL, FALSE);
	g_return_val_if_fail (lineedit->vtable != NULL, FALSE);
	g_return_val_if_fail (lineedit->vtable->save_history != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	lineedit->vtable->save_history(lineedit, filename);

	return TRUE;
}

void
hg_line_edit_set_stdin(HgLineEdit   *lineedit,
		       HgFileObject *stdin)
{
	g_return_if_fail (lineedit != NULL);

	lineedit->stdin = stdin;
}

void
hg_line_edit_set_stdout(HgLineEdit   *lineedit,
			HgFileObject *stdout)
{
	g_return_if_fail (lineedit != NULL);

	lineedit->stdout = stdout;
}
