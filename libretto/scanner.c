/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * scanner.c
 * Copyright (C) 2005-2006 Akira TAGOH
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

#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/hgvaluenode.h>
#include "scanner.h"
#include "operator.h"
#include "vm.h"


#define _libretto_scanner_is_sign(__lb_scanner_c)	\
	((__lb_scanner_c) == '+' ||			\
	 (__lb_scanner_c) == '-')
#define _libretto_scanner_skip_spaces(__lb_scanner_file, __lb_scanner_char) \
	while (1) {							\
		(__lb_scanner_char) = hg_file_object_getc(__lb_scanner_file); \
		if ((__lb_scanner_char) == ' ' ||			\
		    (__lb_scanner_char) == '\t' ||			\
		    (__lb_scanner_char) == '\r' ||			\
		    (__lb_scanner_char) == '\n') {			\
			continue;					\
		}							\
		break;							\
	}
		

struct _LibrettoScanner {
};

typedef enum {
	LB_PUSH_NONE,
	LB_PUSH_OSTACK,
	LB_PUSH_ESTACK,
} LibrettoScannerStackType;

enum {
	LB_TOKEN_LITERAL = 1,
	LB_TOKEN_EVAL_NAME,
	LB_TOKEN_REAL,
	LB_TOKEN_STRING,
	LB_TOKEN_NAME,
	LB_TOKEN_MARK,
	LB_TOKEN_ARRAY,
	LB_TOKEN_DICT,
	LB_TOKEN_PROC,
	LB_TOKEN_PROC_END,
};

static LibrettoScannerCharType __lb_scanner_token[256] = {
	LB_C_NULL, /* 0x00 */		LB_C_NAME, /* 0x01 */
	LB_C_NAME, /* 0x02 */		LB_C_NAME, /* 0x03 */
	LB_C_CONTROL, /* 0x04 */	LB_C_NAME, /* 0x05 */
	LB_C_NAME, /* 0x06 */		LB_C_NAME, /* 0x07 */
	LB_C_NAME, /* 0x08 */		LB_C_SPACE, /* 0x09 (tab \t) */
	LB_C_SPACE, /* 0x0A (LF \n) */	LB_C_NAME, /* 0x0B */
	LB_C_SPACE, /* 0x0C (FF \f) */	LB_C_SPACE, /* 0x0D (CR \r) */
	LB_C_NAME, /* 0x0E */		LB_C_NAME, /* 0x0F */
	LB_C_NAME, /* 0x10 */		LB_C_NAME, /* 0x11 */
	LB_C_NAME, /* 0x12 */		LB_C_NAME, /* 0x13 */
	LB_C_NAME, /* 0x14 */		LB_C_NAME, /* 0x15 */
	LB_C_NAME, /* 0x16 */		LB_C_NAME, /* 0x17 */
	LB_C_NAME, /* 0x18 */		LB_C_NAME, /* 0x19 */
	LB_C_NAME, /* 0x1A */		LB_C_NAME, /* 0x1B */
	LB_C_NAME, /* 0x1C */		LB_C_NAME, /* 0x1D */
	LB_C_NAME, /* 0x1E */		LB_C_NAME, /* 0x1F */
	LB_C_SPACE, /* 0x20 (space) */	LB_C_NAME, /* 0x21 */
	LB_C_NAME, /* 0x22 */		LB_C_NAME, /* 0x23 */
	LB_C_NAME, /* 0x24 */		LB_C_CONTROL, /* 0x25 % */
	LB_C_NAME, /* 0x26 */		LB_C_NAME, /* 0x27 */
	LB_C_CONTROL, /* 0x28 ( */	LB_C_CONTROL, /* 0x29 ) */
	LB_C_NAME, /* 0x2A */		LB_C_NAME, /* 0x2B */
	LB_C_NAME, /* 0x2C */		LB_C_NAME, /* 0x2D */
	LB_C_NAME, /* 0x2E */		LB_C_CONTROL, /* 0x2F / */
	LB_C_NUMERAL, /* 0x30 */	LB_C_NUMERAL, /* 0x31 */
	LB_C_NUMERAL, /* 0x32 */	LB_C_NUMERAL, /* 0x33 */
	LB_C_NUMERAL, /* 0x34 */	LB_C_NUMERAL, /* 0x35 */
	LB_C_NUMERAL, /* 0x36 */	LB_C_NUMERAL, /* 0x37 */
	LB_C_NUMERAL, /* 0x38 */	LB_C_NUMERAL, /* 0x39 */
	LB_C_NAME, /* 0x3A */		LB_C_NAME, /* 0x3B */
	LB_C_CONTROL, /* 0x3C < */	LB_C_NAME, /* 0x3D */
	LB_C_CONTROL, /* 0x3E > */	LB_C_NAME, /* 0x3F */
	LB_C_NAME, /* 0x40 */		LB_C_NAME, /* 0x41 A */
	LB_C_NAME, /* 0x42 B */		LB_C_NAME, /* 0x43 C */
	LB_C_NAME, /* 0x44 D */		LB_C_NAME, /* 0x45 E */
	LB_C_NAME, /* 0x46 F */		LB_C_NAME, /* 0x47 G */
	LB_C_NAME, /* 0x48 H */		LB_C_NAME, /* 0x49 I */
	LB_C_NAME, /* 0x4A J */		LB_C_NAME, /* 0x4B K */
	LB_C_NAME, /* 0x4C L */		LB_C_NAME, /* 0x4D M */
	LB_C_NAME, /* 0x4E N */		LB_C_NAME, /* 0x4F O */
	LB_C_NAME, /* 0x50 P */		LB_C_NAME, /* 0x51 Q */
	LB_C_NAME, /* 0x52 R */		LB_C_NAME, /* 0x53 S */
	LB_C_NAME, /* 0x54 T */		LB_C_NAME, /* 0x55 U */
	LB_C_NAME, /* 0x56 V */		LB_C_NAME, /* 0x57 W */
	LB_C_NAME, /* 0x58 X */		LB_C_NAME, /* 0x59 Y */
	LB_C_NAME, /* 0x5A Z */		LB_C_CONTROL, /* 0x5B [ */
	LB_C_NAME, /* 0x5C */		LB_C_CONTROL, /* 0x5D ] */
	LB_C_NAME, /* 0x5E */		LB_C_NAME, /* 0x5F */
	LB_C_NAME, /* 0x60 */		LB_C_NAME, /* 0x61 a */
	LB_C_NAME, /* 0x62 b */		LB_C_NAME, /* 0x63 c */
	LB_C_NAME, /* 0x64 d */		LB_C_NAME, /* 0x65 e */
	LB_C_NAME, /* 0x66 f */		LB_C_NAME, /* 0x67 g */
	LB_C_NAME, /* 0x68 h */		LB_C_NAME, /* 0x69 i */
	LB_C_NAME, /* 0x6A j */		LB_C_NAME, /* 0x6B k */
	LB_C_NAME, /* 0x6C l */		LB_C_NAME, /* 0x6D m */
	LB_C_NAME, /* 0x6E n */		LB_C_NAME, /* 0x6F o */
	LB_C_NAME, /* 0x70 p */		LB_C_NAME, /* 0x71 q */
	LB_C_NAME, /* 0x72 r */		LB_C_NAME, /* 0x73 s */
	LB_C_NAME, /* 0x74 t */		LB_C_NAME, /* 0x75 u */
	LB_C_NAME, /* 0x76 v */		LB_C_NAME, /* 0x77 w */
	LB_C_NAME, /* 0x78 x */		LB_C_NAME, /* 0x79 y */
	LB_C_NAME, /* 0x7A z */		LB_C_CONTROL, /* 0x7B { */
	LB_C_NAME, /* 0x7C */		LB_C_CONTROL, /* 0x7D } */
	LB_C_NAME, /* 0x7E */		LB_C_NAME, /* 0x7F */
	LB_C_BINARY, /* 0x80 */		LB_C_BINARY, /* 0x81 */
	LB_C_BINARY, /* 0x82 */		LB_C_BINARY, /* 0x83 */
	LB_C_BINARY, /* 0x84 */		LB_C_BINARY, /* 0x85 */
	LB_C_BINARY, /* 0x86 */		LB_C_BINARY, /* 0x87 */
	LB_C_BINARY, /* 0x88 */		LB_C_BINARY, /* 0x89 */
	LB_C_BINARY, /* 0x8A */		LB_C_BINARY, /* 0x8B */
	LB_C_BINARY, /* 0x8C */		LB_C_BINARY, /* 0x8D */
	LB_C_BINARY, /* 0x8E */		LB_C_BINARY, /* 0x8F */
	LB_C_BINARY, /* 0x90 */		LB_C_BINARY, /* 0x91 */
	LB_C_BINARY, /* 0x92 */		LB_C_BINARY, /* 0x93 */
	LB_C_BINARY, /* 0x94 */		LB_C_BINARY, /* 0x95 */
	LB_C_BINARY, /* 0x96 */		LB_C_BINARY, /* 0x97 */
	LB_C_BINARY, /* 0x98 */		LB_C_BINARY, /* 0x99 */
	LB_C_BINARY, /* 0x9A */		LB_C_BINARY, /* 0x9B */
	LB_C_BINARY, /* 0x9C */		LB_C_BINARY, /* 0x9D */
	LB_C_BINARY, /* 0x9E */		LB_C_BINARY, /* 0x9F */
	LB_C_BINARY, /* 0xA0 */		LB_C_NAME, /* 0xA1 */
	LB_C_NAME, /* 0xA2 */		LB_C_NAME, /* 0xA3 */
	LB_C_NAME, /* 0xA4 */		LB_C_NAME, /* 0xA5 */
	LB_C_NAME, /* 0xA6 */		LB_C_NAME, /* 0xA7 */
	LB_C_NAME, /* 0xA8 */		LB_C_NAME, /* 0xA9 */
	LB_C_NAME, /* 0xAA */		LB_C_NAME, /* 0xAB */
	LB_C_NAME, /* 0xAC */		LB_C_NAME, /* 0xAD */
	LB_C_NAME, /* 0xAE */		LB_C_NAME, /* 0xAF */
	LB_C_NAME, /* 0xB0 */		LB_C_NAME, /* 0xB1 */
	LB_C_NAME, /* 0xB2 */		LB_C_NAME, /* 0xB3 */
	LB_C_NAME, /* 0xB4 */		LB_C_NAME, /* 0xB5 */
	LB_C_NAME, /* 0xB6 */		LB_C_NAME, /* 0xB7 */
	LB_C_NAME, /* 0xB8 */		LB_C_NAME, /* 0xB9 */
	LB_C_NAME, /* 0xBA */		LB_C_NAME, /* 0xBB */
	LB_C_NAME, /* 0xBC */		LB_C_NAME, /* 0xBD */
	LB_C_NAME, /* 0xBE */		LB_C_NAME, /* 0xBF */
	LB_C_NAME, /* 0xC0 */		LB_C_NAME, /* 0xC1 */
	LB_C_NAME, /* 0xC2 */		LB_C_NAME, /* 0xC3 */
	LB_C_NAME, /* 0xC4 */		LB_C_NAME, /* 0xC5 */
	LB_C_NAME, /* 0xC6 */		LB_C_NAME, /* 0xC7 */
	LB_C_NAME, /* 0xC8 */		LB_C_NAME, /* 0xC9 */
	LB_C_NAME, /* 0xCA */		LB_C_NAME, /* 0xCB */
	LB_C_NAME, /* 0xCC */		LB_C_NAME, /* 0xCD */
	LB_C_NAME, /* 0xCE */		LB_C_NAME, /* 0xCF */
	LB_C_NAME, /* 0xD0 */		LB_C_NAME, /* 0xD1 */
	LB_C_NAME, /* 0xD2 */		LB_C_NAME, /* 0xD3 */
	LB_C_NAME, /* 0xD4 */		LB_C_NAME, /* 0xD5 */
	LB_C_NAME, /* 0xD6 */		LB_C_NAME, /* 0xD7 */
	LB_C_NAME, /* 0xD8 */		LB_C_NAME, /* 0xD9 */
	LB_C_NAME, /* 0xDA */		LB_C_NAME, /* 0xDB */
	LB_C_NAME, /* 0xDC */		LB_C_NAME, /* 0xDD */
	LB_C_NAME, /* 0xDE */		LB_C_NAME, /* 0xDF */
	LB_C_NAME, /* 0xE0 */		LB_C_NAME, /* 0xE1 */
	LB_C_NAME, /* 0xE2 */		LB_C_NAME, /* 0xE3 */
	LB_C_NAME, /* 0xE4 */		LB_C_NAME, /* 0xE5 */
	LB_C_NAME, /* 0xE6 */		LB_C_NAME, /* 0xE7 */
	LB_C_NAME, /* 0xE8 */		LB_C_NAME, /* 0xE9 */
	LB_C_NAME, /* 0xEA */		LB_C_NAME, /* 0xEB */
	LB_C_NAME, /* 0xEC */		LB_C_NAME, /* 0xED */
	LB_C_NAME, /* 0xEE */		LB_C_NAME, /* 0xEF */
	LB_C_NAME, /* 0xF0 */		LB_C_NAME, /* 0xF1 */
	LB_C_NAME, /* 0xF2 */		LB_C_NAME, /* 0xF3 */
	LB_C_NAME, /* 0xF4 */		LB_C_NAME, /* 0xF5 */
	LB_C_NAME, /* 0xF6 */		LB_C_NAME, /* 0xF7 */
	LB_C_NAME, /* 0xF8 */		LB_C_NAME, /* 0xF9 */
	LB_C_NAME, /* 0xFA */		LB_C_NAME, /* 0xFB */
	LB_C_NAME, /* 0xFC */		LB_C_NAME, /* 0xFD */
	LB_C_NAME, /* 0xFE */		LB_C_NAME, /* 0xFF */
};

/*
 * Private Functions
 */
HgValueNode *
_libretto_scanner_get_object(LibrettoVM   *vm,
			     HgFileObject *file)
{
	HgMemPool *pool;
	LibrettoStack *estack, *ostack;
	HgDict *systemdict;
	HgValueNode *node, *retval = NULL;
	guchar c;
	gboolean need_loop = TRUE, sign = FALSE, negate = FALSE, numeral = FALSE;
	gboolean power = FALSE, psign = FALSE, pnegate = FALSE;
	gint token_type = 0, string_depth = 0;
	HgString *string = NULL;
	HgArray *array = NULL;
	gdouble d = 0.0L, real = 0.0L, de;
	gint32 radix = 10, i = 0, e = 0;

	pool = libretto_vm_get_current_pool(vm);
	ostack = libretto_vm_get_ostack(vm);
	estack = libretto_vm_get_estack(vm);
	systemdict = libretto_vm_get_dict_systemdict(vm);

	/* skip the white spaces first */
	_libretto_scanner_skip_spaces(file, c);

	if (c == 0 && hg_file_object_is_eof(file))
		return NULL;

	hg_file_object_ungetc(file, c);
	while (need_loop) {
		c = hg_file_object_getc(file);
		switch (__lb_scanner_token[c]) {
		    case LB_C_NULL:
			    need_loop = FALSE;
			    break;
		    case LB_C_CONTROL:
			    if (token_type == LB_TOKEN_LITERAL && c == '/') {
				    if (hg_string_length(string) > 0) {
					    hg_file_object_ungetc(file, c);
					    need_loop = FALSE;
				    } else {
					    token_type = LB_TOKEN_EVAL_NAME;
				    }
				    break;
			    } else if (token_type == LB_TOKEN_STRING) {
				    if (c == ')') {
					    string_depth--;
					    if (string_depth == 0) {
						    need_loop = FALSE;
						    break;
					    }
				    } else if (c == '(') {
					    string_depth++;
				    }
				    if (!hg_string_append_c(string, c)) {
					    _libretto_operator_set_error(vm,
									 __lb_operator_list[LB_op_token],
									 LB_e_VMerror);
					    return NULL;
				    }
				    break;
			    } else if (token_type != 0) {
				    hg_file_object_ungetc(file, c);
				    need_loop = FALSE;
				    break;
			    }
			    switch (c) {
				case '/':
					token_type = LB_TOKEN_LITERAL;
					string = hg_string_new(pool, -1);
					break;
				case '(':
					token_type = LB_TOKEN_STRING;
					string = hg_string_new(pool, -1);
					string_depth++;
					break;
				case ')':
					_libretto_operator_set_error(vm,
								     __lb_operator_list[LB_op_token],
								     LB_e_syntaxerror);
					need_loop = FALSE;
					break;
				case '<':
					c = hg_file_object_getc(file);
					if (c == '<') {
						/* dict mark */
						token_type = LB_TOKEN_MARK;
						need_loop = FALSE;
					} else if (c == '~') {
						/* ASCII85 encoding */
						g_warning("FIXME: implement me");
					} else {
						g_warning("FIXME: implement me");
						hg_file_object_ungetc(file, c);
					}
					break;
				case '>':
					c = hg_file_object_getc(file);
					if (c == '>') {
						token_type = LB_TOKEN_DICT;
						need_loop = FALSE;
					} else if (c == '~') {
						/* ASCII85 encoding */
						g_warning("FIXME: implement me");
					} else {
						g_warning("FIXME: implement me");
						hg_file_object_ungetc(file, c);
					}
					break;
				case '[':
					token_type = LB_TOKEN_MARK;
					need_loop = FALSE;
					break;
				case ']':
					token_type = LB_TOKEN_ARRAY;
					need_loop = FALSE;
					break;
				case '{':
					array = hg_array_new(pool, -1);
					if (array == NULL) {
						_libretto_operator_set_error(vm,
									     __lb_operator_list[LB_op_token],
									     LB_e_VMerror);
						break;
					}
					while (1) {
						node = _libretto_scanner_get_object(vm, file);
						if (libretto_vm_has_error(vm)) {
							return NULL;
						}
						if (node == NULL) {
							_libretto_operator_set_error(vm,
										     __lb_operator_list[LB_op_token],
										     LB_e_syntaxerror);
							break;
						}
						if (HG_IS_VALUE_NULL (node) && HG_VALUE_GET_NULL (node) != NULL) {
							/* HACK: assume that this is a mark for proc end. */
							token_type = LB_TOKEN_PROC;
							hg_array_fix_array_size(array);
							break;
						}
						if (!hg_array_append(array, node)) {
							_libretto_operator_set_error(vm,
										     __lb_operator_list[LB_op_token],
										     LB_e_VMerror);
							return NULL;
						}
					}
					need_loop = FALSE;
					break;
				case '}':
					token_type = LB_TOKEN_PROC_END;
					need_loop = FALSE;
					break;
				case 0x04:
					need_loop = FALSE;
					break;
				case '%':
					/* FIXME: parse comment? */
					while (1) {
						c = hg_file_object_getc(file);
						if (c == 0 || c == '\r' || c == '\n')
							break;
					}
					_libretto_scanner_skip_spaces(file, c);
					hg_file_object_ungetc(file, c);
					break;
				default:
					g_warning("[BUG] unknown control object %c.", c);
					break;
			    }
			    break;
		    case LB_C_NAME:
			    if (c == '\\') {
				    c = hg_file_object_getc(file);
				    switch (c) {
					case 'n':
						c = '\n';
						break;
					case 'r':
						c = '\r';
						break;
					case 't':
						c = '\t';
						break;
					case 'b':
						c = '\b';
						break;
					case 'f':
						c = '\f';
						break;
					default:
						break;
				    }
			    }
			    switch (token_type) {
				case 0:
					/* in this case, the token might be
					 * name object or possibly real.
					 */
					string = hg_string_new(pool, -1);
					if (!hg_string_append_c(string, c)) {
						_libretto_operator_set_error(vm,
									     __lb_operator_list[LB_op_token],
									     LB_e_VMerror);
						return NULL;
					}
					if (_libretto_scanner_is_sign(c)) {
						numeral = TRUE;
						sign = TRUE;
						if (c == '-')
							negate = TRUE;
					} else if (c == '.') {
						numeral = TRUE;
						real = 10.0L;
						d = i;
					}
					token_type = LB_TOKEN_NAME;
					break;
				case LB_TOKEN_NAME:
					if (!power && HG_VALUE_REAL_SIMILAR (real, 0) && c == '.') {
						real = 10.0L;
						d = i;
					} else if (!power && (c == 'e' || c == 'E')) {
						power = TRUE;
						if (HG_VALUE_REAL_SIMILAR (real, 0)) {
							real = 10.0L;
							d = i;
						}
						if (!hg_string_append_c(string, c)) {
							_libretto_operator_set_error(vm,
										     __lb_operator_list[LB_op_token],
										     LB_e_VMerror);
							return NULL;
						}
						c = hg_file_object_getc(file);
						if (_libretto_scanner_is_sign(c)) {
							psign = TRUE;
							if (c == '-') {
								pnegate = TRUE;
							}
						} else {
							if (c < '0' || c > '9')
								numeral = FALSE;
							hg_file_object_ungetc(file, c);
							break;
						}
					} else {
						numeral = FALSE;
					}
				case LB_TOKEN_LITERAL:
				case LB_TOKEN_EVAL_NAME:
				case LB_TOKEN_STRING:
					if (!hg_string_append_c(string, c)) {
						_libretto_operator_set_error(vm,
									     __lb_operator_list[LB_op_token],
									     LB_e_VMerror);
						return NULL;
					}
					break;
				default:
					break;
			    }
			    break;
		    case LB_C_SPACE:
			    if (token_type == LB_TOKEN_STRING) {
				    if (!hg_string_append_c(string, c)) {
					    _libretto_operator_set_error(vm,
									 __lb_operator_list[LB_op_token],
									 LB_e_VMerror);
					    return NULL;
				    }
			    } else {
				    need_loop = FALSE;
			    }
			    break;
		    case LB_C_NUMERAL:
			    switch (token_type) {
				case 0:
					/* it might be a numeral, but possibly name. */
					string = hg_string_new(pool, -1);
					if (!hg_string_append_c(string, c)) {
						_libretto_operator_set_error(vm,
									     __lb_operator_list[LB_op_token],
									     LB_e_VMerror);
						return NULL;
					}
					token_type = LB_TOKEN_NAME;
					numeral = TRUE;
					i = c - '0';
					break;
				case LB_TOKEN_LITERAL:
				case LB_TOKEN_EVAL_NAME:
				case LB_TOKEN_STRING:
				case LB_TOKEN_NAME:
					if (numeral) {
						if (power) {
							e *= 10;
							e += c - '0';
						} else if (!HG_VALUE_REAL_SIMILAR (real, 0)) {
							d += (c - '0') / real;
							real *= 10;
						} else {
							i *= radix;
							i += c - '0';
						}
					}
					if (!hg_string_append_c(string, c)) {
						_libretto_operator_set_error(vm,
									     __lb_operator_list[LB_op_token],
									     LB_e_VMerror);
						return NULL;
					}
					break;
				default:
					break;
			    }
			    break;
		    case LB_C_BINARY:
			    break;
		    default:
			    g_warning("Unknown character %c(%d)[%d]", c, c, __lb_scanner_token[c]);
			    break;
		}
	}
	switch (token_type) {
	    case LB_TOKEN_LITERAL:
		    retval = libretto_vm_get_name_node(vm, hg_string_get_string(string));
		    hg_object_unexecutable((HgObject *)retval);
		    hg_mem_free(string);
		    break;
	    case LB_TOKEN_EVAL_NAME:
		    node = libretto_vm_get_name_node(vm, hg_string_get_string(string));
		    retval = libretto_vm_lookup(vm, node);
		    if (retval == NULL) {
			    _libretto_operator_set_error(vm,
							 __lb_operator_list[LB_op_token],
							 LB_e_undefined);
		    }
		    hg_mem_free(string);
		    break;
	    case LB_TOKEN_NAME:
		    if (numeral) {
			    if (pnegate)
				    e = -e;
			    if (!HG_VALUE_REAL_SIMILAR (real, 0)) {
				    if (power) {
					    de = exp10((double)e);
					    d *= de;
				    }
				    if (negate)
					    d = -d;
				    HG_VALUE_MAKE_REAL (pool, retval, d);
			    } else {
				    if (negate)
					    i = -i;
				    HG_VALUE_MAKE_INTEGER (pool, retval, i);
			    }
		    } else {
			    retval = libretto_vm_get_name_node(vm, hg_string_get_string(string));
			    hg_object_executable((HgObject *)retval);
		    }
		    hg_mem_free(string);
		    break;
	    case LB_TOKEN_MARK:
		    HG_VALUE_MAKE_MARK (pool, retval);
		    break;
	    case LB_TOKEN_ARRAY:
		    retval = hg_dict_lookup_with_string(systemdict, "%arraytomark");
		    break;
	    case LB_TOKEN_DICT:
		    retval = hg_dict_lookup_with_string(systemdict, "%dicttomark");
		    break;
	    case LB_TOKEN_PROC:
		    HG_VALUE_MAKE_ARRAY (retval, array);
		    hg_object_executable((HgObject *)retval);
		    break;
	    case LB_TOKEN_PROC_END:
		    HG_VALUE_MAKE_NULL (pool, retval, GINT_TO_POINTER (1));
		    break;
	    case LB_TOKEN_STRING:
		    if (string_depth == 0) {
			    hg_string_fix_string_size(string);
			    HG_VALUE_MAKE_STRING (retval, string);
		    }
		    break;
	    default:
		    g_warning("FIXME: unknown token type %d\n", token_type);
		    break;
	}

	node = hg_dict_lookup_with_string(libretto_vm_get_dict_statusdict(vm), "%initialized");
	if ((node == NULL ||
	     HG_VALUE_GET_BOOLEAN (node) == FALSE) &&
	    retval != NULL && !HG_IS_VALUE_STRING (retval)) {
		hg_object_unwritable((HgObject *)retval);
	}

	return retval;
}

/*
 * Public Functions
 */
HgValueNode *
libretto_scanner_get_object(LibrettoVM *vm,
			    HgFileObject *file)
{
	g_return_val_if_fail (vm != NULL, NULL);
	g_return_val_if_fail (file != NULL, NULL);

	return _libretto_scanner_get_object(vm, file);
}
