/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgscanner.l
 * Copyright (C) 2010-2011 Akira TAGOH
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

#include <ctype.h>
#include <math.h>
/* XXX: GLib is still needed for GString */
#include <glib.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgint.h"
#include "hgname.h"
#include "hgreal.h"
#include "hgutils.h"
#include "hgvm.h"
#include "hgscanner.h"

#include "hgscanner.proto.h"

#define HG_SCANNER_MEM_SIZE	100000
#define YY_DECL			static hg_bool_t _hg_scanner_yylex(hg_scanner_t *scanner, yyscan_t yyscanner, hg_vm_t *vm)
#define yyterminate()		{BEGIN(INITIAL);return FALSE;}
#define YY_NO_INPUT
#define YY_INPUT(_buf_,_result_,_max_size_)			\
	HG_STMT_START {						\
		hg_scanner_t *_scanner_ = yyextra;		\
								\
		if (!hg_file_is_eof(_scanner_->infile)) {		\
			_result_ = hg_file_read(_scanner_->infile,	\
						(_buf_),		\
						1,			\
						1);			\
			if (hg_file_is_eof(_scanner_->infile)) {	\
				_result_ = 1;				\
				(_buf_)[0] = ' ';			\
				(_buf_)[1] = 0;				\
			}						\
		} else {						\
			_result_ = 0;					\
		}							\
	} HG_STMT_END
#define YY_EXTRA_TYPE		hg_scanner_t *
#define YY_USER_INIT		yyextra = scanner;
#define ECHO			do { if (fwrite(yytext, yyleng, 1, yyout)){}; fflush(yyout); g_assert_not_reached();} while (0)


struct _hg_scanner_t {
	yyscan_t    yyscanner;
	hg_mem_t   *mem;
	hg_quark_t  self;
	hg_quark_t  result;
	hg_file_t  *infile;
};

#ifndef yyget_column
int yyget_column(yyscan_t yyscanner);
#endif
#ifndef yyset_column
void yyset_column(int column_no, yyscan_t yyscanner);
#endif
%}
%option nounput noyywrap warn nounistd reentrant
%option prefix="hgscanner"
%x STRING HEXSTRING ASCII85 DSC

CONTROL		[\x04%\(\)/<>\[\]\{\}]
SPACE		[\t\f\r\n\0 ]
CTRL_OR_SPACE	[\x04%\(\)/<>\[\]\{\}\t\f\r\n\0 ]
HEXDIGITS	[0-9a-fA-F]
%%
	hg_int_t string_count = 0;
	GString *string = NULL;
	static const hg_char_t *const radix_index = "0123456789abcdefghijklmnopqrstuvwxyz";
	hg_char_t buf_ascii85[8];
	hg_int_t ascii85_counter = 0;

<DSC>{
	"!PS-Adobe-"[0-9.]+.*	{
		if (yyget_lineno(scanner->yyscanner) == 1) {
			hg_int_t i;
			hg_real_t v = 0.0;
			hg_vm_langlevel_t l;
			GString *s = g_string_new(NULL);

			for (i = 10; i < yyleng; i++) {
				if ((yytext[i] < '0' || yytext[i] > '9') &&
				    yytext[i] != '.')
					break;
				g_string_append_c(s, yytext[i]);
			}

			sscanf((hg_char_t *)s->str, "%lf", &v);
			l = (hg_uint_t)v - 1;
			if (l < HG_LANG_LEVEL_1)
				l = HG_LANG_LEVEL_1;
			if (l >= HG_LANG_LEVEL_END)
				l = HG_LANG_LEVEL_3;
			if (hg_vm_get_language_level(vm) != l)
				hg_vm_set_language_level(vm, l);
		}
		BEGIN(INITIAL);
	}
	.*|\r\n|\r|\n	{
		BEGIN(INITIAL);
	}
	<<EOF>>	{
		yyterminate();
	}
}
\%	{
	BEGIN(DSC);
}
<STRING>{
	"("	{
		string_count++;
		g_string_append_c(string, yytext[0]);
	}
	")"	{
		string_count--;
		if (string_count == 0) {
			hg_bool_t retval = TRUE;

			BEGIN(INITIAL);
			scanner->result = hg_string_new_with_value(hg_vm_get_mem(vm),
								   string->str,
								   string->len,
								   NULL);
			if (scanner->result == Qnil) {
				retval = FALSE;
				/* yyterminate(); */
			}
			g_string_free(string, TRUE);
			return retval;
		} else {
			g_string_append_c(string, yytext[0]);
		}
	}
	\\[0-7]{1,3}	{
		hg_int_t result;

		sscanf(yytext + 1,
		       "%o",
		       &result);
		if (result > 0xff) {
			hg_debug(HG_MSGCAT_SCAN, "Octadecimal code is too big");
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
			g_string_free(string, TRUE);
			yyterminate();
		}
		g_string_append_c(string, result);
	}
	\\[0-9]+	{
		hg_debug(HG_MSGCAT_SCAN, "Bad octal sequence");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
		g_string_free(string, TRUE);
		yyterminate();
	}
	"\\n"		{
		g_string_append_c(string, '\n');
	}
	"\\r"		{
		g_string_append_c(string, '\r');
	}
	"\\t"		{
		g_string_append_c(string, '\t');
	}
	"\\b"		{
		g_string_append_c(string, '\b');
	}
	"\\f"		{
		g_string_append_c(string, '\f');
	}
	\\(\n|\r|\r\n)	{
		/* ignore */
	}
	\\.	{
		g_string_append_c(string, yytext[1]);
	}
	.|\r\n|\r|\n	{
		g_string_append_len(string, yytext, yyleng);
	}
	<<EOF>>		{
		hg_debug(HG_MSGCAT_SCAN, "Reached the end of the file during scanning the string");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
		g_string_free(string, TRUE);
		yyterminate();
	}
}
\(	{
		string_count++;
		BEGIN(STRING);
		string = g_string_new(NULL);
	}
<ASCII85>{
	"~>"	{
		hg_bool_t retval = TRUE;

		BEGIN(INITIAL);
		scanner->result = hg_string_new_with_value(hg_vm_get_mem(vm),
							   string->str,
							   string->len,
							   NULL);
		if (scanner->result == Qnil) {
			retval = FALSE;
			/* yyterminate(); */
		}
		g_string_free(string, TRUE);
		return retval;
	}
	{SPACE}		{
		/* ignore */
	}
	[!-uz]	{
		if (yytext[0] == 'z') {
			if (ascii85_counter == 0) {
				/* special case to take care of all-zero */
				g_string_append_c(string, 0);
				g_string_append_c(string, 0);
				g_string_append_c(string, 0);
				g_string_append_c(string, 0);
			} else {
				hg_debug(HG_MSGCAT_SCAN, "Syntax error in ASCII85 format");
				hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
				g_string_free(string, TRUE);
				yyterminate();
			}
		} else {
			buf_ascii85[ascii85_counter++] = yytext[0];
			if (ascii85_counter == 5) {
				guint64 decoded = 0;

				decoded += (buf_ascii85[0] - 33) * 0x31c84b1LL;
				decoded += (buf_ascii85[1] - 33) * 0x95eedLL;
				decoded += (buf_ascii85[2] - 33) * 0x1c39LL;
				decoded += (buf_ascii85[3] - 33) * 0x55LL;
				decoded += (buf_ascii85[4] - 33);
				if (decoded > HG_MAXUINT) {
					hg_debug(HG_MSGCAT_SCAN, "Syntax error in ASCII85 format");
					hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
					g_string_free(string, TRUE);
					yyterminate();
				}
				g_string_append_c(string, (decoded & 0xff000000) >> 24);
				g_string_append_c(string, (decoded & 0x00ff0000) >> 16);
				g_string_append_c(string, (decoded & 0x0000ff00) >> 8);
				g_string_append_c(string, (decoded & 0x000000ff));
				ascii85_counter = 0;
			}
		}
	}
	<<EOF>>		{
		hg_debug(HG_MSGCAT_SCAN, "Reached the end of the file during scanning the hex string");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
		g_string_free(string, TRUE);
		yyterminate();
	}
	.	{
		hg_debug(HG_MSGCAT_SCAN, "Syntax error");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
		g_string_free(string, TRUE);
		yyterminate();
	}
}
\<~	{
	BEGIN(ASCII85);
	string = g_string_new(NULL);
	ascii85_counter = 0;
}
<HEXSTRING>{
	">"	{
		hg_bool_t retval = TRUE;

		BEGIN(INITIAL);
		scanner->result = hg_string_new_with_value(hg_vm_get_mem(vm),
							   string->str,
							   string->len,
							   NULL);
		if (scanner->result == Qnil) {
			retval = FALSE;
			/* yyterminate(); */
		}
		g_string_free(string, TRUE);
		return retval;
	}
	{HEXDIGITS}{2}/[0-9a-fA-F\t\f\r\n\0> ]	{
		hg_int_t i;
		hg_char_t *p;
		hg_usize_t index;
		hg_int_t result = 0;

		for (i = 0; i < yyleng; i++) {
			p = strchr(radix_index, tolower(yytext[i]));
			index = p - radix_index;
			result *= 16;
			result += index;
		}
		if (i == 1)
			result *= 16;
		g_string_append_c(string, result);
	}
	{HEXDIGITS}{1}/[\t\f\r\n\0> ]	{
		hg_int_t i;
		hg_char_t *p;
		hg_usize_t index;
		hg_int_t result = 0;

		for (i = 0; i < yyleng; i++) {
			p = strchr(radix_index, tolower(yytext[i]));
			index = p - radix_index;
			result *= 16;
			result += index;
		}
		if (i == 1)
			result *= 16;
		g_string_append_c(string, result);
	}
	{SPACE}		{
		/* ignore */
	}
	<<EOF>>		{
		hg_debug(HG_MSGCAT_SCAN, "Reached the end of the file during scanning the hex string");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
		g_string_free(string, TRUE);
		yyterminate();
	}
	.	{
		hg_debug(HG_MSGCAT_SCAN, "Syntax error");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
		g_string_free(string, TRUE);
		yyterminate();
	}
}
\<	{
		BEGIN(HEXSTRING);
		string = g_string_new(NULL);
	}
\/\/[^\x04%\(\)/<>\[\]\{\}\t\f\r\n ]*/{CTRL_OR_SPACE}	{
		scanner->result = HG_QEVALNAME (&yytext[2]);
		hg_quark_set_executable(&scanner->result, TRUE);
		return TRUE;
	}
\/[^\x04%\(\)\/<>\[\]\{\}\t\f\r\n ]*/{CTRL_OR_SPACE}	{
		scanner->result = HG_QNAME (&yytext[1]);
		return TRUE;
	}
[\x80-\xa1]+		{
		/* binary tokens */
		hg_warning("XXX: binary token appears");
	}
\r\n			{
		/* ignore */
	}
\r			{
		/* ignore */
	}
\n			{
		/* ignore */
	}
[\t\f ]+		{
		/* ignore */
	}
[+-]?[0-9]*\.[0-9]+|[+-]?[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+/{CTRL_OR_SPACE}	{
		hg_real_t result = 0.0L;

		sscanf(yytext, "%lf", &result);
		scanner->result = HG_QREAL (result);

		return TRUE;
	}
[0-9]+#[0-9a-zA-Z]+/{CTRL_OR_SPACE}	{
		hg_real_t fradix = 0.0L;
		hg_int_t result = 0;
		hg_usize_t radix, len, i, index;
		hg_char_t *s, *p;

		s = (hg_char_t *)hg_malloc(sizeof (hg_char_t) * yyleng);
		sscanf(yytext, "%lf#%s", &fradix, s);
		radix = fradix;
		if (fradix > HG_MAXINT ||
		    fradix < HG_MININT ||
		    radix < 2 ||
		    radix > 36) {
			hg_free(s);
			REJECT;
		}
		len = strlen(s);
		for (i = 0; i < len; i++) {
			p = strchr(radix_index, tolower(s[i]));
			index = p - radix_index;
			if (index >= radix) {
				hg_free(s);
				REJECT;
			}
			if ((result > 0 && (result * radix) < 0) ||
			    (result < 0 && (result * radix) > 0)) {
				hg_debug(HG_MSGCAT_SCAN, "Digit is too big");
				hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
				hg_free(s);
				yyterminate();
			}
			result *= radix;
			result += index;
		}
		hg_free(s);
		scanner->result = HG_QINT (result);
		return TRUE;
	}
[+-]?[0-9]+/{CTRL_OR_SPACE}	{
		hg_usize_t i, pos = 0;
		hg_real_t result = 0.0L, f;
		hg_int_t sign = 0;
		hg_real_t exp10i[] = {
			1L,
			10L,
			100L,
			1000L,
			10000L,
			100000L,
			1000000L,
			10000000L,
			100000000L,
			1000000000L
		};

		if (yytext[0] == '+' ||
		    yytext[0] == '-') {
			sign = (yytext[0] == '-' ? -1 : 0);
			pos++;
		}
		for (i = pos; i < yyleng; i++) {
			f = (hg_real_t)(long)(yytext[i] - '0');
			if ((yyleng - i - 1) < 10) {
				result += f * exp10i[(yyleng - i - 1)];
			} else {
				result += f * exp10((hg_real_t)(yyleng - i - 1));
			}
		}
		if (sign < 0)
			result = -result;
		if (isinf(result) ||
		    isnan(result)) {
			hg_debug(HG_MSGCAT_SCAN, "Digit is too big");
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_syntaxerror);
			yyterminate();
		}
		if (HG_REAL_LE (result, HG_MAXINT) &&
		    HG_REAL_GE (result, HG_MININT)) {
			scanner->result = HG_QINT ((hg_int_t)result);
		} else {
			scanner->result = HG_QREAL (result);
		}
		return TRUE;
	}
\<\<|\>\>|\[|\]|\{|\}	{
		scanner->result = HG_QNAME (yytext);
		hg_quark_set_executable(&scanner->result, TRUE);
		return TRUE;
	}
[^\x04%\(\)/\<\>\[\]\{\}\t\f\r\n ]+/{CTRL_OR_SPACE}	{
		scanner->result = HG_QNAME (yytext);
		hg_quark_set_executable(&scanner->result, TRUE);
		return TRUE;
	}
<<EOF>>		{
		yyterminate();
	}
%%
/**
 * hg_scanner_new:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_scanner_t *
hg_scanner_new(hg_mem_t  *mem)
{
	hg_scanner_t *retval;
	hg_quark_t q;

	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);

	q = hg_mem_alloc(mem, sizeof (hg_scanner_t), (hg_pointer_t *)&retval);
	if (q == Qnil)
		return NULL;
	memset(retval, 0, sizeof (hg_scanner_t));
	retval->mem = mem;
	retval->self = q;
	retval->result = Qnil;

	yylex_init(&retval->yyscanner);
	if (retval->yyscanner == NULL) {
		hg_scanner_destroy(retval);
		return NULL;
	}

	hg_mem_ref(mem, q);

	return retval;
}

/**
 * hg_scanner_destroy:
 * @scanner:
 *
 * FIXME
 */
void
hg_scanner_destroy(hg_scanner_t *scanner)
{
	hg_return_if_fail (scanner != NULL, HG_e_VMerror);

	if (scanner->yyscanner)
		yylex_destroy(scanner->yyscanner);
	hg_mem_free(scanner->mem, scanner->self);
}

/**
 * hg_scanner_attach_file:
 * @scanner:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_scanner_attach_file(hg_scanner_t *scanner,
		       hg_file_t    *file)
{
	YY_BUFFER_STATE yystate;
	struct yyguts_t *yyg HG_GNUC_UNUSED;

	hg_return_val_if_fail (scanner != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (file != NULL, FALSE, HG_e_VMerror);

	yyg = (struct yyguts_t *)scanner->yyscanner;
	yystate = hg_file_get_yybuffer(file);
	if (yystate == NULL) {
		yystate = yy_create_buffer(NULL, YY_BUF_SIZE, scanner->yyscanner);
		hg_file_set_yybuffer(file,
				     yystate,
				     (hg_file_yybuffer_finalizer_func_t)yy_delete_buffer,
				     scanner->yyscanner);
	}
	if (scanner->infile != file) {
		if (scanner->infile != NULL)
			hg_mem_unlock_object(scanner->infile->o.mem,
					     scanner->infile->o.self);

		hg_mem_lock_object(file->o.mem,
				   file->o.self);
		scanner->infile = file;

		yy_switch_to_buffer(yystate, scanner->yyscanner);
		yyset_lineno(hg_file_get_lineno(file), scanner->yyscanner);
	}

	return TRUE;
}

/**
 * hg_scanner_scan:
 * @scanner:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_scanner_scan(hg_scanner_t *scanner,
		hg_vm_t      *vm)
{
	hg_bool_t retval;

	hg_return_val_if_fail (scanner != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	scanner->result = Qnil;

	retval = _hg_scanner_yylex(scanner, scanner->yyscanner, vm);
	hg_file_set_lineno(scanner->infile, yyget_lineno(scanner->yyscanner));

	return retval;
}

/**
 * hg_scanner_get_token:
 * @scanner:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_scanner_get_token(hg_scanner_t *scanner)
{
	hg_return_val_if_fail (scanner != NULL, Qnil, HG_e_VMerror);

	hg_debug(HG_MSGCAT_SCAN, "Scanned object: 0x%lx", scanner->result);

	return scanner->result;
}

/**
 * hg_scanner_get_infile:
 * @scanner:
 *
 * FIXME
 *
 * Returns:
 */
hg_file_t *
hg_scanner_get_infile(hg_scanner_t *scanner)
{
	hg_return_val_if_fail (scanner != NULL, NULL, HG_e_VMerror);

	return scanner->infile;
}
