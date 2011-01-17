/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglineedit.l
 * Copyright (C) 2006-2010 Akira TAGOH
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
%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hglineedit.h"

#include "hglineedit.proto.h"

#define YY_DECL			static gchar *_hg_lineedit_yylex(hg_lineedit_t *scanner, yyscan_t yyscanner)
#define yyterminate()		{BEGIN(INITIAL);g_string_free(string, TRUE);return NULL;}
#define YY_NO_INPUT
#define YY_INPUT(_buf_,_result_,_max_size_)			\
	g_assert_not_reached()
#define YY_EXTRA_TYPE		hg_lineedit_t *
#define YY_USER_INIT		yyextra = scanner;
#define ECHO			do { if (fwrite(yytext, yyleng, 1, yyout)){}; fflush(yyout); g_assert_not_reached();} while (0)


struct _hg_lineedit_t {
	hg_mem_t                   *mem;
	hg_quark_t                  qself;
	const hg_lineedit_vtable_t *vtable;
	hg_file_t                  *infile;
	hg_file_t                  *outfile;
	yyscan_t                    yyscanner;
	gpointer                    user_data;
};


#ifndef yyget_column
int yyget_column(yyscan_t yyscanner);
#endif
#ifndef yyset_column
void yyset_column(int column_no, yyscan_t yyscanner);
#endif


static hg_lineedit_vtable_t __hg_lineedit_default_vtable = {
	.get_line     = _hg_lineedit_real_get_line,
	.add_history  = _hg_lineedit_real_add_history,
	.load_history = _hg_lineedit_real_load_history,
	.save_history = _hg_lineedit_real_save_history
};
%}
%option nounput noyywrap warn nounistd reentrant
%option prefix="hglineedit"
%x STRING HEXSTRING
%s PROC

CONTROL		[\x04%\(\)/<>\[\]\{\}]
SPACE		[\t\f\r\n\0 ]
HEXDIGITS	[0-9a-fA-F]
%%
	gint string_count = 0, proc_count = 0;
	GString *string = g_string_new(NULL);

<STRING>{
	"("	{
		string_count++;
		g_string_append_c(string, yytext[0]);
	}
	")"	{
		string_count--;
		g_string_append_c(string, yytext[0]);
		if (string_count == 0) {
			BEGIN(INITIAL);
		}
	}
	\\.	{
		g_string_append_len(string, yytext, yyleng);
	}
	.|\r\n|\r|\n	{
		g_string_append_len(string, yytext, yyleng);
	}
	<<EOF>>		{
		yyterminate();
	}
}
\(	{
		string_count++;
		BEGIN(STRING);
		g_string_append_c(string, yytext[0]);
	}
<HEXSTRING>{
	">"	{
		BEGIN(INITIAL);
		g_string_append_c(string, yytext[0]);
	}
	[^>]	{
		g_string_append_len(string, yytext, yyleng);
	}
	<<EOF>>		{
		yyterminate();
	}
}
\<		{
		BEGIN(HEXSTRING);
		g_string_append_c(string, yytext[0]);
	}
"{"		{
		BEGIN(PROC);
		proc_count++;
		g_string_append_c(string, yytext[0]);
	}
<PROC>\r\n|\r|\n	{
		g_string_append_len(string, yytext, yyleng);
	}
"}"		{
		proc_count--;
		g_string_append_c(string, yytext[0]);
		if (proc_count == 0) {
			BEGIN(INITIAL);
		}
	}
<INITIAL>\r\n|\r|\n	{
		g_string_append_len(string, yytext, yyleng);

		return g_string_free(string, FALSE);
	}
[^\r\n\(\)\<\>]	{
		g_string_append_len(string, yytext, yyleng);
	}
"<<"|">>"	{
		g_string_append_len(string, yytext, yyleng);
	}
<<EOF>>		{
		yyterminate();
	}
%%
/*< private >*/
static gchar *
_hg_lineedit_real_get_line(hg_lineedit_t *lineedit,
			   const gchar   *prompt,
			   gpointer       user_data)
{
	GString *buf = g_string_new(NULL);
	gchar c;
	GError *err = NULL;

	hg_return_val_if_fail (lineedit->infile != NULL, NULL);
	hg_return_val_if_fail (lineedit->outfile != NULL, NULL);

	if (prompt) {
		hg_file_write(lineedit->outfile, (gchar *)prompt,
			      sizeof (gchar), strlen(prompt), &err);
		hg_file_flush(lineedit->outfile, &err);
	}
	while (1) {
		hg_file_read(lineedit->infile, &c,
			     sizeof (gchar), 1, &err);
		if (err) {
			if (err->code == EAGAIN) {
				sleep(1);
				g_clear_error(&err);
				continue;
			} else if (err->code == EBADF) {
				g_string_free(buf, TRUE);
				g_error_free(err);

				return NULL;
			}
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
			g_error_free(err);
			break;
		}
		if (hg_file_is_eof(lineedit->infile)) {
			g_string_free(buf, TRUE);

			return NULL;
		}
		if (c == '\n')
			break;
		g_string_append_c(buf, c);
	}

	return g_string_free(buf, FALSE);
}

static void
_hg_lineedit_real_add_history(hg_lineedit_t *lineedit,
			      const gchar   *history,
			      gpointer       user_data)
{
}

static gboolean
_hg_lineedit_real_load_history(hg_lineedit_t *lineedit,
			       const gchar   *historyfile,
			       gpointer       user_data)
{
	return TRUE;
}

static gboolean
_hg_lineedit_real_save_history(hg_lineedit_t *lineedit,
			       const gchar   *historyfile,
			       gpointer       user_data)
{
	return TRUE;
}

/*< public >*/
/**
 * hg_lineedit_get_default_vtable:
 *
 * FIXME
 *
 * Returns:
 */
hg_lineedit_vtable_t *
hg_lineedit_get_default_vtable(void)
{
	return &__hg_lineedit_default_vtable;
}

/**
 * hg_lineedit_new:
 * @mem:
 * @vtable:
 * @infile:
 * @outfile:
 *
 * FIXME
 *
 * Returns:
 */
hg_lineedit_t *
hg_lineedit_new(hg_mem_t                   *mem,
		const hg_lineedit_vtable_t *vtable,
		hg_file_t                  *infile,
		hg_file_t                  *outfile)
{
	hg_lineedit_t *retval;
	hg_quark_t q;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (vtable != NULL, NULL);

	q = hg_mem_alloc(mem, sizeof (hg_lineedit_t), (gpointer *)&retval);
	if (q == Qnil)
		return NULL;

	retval->qself = q;
	retval->mem = mem;
	retval->vtable = vtable;
	retval->infile = infile;
	retval->outfile = outfile;

	yylex_init(&retval->yyscanner);
	if (retval->yyscanner == NULL) {
		hg_lineedit_destroy(retval);
		return NULL;
	}

	return retval;
}

/**
 * hg_lineedit_destroy:
 * @lineedit:
 *
 * FIXME
 */
void
hg_lineedit_destroy(hg_lineedit_t *lineedit)
{
	if (lineedit == NULL)
		return;

	if (lineedit->infile)
		hg_mem_unlock_object(lineedit->infile->o.mem,
				     lineedit->infile->o.self);
	if (lineedit->outfile)
		hg_mem_unlock_object(lineedit->outfile->o.mem,
				     lineedit->outfile->o.self);
	if (lineedit->yyscanner)
		yylex_destroy(lineedit->yyscanner);

	hg_mem_free(lineedit->mem, lineedit->qself);
}

/**
 * hg_lineedit_get_line:
 * @lineedit:
 * @prompt:
 * @enable_history:
 *
 * FIXME
 *
 * Returns:
 */
gchar *
hg_lineedit_get_line(hg_lineedit_t *lineedit,
		     const gchar   *prompt,
		     gboolean       enable_history)
{
	gchar *retval;

	hg_return_val_if_fail (lineedit != NULL, NULL);
	hg_return_val_if_fail (lineedit->vtable != NULL, NULL);
	hg_return_val_if_fail (lineedit->vtable->get_line != NULL, NULL);

	retval = lineedit->vtable->get_line(lineedit,
					    prompt,
					    lineedit->user_data);
	if (retval != NULL && enable_history) {
		if (lineedit->vtable->add_history)
			lineedit->vtable->add_history(lineedit,
						      retval,
						      lineedit->user_data);
	}

	return retval;
}

/**
 * hg_lineedit_get_statement:
 * @lineedit:
 * @prompt:
 * @enable_history:
 *
 * FIXME
 *
 * Returns:
 */
gchar *
hg_lineedit_get_statement(hg_lineedit_t *lineedit,
			  const gchar   *prompt,
			  gboolean       enable_history)
{
	GString *string = g_string_new(NULL);
	gchar *ret, *retval;
	YY_BUFFER_STATE yystate;
	struct yyguts_t *yyg;

	while (1) {
		ret = hg_lineedit_get_line(lineedit, prompt, FALSE);
		if (ret == NULL)
			return NULL;
		g_string_append_printf(string, "%s\n", ret);
		g_free(ret);
		yyg = (struct yyguts_t *)lineedit->yyscanner;
		if (YY_CURRENT_BUFFER) {
			yy_delete_buffer(YY_CURRENT_BUFFER, lineedit->yyscanner);
		}
		yystate = yy_scan_bytes(string->str, string->len, lineedit->yyscanner);
		yy_switch_to_buffer(yystate, lineedit->yyscanner);
		retval = _hg_lineedit_yylex(lineedit, lineedit->yyscanner);
		if (retval == NULL) {
			/* obtaining more lines until no /syntaxerror raised */
			continue;
		} else {
			if (enable_history) {
				if (lineedit->vtable->add_history)
					lineedit->vtable->add_history(lineedit,
								      retval,
								      lineedit->user_data);
			}
			break;
		}
	}

	return retval;
}

/**
 * hg_lineedit_load_history:
 * @lineedit:
 * @historyfile:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_lineedit_load_history(hg_lineedit_t *lineedit,
			 const gchar   *historyfile)
{
	hg_return_val_if_fail (lineedit != NULL, FALSE);
	hg_return_val_if_fail (historyfile != NULL, FALSE);
	hg_return_val_if_fail (lineedit->vtable != NULL, FALSE);

	if (lineedit->vtable->load_history)
		return lineedit->vtable->load_history(lineedit,
						      historyfile,
						      lineedit->user_data);

	return TRUE;
}

/**
 * hg_lineedit_save_history:
 * @lineedit:
 * @historyfile:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_lineedit_save_history(hg_lineedit_t *lineedit,
			 const gchar   *historyfile)
{
	hg_return_val_if_fail (lineedit != NULL, FALSE);
	hg_return_val_if_fail (historyfile != NULL, FALSE);
	hg_return_val_if_fail (lineedit->vtable != NULL, FALSE);

	if (lineedit->vtable->save_history)
		return lineedit->vtable->save_history(lineedit,
						      historyfile,
						      lineedit->user_data);

	return TRUE;
}

/**
 * hg_lineedit_set_infile:
 * @lineedit:
 * @infile:
 *
 * FIXME
 */
void
hg_lineedit_set_infile(hg_lineedit_t *lineedit,
		       hg_file_t     *infile)
{
	hg_return_if_fail (lineedit != NULL);
	hg_return_if_fail (infile != NULL);

	if (lineedit->infile)
		hg_mem_unlock_object(lineedit->infile->o.mem,
				     lineedit->infile->o.self);

	lineedit->infile = infile;
}

/**
 * hg_lineedit_set_outfile:
 * @lineedit:
 * @outfile:
 *
 * FIXME
 */
void
hg_lineedit_set_outfile(hg_lineedit_t *lineedit,
			hg_file_t     *outfile)
{
	hg_return_if_fail (lineedit != NULL);
	hg_return_if_fail (outfile != NULL);

	if (lineedit->outfile)
		hg_mem_unlock_object(lineedit->outfile->o.mem,
				     lineedit->outfile->o.self);

	lineedit->outfile = outfile;
}

/**
 * hg_lineedit_gc_mark:
 * @lineedit:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_lineedit_gc_mark(hg_lineedit_t  *lineedit,
		    GError        **error)
{
	hg_return_val_if_fail (lineedit != NULL, FALSE);

	return hg_mem_gc_mark(lineedit->mem, lineedit->qself, error);
}
