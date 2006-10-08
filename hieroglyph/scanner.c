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

#include <ctype.h>
#include "scanner.h"
#include "hgmem.h"
#include "hgarray.h"
#include "hgdict.h"
#include "hgfile.h"
#include "hgstring.h"
#include "hgvaluenode.h"
#include "iarray.h"
#include "operator.h"
#include "vm.h"


static gboolean _hg_scanner_parse_number(HgVM          *vm,
					 HgFileObject  *file,
					 gint           token_type,
					 gint           radix,
					 gint           sign,
					 gboolean       is_integer,
					 HgString      *string,
					 HgValueNode  **node,
					 gboolean      *error);

#define _hg_scanner_is_sign(__hg_scanner_c)		\
	((__hg_scanner_c) == '+' ||			\
	 (__hg_scanner_c) == '-')
#define _hg_scanner_skip_spaces(__hg_scanner_file, __hg_scanner_char)	\
	while (1) {							\
		(__hg_scanner_char) = hg_file_object_getc(__hg_scanner_file); \
		if (_hg_scanner_isspace(__hg_scanner_char)) {		\
			continue;					\
		}							\
		break;							\
	}
#define _hg_scanner_set_error(v, o, e)					\
	G_STMT_START {							\
		HgValueNode *__hg_op_node;				\
									\
		HG_VALUE_MAKE_OPERATOR (__hg_op_node, (o));		\
		hg_vm_set_error((v), __hg_op_node, (e), FALSE);	\
	} G_STMT_END
		

struct _HieroGlyphScanner {
};

typedef enum {
	HG_SCAN_PUSH_NONE,
	HG_SCAN_PUSH_OSTACK,
	HG_SCAN_PUSH_ESTACK,
} HgScannerStackType;

enum {
	HG_SCAN_TOKEN_LITERAL = 1,
	HG_SCAN_TOKEN_EVAL_NAME,
	HG_SCAN_TOKEN_NUMERIC,
	HG_SCAN_TOKEN_STRING,
	HG_SCAN_TOKEN_NAME,
	HG_SCAN_TOKEN_MARK,
	HG_SCAN_TOKEN_ARRAY,
	HG_SCAN_TOKEN_DICT,
	HG_SCAN_TOKEN_PROC,
	HG_SCAN_TOKEN_PROC_END,
};

static HgScannerCharType __hg_scanner_token[256] = {
	HG_SCAN_C_NULL, /* 0x00 */		HG_SCAN_C_NAME, /* 0x01 */
	HG_SCAN_C_NAME, /* 0x02 */		HG_SCAN_C_NAME, /* 0x03 */
	HG_SCAN_C_CONTROL, /* 0x04 */		HG_SCAN_C_NAME, /* 0x05 */
	HG_SCAN_C_NAME, /* 0x06 */		HG_SCAN_C_NAME, /* 0x07 */
	HG_SCAN_C_NAME, /* 0x08 */		HG_SCAN_C_SPACE, /* 0x09 (tab \t) */
	HG_SCAN_C_SPACE, /* 0x0A (LF \n) */	HG_SCAN_C_NAME, /* 0x0B */
	HG_SCAN_C_SPACE, /* 0x0C (FF \f) */	HG_SCAN_C_SPACE, /* 0x0D (CR \r) */
	HG_SCAN_C_NAME, /* 0x0E */		HG_SCAN_C_NAME, /* 0x0F */
	HG_SCAN_C_NAME, /* 0x10 */		HG_SCAN_C_NAME, /* 0x11 */
	HG_SCAN_C_NAME, /* 0x12 */		HG_SCAN_C_NAME, /* 0x13 */
	HG_SCAN_C_NAME, /* 0x14 */		HG_SCAN_C_NAME, /* 0x15 */
	HG_SCAN_C_NAME, /* 0x16 */		HG_SCAN_C_NAME, /* 0x17 */
	HG_SCAN_C_NAME, /* 0x18 */		HG_SCAN_C_NAME, /* 0x19 */
	HG_SCAN_C_NAME, /* 0x1A */		HG_SCAN_C_NAME, /* 0x1B */
	HG_SCAN_C_NAME, /* 0x1C */		HG_SCAN_C_NAME, /* 0x1D */
	HG_SCAN_C_NAME, /* 0x1E */		HG_SCAN_C_NAME, /* 0x1F */
	HG_SCAN_C_SPACE, /* 0x20 (space) */	HG_SCAN_C_NAME, /* 0x21 */
	HG_SCAN_C_NAME, /* 0x22 */		HG_SCAN_C_NAME, /* 0x23 */
	HG_SCAN_C_NAME, /* 0x24 */		HG_SCAN_C_CONTROL, /* 0x25 % */
	HG_SCAN_C_NAME, /* 0x26 */		HG_SCAN_C_NAME, /* 0x27 */
	HG_SCAN_C_CONTROL, /* 0x28 ( */		HG_SCAN_C_CONTROL, /* 0x29 ) */
	HG_SCAN_C_NAME, /* 0x2A */		HG_SCAN_C_NAME, /* 0x2B */
	HG_SCAN_C_NAME, /* 0x2C */		HG_SCAN_C_NAME, /* 0x2D */
	HG_SCAN_C_NAME, /* 0x2E */		HG_SCAN_C_CONTROL, /* 0x2F / */
	HG_SCAN_C_NUMERAL, /* 0x30 */		HG_SCAN_C_NUMERAL, /* 0x31 */
	HG_SCAN_C_NUMERAL, /* 0x32 */		HG_SCAN_C_NUMERAL, /* 0x33 */
	HG_SCAN_C_NUMERAL, /* 0x34 */		HG_SCAN_C_NUMERAL, /* 0x35 */
	HG_SCAN_C_NUMERAL, /* 0x36 */		HG_SCAN_C_NUMERAL, /* 0x37 */
	HG_SCAN_C_NUMERAL, /* 0x38 */		HG_SCAN_C_NUMERAL, /* 0x39 */
	HG_SCAN_C_NAME, /* 0x3A */		HG_SCAN_C_NAME, /* 0x3B */
	HG_SCAN_C_CONTROL, /* 0x3C < */		HG_SCAN_C_NAME, /* 0x3D */
	HG_SCAN_C_CONTROL, /* 0x3E > */		HG_SCAN_C_NAME, /* 0x3F */
	HG_SCAN_C_NAME, /* 0x40 */		HG_SCAN_C_NAME, /* 0x41 A */
	HG_SCAN_C_NAME, /* 0x42 B */		HG_SCAN_C_NAME, /* 0x43 C */
	HG_SCAN_C_NAME, /* 0x44 D */		HG_SCAN_C_NAME, /* 0x45 E */
	HG_SCAN_C_NAME, /* 0x46 F */		HG_SCAN_C_NAME, /* 0x47 G */
	HG_SCAN_C_NAME, /* 0x48 H */		HG_SCAN_C_NAME, /* 0x49 I */
	HG_SCAN_C_NAME, /* 0x4A J */		HG_SCAN_C_NAME, /* 0x4B K */
	HG_SCAN_C_NAME, /* 0x4C L */		HG_SCAN_C_NAME, /* 0x4D M */
	HG_SCAN_C_NAME, /* 0x4E N */		HG_SCAN_C_NAME, /* 0x4F O */
	HG_SCAN_C_NAME, /* 0x50 P */		HG_SCAN_C_NAME, /* 0x51 Q */
	HG_SCAN_C_NAME, /* 0x52 R */		HG_SCAN_C_NAME, /* 0x53 S */
	HG_SCAN_C_NAME, /* 0x54 T */		HG_SCAN_C_NAME, /* 0x55 U */
	HG_SCAN_C_NAME, /* 0x56 V */		HG_SCAN_C_NAME, /* 0x57 W */
	HG_SCAN_C_NAME, /* 0x58 X */		HG_SCAN_C_NAME, /* 0x59 Y */
	HG_SCAN_C_NAME, /* 0x5A Z */		HG_SCAN_C_CONTROL, /* 0x5B [ */
	HG_SCAN_C_NAME, /* 0x5C */		HG_SCAN_C_CONTROL, /* 0x5D ] */
	HG_SCAN_C_NAME, /* 0x5E */		HG_SCAN_C_NAME, /* 0x5F */
	HG_SCAN_C_NAME, /* 0x60 */		HG_SCAN_C_NAME, /* 0x61 a */
	HG_SCAN_C_NAME, /* 0x62 b */		HG_SCAN_C_NAME, /* 0x63 c */
	HG_SCAN_C_NAME, /* 0x64 d */		HG_SCAN_C_NAME, /* 0x65 e */
	HG_SCAN_C_NAME, /* 0x66 f */		HG_SCAN_C_NAME, /* 0x67 g */
	HG_SCAN_C_NAME, /* 0x68 h */		HG_SCAN_C_NAME, /* 0x69 i */
	HG_SCAN_C_NAME, /* 0x6A j */		HG_SCAN_C_NAME, /* 0x6B k */
	HG_SCAN_C_NAME, /* 0x6C l */		HG_SCAN_C_NAME, /* 0x6D m */
	HG_SCAN_C_NAME, /* 0x6E n */		HG_SCAN_C_NAME, /* 0x6F o */
	HG_SCAN_C_NAME, /* 0x70 p */		HG_SCAN_C_NAME, /* 0x71 q */
	HG_SCAN_C_NAME, /* 0x72 r */		HG_SCAN_C_NAME, /* 0x73 s */
	HG_SCAN_C_NAME, /* 0x74 t */		HG_SCAN_C_NAME, /* 0x75 u */
	HG_SCAN_C_NAME, /* 0x76 v */		HG_SCAN_C_NAME, /* 0x77 w */
	HG_SCAN_C_NAME, /* 0x78 x */		HG_SCAN_C_NAME, /* 0x79 y */
	HG_SCAN_C_NAME, /* 0x7A z */		HG_SCAN_C_CONTROL, /* 0x7B { */
	HG_SCAN_C_NAME, /* 0x7C */		HG_SCAN_C_CONTROL, /* 0x7D } */
	HG_SCAN_C_NAME, /* 0x7E */		HG_SCAN_C_NAME, /* 0x7F */
	HG_SCAN_C_BINARY, /* 0x80 */		HG_SCAN_C_BINARY, /* 0x81 */
	HG_SCAN_C_BINARY, /* 0x82 */		HG_SCAN_C_BINARY, /* 0x83 */
	HG_SCAN_C_BINARY, /* 0x84 */		HG_SCAN_C_BINARY, /* 0x85 */
	HG_SCAN_C_BINARY, /* 0x86 */		HG_SCAN_C_BINARY, /* 0x87 */
	HG_SCAN_C_BINARY, /* 0x88 */		HG_SCAN_C_BINARY, /* 0x89 */
	HG_SCAN_C_BINARY, /* 0x8A */		HG_SCAN_C_BINARY, /* 0x8B */
	HG_SCAN_C_BINARY, /* 0x8C */		HG_SCAN_C_BINARY, /* 0x8D */
	HG_SCAN_C_BINARY, /* 0x8E */		HG_SCAN_C_BINARY, /* 0x8F */
	HG_SCAN_C_BINARY, /* 0x90 */		HG_SCAN_C_BINARY, /* 0x91 */
	HG_SCAN_C_BINARY, /* 0x92 */		HG_SCAN_C_BINARY, /* 0x93 */
	HG_SCAN_C_BINARY, /* 0x94 */		HG_SCAN_C_BINARY, /* 0x95 */
	HG_SCAN_C_BINARY, /* 0x96 */		HG_SCAN_C_BINARY, /* 0x97 */
	HG_SCAN_C_BINARY, /* 0x98 */		HG_SCAN_C_BINARY, /* 0x99 */
	HG_SCAN_C_BINARY, /* 0x9A */		HG_SCAN_C_BINARY, /* 0x9B */
	HG_SCAN_C_BINARY, /* 0x9C */		HG_SCAN_C_BINARY, /* 0x9D */
	HG_SCAN_C_BINARY, /* 0x9E */		HG_SCAN_C_BINARY, /* 0x9F */
	HG_SCAN_C_BINARY, /* 0xA0 */		HG_SCAN_C_NAME, /* 0xA1 */
	HG_SCAN_C_NAME, /* 0xA2 */		HG_SCAN_C_NAME, /* 0xA3 */
	HG_SCAN_C_NAME, /* 0xA4 */		HG_SCAN_C_NAME, /* 0xA5 */
	HG_SCAN_C_NAME, /* 0xA6 */		HG_SCAN_C_NAME, /* 0xA7 */
	HG_SCAN_C_NAME, /* 0xA8 */		HG_SCAN_C_NAME, /* 0xA9 */
	HG_SCAN_C_NAME, /* 0xAA */		HG_SCAN_C_NAME, /* 0xAB */
	HG_SCAN_C_NAME, /* 0xAC */		HG_SCAN_C_NAME, /* 0xAD */
	HG_SCAN_C_NAME, /* 0xAE */		HG_SCAN_C_NAME, /* 0xAF */
	HG_SCAN_C_NAME, /* 0xB0 */		HG_SCAN_C_NAME, /* 0xB1 */
	HG_SCAN_C_NAME, /* 0xB2 */		HG_SCAN_C_NAME, /* 0xB3 */
	HG_SCAN_C_NAME, /* 0xB4 */		HG_SCAN_C_NAME, /* 0xB5 */
	HG_SCAN_C_NAME, /* 0xB6 */		HG_SCAN_C_NAME, /* 0xB7 */
	HG_SCAN_C_NAME, /* 0xB8 */		HG_SCAN_C_NAME, /* 0xB9 */
	HG_SCAN_C_NAME, /* 0xBA */		HG_SCAN_C_NAME, /* 0xBB */
	HG_SCAN_C_NAME, /* 0xBC */		HG_SCAN_C_NAME, /* 0xBD */
	HG_SCAN_C_NAME, /* 0xBE */		HG_SCAN_C_NAME, /* 0xBF */
	HG_SCAN_C_NAME, /* 0xC0 */		HG_SCAN_C_NAME, /* 0xC1 */
	HG_SCAN_C_NAME, /* 0xC2 */		HG_SCAN_C_NAME, /* 0xC3 */
	HG_SCAN_C_NAME, /* 0xC4 */		HG_SCAN_C_NAME, /* 0xC5 */
	HG_SCAN_C_NAME, /* 0xC6 */		HG_SCAN_C_NAME, /* 0xC7 */
	HG_SCAN_C_NAME, /* 0xC8 */		HG_SCAN_C_NAME, /* 0xC9 */
	HG_SCAN_C_NAME, /* 0xCA */		HG_SCAN_C_NAME, /* 0xCB */
	HG_SCAN_C_NAME, /* 0xCC */		HG_SCAN_C_NAME, /* 0xCD */
	HG_SCAN_C_NAME, /* 0xCE */		HG_SCAN_C_NAME, /* 0xCF */
	HG_SCAN_C_NAME, /* 0xD0 */		HG_SCAN_C_NAME, /* 0xD1 */
	HG_SCAN_C_NAME, /* 0xD2 */		HG_SCAN_C_NAME, /* 0xD3 */
	HG_SCAN_C_NAME, /* 0xD4 */		HG_SCAN_C_NAME, /* 0xD5 */
	HG_SCAN_C_NAME, /* 0xD6 */		HG_SCAN_C_NAME, /* 0xD7 */
	HG_SCAN_C_NAME, /* 0xD8 */		HG_SCAN_C_NAME, /* 0xD9 */
	HG_SCAN_C_NAME, /* 0xDA */		HG_SCAN_C_NAME, /* 0xDB */
	HG_SCAN_C_NAME, /* 0xDC */		HG_SCAN_C_NAME, /* 0xDD */
	HG_SCAN_C_NAME, /* 0xDE */		HG_SCAN_C_NAME, /* 0xDF */
	HG_SCAN_C_NAME, /* 0xE0 */		HG_SCAN_C_NAME, /* 0xE1 */
	HG_SCAN_C_NAME, /* 0xE2 */		HG_SCAN_C_NAME, /* 0xE3 */
	HG_SCAN_C_NAME, /* 0xE4 */		HG_SCAN_C_NAME, /* 0xE5 */
	HG_SCAN_C_NAME, /* 0xE6 */		HG_SCAN_C_NAME, /* 0xE7 */
	HG_SCAN_C_NAME, /* 0xE8 */		HG_SCAN_C_NAME, /* 0xE9 */
	HG_SCAN_C_NAME, /* 0xEA */		HG_SCAN_C_NAME, /* 0xEB */
	HG_SCAN_C_NAME, /* 0xEC */		HG_SCAN_C_NAME, /* 0xED */
	HG_SCAN_C_NAME, /* 0xEE */		HG_SCAN_C_NAME, /* 0xEF */
	HG_SCAN_C_NAME, /* 0xF0 */		HG_SCAN_C_NAME, /* 0xF1 */
	HG_SCAN_C_NAME, /* 0xF2 */		HG_SCAN_C_NAME, /* 0xF3 */
	HG_SCAN_C_NAME, /* 0xF4 */		HG_SCAN_C_NAME, /* 0xF5 */
	HG_SCAN_C_NAME, /* 0xF6 */		HG_SCAN_C_NAME, /* 0xF7 */
	HG_SCAN_C_NAME, /* 0xF8 */		HG_SCAN_C_NAME, /* 0xF9 */
	HG_SCAN_C_NAME, /* 0xFA */		HG_SCAN_C_NAME, /* 0xFB */
	HG_SCAN_C_NAME, /* 0xFC */		HG_SCAN_C_NAME, /* 0xFD */
	HG_SCAN_C_NAME, /* 0xFE */		HG_SCAN_C_NAME, /* 0xFF */
};

/*
 * Private Functions
 */
HgValueNode *
_hg_scanner_get_object(HgVM         *vm,
		       HgFileObject *file)
{
	HgMemPool *pool;
	HgStack *estack, *ostack;
	HgDict *systemdict;
	HgValueNode *node, *retval = NULL;
	guchar c;
	gboolean need_loop = TRUE, maybe_real = FALSE, error = FALSE;
	gint token_type = 0, string_depth = 0, sign = 0;
	HgString *string = NULL;
	HgArray *array = NULL;

	pool = hg_vm_get_current_pool(vm);
	ostack = hg_vm_get_ostack(vm);
	estack = hg_vm_get_estack(vm);
	systemdict = hg_vm_get_dict_systemdict(vm);

	/* skip the white spaces first */
	_hg_scanner_skip_spaces(file, c);

	if (c == 0 && hg_file_object_is_eof(file))
		return NULL;

	hg_file_object_ungetc(file, c);
	while (need_loop) {
		c = hg_file_object_getc(file);
		switch (__hg_scanner_token[c]) {
		    case HG_SCAN_C_NULL:
			    need_loop = FALSE;
			    break;
		    case HG_SCAN_C_CONTROL:
			    if (token_type == HG_SCAN_TOKEN_LITERAL && c == '/') {
				    if (hg_string_length(string) > 0) {
					    hg_file_object_ungetc(file, c);
					    need_loop = FALSE;
				    } else {
					    token_type = HG_SCAN_TOKEN_EVAL_NAME;
				    }
				    break;
			    } else if (token_type == HG_SCAN_TOKEN_STRING) {
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
					    _hg_scanner_set_error(vm,
								  __hg_operator_list[HG_op_token],
								  VM_e_VMerror);
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
					token_type = HG_SCAN_TOKEN_LITERAL;
					string = hg_string_new(pool, -1);
					break;
				case '(':
					token_type = HG_SCAN_TOKEN_STRING;
					string = hg_string_new(pool, -1);
					string_depth++;
					break;
				case ')':
					_hg_scanner_set_error(vm,
							      __hg_operator_list[HG_op_token],
							      VM_e_syntaxerror);
					need_loop = FALSE;
					break;
				case '<':
					c = hg_file_object_getc(file);
					if (c == '<') {
						/* dict mark */
						token_type = HG_SCAN_TOKEN_MARK;
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
						token_type = HG_SCAN_TOKEN_DICT;
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
					token_type = HG_SCAN_TOKEN_MARK;
					need_loop = FALSE;
					break;
				case ']':
					token_type = HG_SCAN_TOKEN_ARRAY;
					need_loop = FALSE;
					break;
				case '{':
					array = hg_array_new(pool, -1);
					if (array == NULL) {
						_hg_scanner_set_error(vm,
								      __hg_operator_list[HG_op_token],
								      VM_e_VMerror);
						return NULL;
					}
					while (1) {
						node = _hg_scanner_get_object(vm, file);
						if (hg_vm_has_error(vm)) {
							return NULL;
						}
						if (node == NULL) {
							_hg_scanner_set_error(vm,
									      __hg_operator_list[HG_op_token],
									      VM_e_syntaxerror);
							need_loop = FALSE;
							break;
						}
						if (HG_IS_VALUE_NULL (node) && HG_VALUE_GET_NULL (node) != NULL) {
							/* HACK: assume that this is a mark for proc end. */
							token_type = HG_SCAN_TOKEN_PROC;
							hg_array_fix_array_size(array);
							hg_mem_free(node);
							break;
						}
						if (!hg_array_append(array, node)) {
							_hg_scanner_set_error(vm,
									      __hg_operator_list[HG_op_token],
									      VM_e_VMerror);
							return NULL;
						}
					}
					need_loop = FALSE;
					break;
				case '}':
					token_type = HG_SCAN_TOKEN_PROC_END;
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
					_hg_scanner_skip_spaces(file, c);
					hg_file_object_ungetc(file, c);
					break;
				default:
					g_warning("[BUG] unknown control object %c.", c);
					break;
			    }
			    break;
		    case HG_SCAN_C_NAME:
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
					case '\r':
						if (token_type == HG_SCAN_TOKEN_STRING) {
							c = hg_file_object_getc(file);
							if (c != '\n') {
								hg_file_object_ungetc(file, c);
								continue;
							}
						} else {
							hg_file_object_ungetc(file, c);
							c = '\\';
						}
						break;
					case '\n':
						if (token_type == HG_SCAN_TOKEN_STRING) {
							/* ignore it */
							continue;
						} else {
							hg_file_object_ungetc(file, c);
							c = '\\';
						}
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
						_hg_scanner_set_error(vm,
								      __hg_operator_list[HG_op_token],
								      VM_e_VMerror);
						return NULL;
					}
					if (_hg_scanner_is_sign(c)) {
						sign = 1;
						if (c == '-')
							sign = -sign;
					} else if (c == '.') {
						maybe_real = TRUE;
					}
					token_type = HG_SCAN_TOKEN_NAME;
					if (_hg_scanner_parse_number(vm, file, token_type,
								     10, sign, !maybe_real,
								     string, &retval, &error)) {
						if (error)
							return NULL;
						token_type = HG_SCAN_TOKEN_NUMERIC;
						need_loop = FALSE;
					}
					break;
				case HG_SCAN_TOKEN_NAME:
				case HG_SCAN_TOKEN_LITERAL:
				case HG_SCAN_TOKEN_EVAL_NAME:
				case HG_SCAN_TOKEN_STRING:
					if (!hg_string_append_c(string, c)) {
						_hg_scanner_set_error(vm,
								      __hg_operator_list[HG_op_token],
								      VM_e_VMerror);
						return NULL;
					}
					break;
				default:
					g_warning("[BUG] it may be unlikely to appear HG_SCAN_C_NAME in token type %d\n", token_type);
					break;
			    }
			    break;
		    case HG_SCAN_C_SPACE:
			    if (token_type == HG_SCAN_TOKEN_STRING) {
				    if (!hg_string_append_c(string, c)) {
					    _hg_scanner_set_error(vm,
								  __hg_operator_list[HG_op_token],
								  VM_e_VMerror);
					    return NULL;
				    }
			    } else {
				    need_loop = FALSE;
			    }
			    break;
		    case HG_SCAN_C_NUMERAL:
			    switch (token_type) {
				case 0:
					/* it might be a numeral, but possibly name. */
					string = hg_string_new(pool, -1);
					hg_file_object_ungetc(file, c);
					token_type = HG_SCAN_TOKEN_NAME;
					if (_hg_scanner_parse_number(vm, file, token_type,
								     10, sign, !maybe_real,
								     string, &retval, &error)) {
						if (error)
							return NULL;
						token_type = HG_SCAN_TOKEN_NUMERIC;
						need_loop = FALSE;
					}
					break;
				case HG_SCAN_TOKEN_LITERAL:
				case HG_SCAN_TOKEN_EVAL_NAME:
				case HG_SCAN_TOKEN_STRING:
				case HG_SCAN_TOKEN_NAME:
					if (!hg_string_append_c(string, c)) {
						_hg_scanner_set_error(vm,
								      __hg_operator_list[HG_op_token],
								      VM_e_VMerror);
						return NULL;
					}
					break;
				default:
					break;
			    }
			    break;
		    case HG_SCAN_C_BINARY:
			    break;
		    default:
			    g_warning("Unknown character %c(%d)[%d]", c, c, __hg_scanner_token[c]);
			    break;
		}
	}
	switch (token_type) {
	    case HG_SCAN_TOKEN_LITERAL:
		    retval = hg_vm_get_name_node(vm, hg_string_get_string(string));
		    hg_object_inexecutable((HgObject *)retval);
		    hg_mem_free(string);
		    break;
	    case HG_SCAN_TOKEN_NAME:
		    retval = hg_vm_get_name_node(vm, hg_string_get_string(string));
		    hg_object_executable((HgObject *)retval);
		    hg_mem_free(string);
		    break;
	    case HG_SCAN_TOKEN_EVAL_NAME:
		    node = hg_vm_get_name_node(vm, hg_string_get_string(string));
		    retval = hg_vm_lookup(vm, node);
		    if (retval == NULL) {
			    _hg_scanner_set_error(vm,
						  __hg_operator_list[HG_op_token],
						  VM_e_undefined);
		    }
		    hg_mem_free(string);
		    break;
	    case HG_SCAN_TOKEN_NUMERIC:
		    hg_mem_free(string);
		    break;
	    case HG_SCAN_TOKEN_MARK:
		    retval = hg_dict_lookup_with_string(systemdict, "mark");
		    break;
	    case HG_SCAN_TOKEN_ARRAY:
		    retval = hg_dict_lookup_with_string(systemdict, "%arraytomark");
		    break;
	    case HG_SCAN_TOKEN_DICT:
		    retval = hg_dict_lookup_with_string(systemdict, "%dicttomark");
		    break;
	    case HG_SCAN_TOKEN_PROC:
		    HG_VALUE_MAKE_ARRAY (retval, array);
		    hg_object_executable((HgObject *)retval);
		    break;
	    case HG_SCAN_TOKEN_PROC_END:
		    HG_VALUE_MAKE_NULL (pool, retval, GINT_TO_POINTER (1));
		    break;
	    case HG_SCAN_TOKEN_STRING:
		    if (string_depth == 0) {
			    hg_string_fix_string_size(string);
			    HG_VALUE_MAKE_STRING (retval, string);
		    }
		    break;
	    default:
		    g_warning("FIXME: unknown token type %d\n", token_type);
		    break;
	}

	/* This is to be not broken the objects in systemdict by the malicious users
	 * after the initialization finished.
	 * makes the objects in read-only anyway.
	 */
	node = hg_dict_lookup_with_string(hg_vm_get_dict_statusdict(vm), "%initialized");
	if ((node == NULL ||
	     HG_VALUE_GET_BOOLEAN (node) == FALSE) &&
	    retval != NULL && !HG_IS_VALUE_STRING (retval)) {
		hg_object_unwritable((HgObject *)retval);
	}

	return retval;
}

static gboolean
_hg_scanner_parse_number(HgVM          *vm,
			 HgFileObject  *file,
			 gint           token_type,
			 gint           radix,
			 gint           sign,
			 gboolean       is_integer,
			 HgString      *string,
			 HgValueNode  **node,
			 gboolean      *error)
{
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	guchar c;
	gboolean need_loop = TRUE, is_valid = FALSE, is_power = FALSE;
	gboolean has_radix = (radix == 10 ? FALSE : TRUE);
	HgIntArray *intarray, *floatarray = NULL;
	gint32 i = 0, power = 0, di = 0;
	gint power_sign = 0;
	static const gchar *const radix_index = "0123456789abcdefghijklmnopqrstuvwxyz";

	intarray = hg_int_array_new(pool, -1);
	if (!is_integer) {
		/* value may be less than 1 */
		hg_int_array_append(intarray, 0);
		floatarray = hg_int_array_new(pool, -1);
	}
	while (need_loop) {
		c = hg_file_object_getc(file);
		switch (__hg_scanner_token[c]) {
		    case HG_SCAN_C_CONTROL:
			    hg_file_object_ungetc(file, c);
		    case HG_SCAN_C_NULL:
		    case HG_SCAN_C_SPACE:
			    need_loop = FALSE;
			    break;
		    case HG_SCAN_C_NAME:
			    is_valid = TRUE;
			    if (c != '\\') {
				    switch (token_type) {
					case HG_SCAN_TOKEN_NAME:
						if (c == '.') {
							if (!is_integer || is_power || has_radix) {
								is_valid = FALSE;
								break;
							}
							di = i;
							floatarray = hg_int_array_new(pool, -1);
							is_integer = FALSE;
							i = 0;
						} else if ((c == 'e' || c == 'E') && !has_radix) {
							if (is_power) {
								is_valid = FALSE;
								break;
							}
							is_power = TRUE;
							if (!hg_string_append_c(string, c)) {
								_hg_scanner_set_error(vm,
										      __hg_operator_list[HG_op_token],
										      VM_e_VMerror);
								*error = TRUE;
								return FALSE;
							}
							c = hg_file_object_getc(file);
							if (_hg_scanner_is_sign(c)) {
								power_sign = 1;
								if (c == '-')
									power_sign = -power_sign;
							} else if (c < '0' || c > '9') {
								is_valid = FALSE;
								break;
							} else {
								hg_file_object_ungetc(file, c);
							}
						} else if (c == '#') {
							if (is_power || !is_integer || has_radix ||
							    hg_int_array_length(intarray) > 0 ||
							    i > 36) {
								is_valid = FALSE;
								break;
							}
							has_radix = TRUE;
							radix = i;
							i = 0;
						} else if ((c >= 'a' && c <= 'z') ||
							   (c >= 'A' && c <= 'Z')) {
							char *p;
							gint index;

							if (is_power || !is_integer) {
								is_valid = FALSE;
								break;
							}
							p = strchr(radix_index, tolower(c));
							index = p - radix_index;
							if (index >= radix) {
								is_valid = FALSE;
								break;
							}
							if ((i > 0 && (i * radix) < 0) ||
							    (i < 0 && (i * radix) > 0)) {
								/* the overflow may happens but we can't
								 * fallback to real.
								 */
								_hg_scanner_set_error(vm,
										      __hg_operator_list[HG_op_token],
										      VM_e_limitcheck);
								*error = TRUE;
								return FALSE;
							}
							i *= radix;
							i += index;
						} else {
							is_valid = FALSE;
							break;
						}

						if (!hg_string_append_c(string, c)) {
							_hg_scanner_set_error(vm,
									      __hg_operator_list[HG_op_token],
									      VM_e_VMerror);
							*error = TRUE;
							return FALSE;
						}
						break;
					case HG_SCAN_TOKEN_LITERAL:
					case HG_SCAN_TOKEN_EVAL_NAME:
					case HG_SCAN_TOKEN_STRING:
						/* it may be unlikely */
						is_valid = FALSE;
						break;
					default:
						g_warning("[BUG] it may be unlikely to appear HG_SCAN_C_NAME in token type %d\n", token_type);
						is_valid = FALSE;
						break;
				    }
				    if (is_valid)
					    break;
			    }
			    goto non_numeral_handler;
		    case HG_SCAN_C_NUMERAL:
			    is_valid = TRUE;
			    switch (token_type) {
				case HG_SCAN_TOKEN_LITERAL:
				case HG_SCAN_TOKEN_EVAL_NAME:
				case HG_SCAN_TOKEN_STRING:
					/* it may be unlikely */
					goto non_numeral_handler;
				case HG_SCAN_TOKEN_NAME:
					if (is_power) {
						power *= 10;
						power += c - '0';
					} else {
						if (has_radix &&
						    ((i > 0 && (i * radix) < 0) ||
						     (i < 0 && (i * radix) > 0))) {
							/* the overflow may happens but we can't
							 * fallback to real.
							 */
							_hg_scanner_set_error(vm,
									      __hg_operator_list[HG_op_token],
									      VM_e_limitcheck);
							*error = TRUE;
							return FALSE;
						} else if (!has_radix &&
							   i > 100000000) {
							/* it could has 10 digits in 32bit integer though,
							 * it may be complicated to convert it to real,
							 * so that the overflow will happens within 10 digits.
							 */
							if (floatarray) {
								hg_int_array_append(floatarray, i);
								i = 0;
							} else {
								hg_int_array_append(intarray, i);
								i = 0;
							}
						}
						i *= radix;
						i += c - '0';
					}
					if (!hg_string_append_c(string, c)) {
						_hg_scanner_set_error(vm,
								      __hg_operator_list[HG_op_token],
								      VM_e_VMerror);
						*error = TRUE;
						return FALSE;
					}
					break;
				default:
					g_warning("[BUG] it may be unlikely to appear HG_SCAN_C_NUMERAL in token type %d\n", token_type);
					goto non_numeral_handler;
			    }
			    break;
		    case HG_SCAN_C_BINARY:
		    default:
		    non_numeral_handler:
			    /* postpone dealing with this */
			    hg_file_object_ungetc(file, c);
			    need_loop = FALSE;
			    is_valid = FALSE;
			    break;
		}
	}
	if (is_valid) {
		if (hg_int_array_length(intarray) == 0 && floatarray == NULL && !is_power) {
			/* integer */
			if (sign < 0)
				i = -i;
			HG_VALUE_MAKE_INTEGER (pool, *node, i);
		} else {
			guint j, length, digits = (guint)log10(i) + 1, intdigits = 0;
			gdouble d = 0.0L, dd = 0.0L;

			if (di > 0)
				intdigits = (guint)log10(di) + 1;
			if (has_radix) {
				/* this is unlikely */
				g_warning("[BUG] numeric with radix is going to be the real.");
				return FALSE;
			}
			if (floatarray) {
				length = hg_int_array_length(floatarray);
				for (j = 0; j < length; j++) {
					dd += ((gdouble)hg_int_array_index(floatarray, j) * exp10((gdouble)-((j + 1) * 9)));
				}
				dd += (i * exp10(-(gdouble)digits));
				digits = 0;
				i = 0;
			}
			length = hg_int_array_length(intarray);
			for (j = 0; j < length; j++) {
				d += ((gdouble)hg_int_array_index(intarray, j) * exp10((gdouble)((length - j - 1) * 9 + digits + intdigits)));
				if (isinf(d) != 0 || isnan(d) != 0) {
					_hg_scanner_set_error(vm,
							      __hg_operator_list[HG_op_token],
							      VM_e_limitcheck);
					*error = TRUE;
					return FALSE;
				}
			}
			if (di != 0) {
				d += (gdouble)di;
			} else {
				d += (gdouble)i;
			}
			d += dd;
			if (is_power) {
				if (power_sign < 0)
					power = -power;
				dd = exp10((gdouble)power);
				d *= dd;
			}
			if (sign < 0)
				d = -d;
			if (is_integer && d <= G_MAXINT32 && d >= G_MININT32)
				HG_VALUE_MAKE_INTEGER (pool, *node, (gint32)d);
			else
				HG_VALUE_MAKE_REAL (pool, *node, d);
		}
	}

	if (intarray)
		hg_int_array_free(intarray);
	if (floatarray)
		hg_int_array_free(floatarray);

	return is_valid;
}

/*
 * Public Functions
 */
HgValueNode *
hg_scanner_get_object(HgVM         *vm,
		      HgFileObject *file)
{
	g_return_val_if_fail (vm != NULL, NULL);
	g_return_val_if_fail (file != NULL, NULL);

	return _hg_scanner_get_object(vm, file);
}
