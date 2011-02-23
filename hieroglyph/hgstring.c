/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
/* GLib is still needed for GString */
#include <glib.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgquark.h"
#include "hgstring.h"

#include "hgstring.proto.h"

#define HG_STRING_ALLOC_SIZE	256
#define HG_STRING_MAX_SIZE	65535 /* defined as PostScript spec */


HG_DEFINE_VTABLE_WITH (string, NULL, NULL, NULL);

/*< private >*/
static hg_usize_t
_hg_object_string_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_string_t));
}

static hg_uint_t
_hg_object_string_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_string_initialize(hg_object_t *object,
			     va_list      args)
{
	hg_string_t *str = (hg_string_t *)object;
	const hg_char_t *string = NULL;
	hg_usize_t length;

	str->qstring = Qnil;
	str->length = 0;

	string = va_arg(args, const hg_char_t *);
	str->offset = va_arg(args, hg_usize_t);
	hg_return_val_if_fail (str->offset >= 0, FALSE, HG_e_invalidaccess);

	length = va_arg(args, hg_usize_t);

	/* not allocating any containers here */
	str->allocated_size = length + 1;
	str->is_fixed_size = FALSE;

	if (string) {
		return hg_string_append(str, string, length);
	}

	return TRUE;
}

static hg_quark_t
_hg_object_string_copy(hg_object_t             *object,
		       hg_quark_iterate_func_t  func,
		       hg_pointer_t             user_data,
		       hg_pointer_t            *ret)
{
	hg_string_t *s = (hg_string_t *)object;
	hg_quark_t retval = Qnil;
	hg_char_t *cstr = hg_string_get_cstr(s);

	hg_return_val_if_fail (object->type == HG_TYPE_STRING, Qnil, HG_e_typecheck);

	retval = HG_QSTRING_LEN (s->o.mem, cstr, hg_string_length(s));
	g_free(cstr);

	return retval;
}

static hg_char_t *
_hg_object_string_to_cstr(hg_object_t             *object,
			  hg_quark_iterate_func_t  func,
			  hg_pointer_t             user_data)
{
	GString *retval = g_string_new(NULL);
	hg_string_t *s = (hg_string_t *)object;
	hg_char_t *cstr = hg_string_get_cstr(s);
	hg_char_t buffer[8];
	hg_usize_t i;

	hg_return_val_if_fail (object->type == HG_TYPE_STRING, NULL, HG_e_typecheck);

	g_string_append_c(retval, '(');
	for (i = 0; i < hg_string_maxlength(s); i++) {
		if (cstr == NULL) {
			g_string_append(retval, "\\000");
		} else if (isprint(cstr[i])) {
			g_string_append_c(retval, cstr[i]);
		} else {
			switch (cstr[i]) {
			    case '\n':
				    g_string_append(retval, "\\n");
				    break;
			    case '\r':
				    g_string_append(retval, "\\r");
				    break;
			    case '\t':
				    g_string_append(retval, "\\t");
				    break;
			    case '\b':
				    g_string_append(retval, "\\b");
				    break;
			    case '\f':
				    g_string_append(retval, "\\f");
				    break;
			    default:
				    snprintf(buffer, 8, "%04o", cstr[i] & 0xff);
				    g_string_append_c(retval, '\\');
				    g_string_append(retval, buffer);
				    break;
			}
		}
	}
	g_free(cstr);
	g_string_append_c(retval, ')');

	return g_string_free(retval, FALSE);
}

static hg_bool_t
_hg_object_string_gc_mark(hg_object_t *object)
{
	hg_string_t *string = (hg_string_t *)object;

	hg_return_val_if_fail (object->type == HG_TYPE_STRING, FALSE, HG_e_typecheck);

	return hg_mem_gc_mark(string->o.mem, string->qstring);
}

static hg_bool_t
_hg_object_string_compare(hg_object_t             *o1,
			  hg_object_t             *o2,
			  hg_quark_compare_func_t  func,
			  hg_pointer_t             user_data)
{
	return hg_string_compare((hg_string_t *)o1, (hg_string_t *)o2);
}

static hg_bool_t
_hg_string_maybe_expand(hg_string_t *string)
{
	if (string->is_fixed_size ||
	    string->offset != 0)
		return TRUE;

	hg_return_val_if_fail (string->length < string->allocated_size, FALSE, HG_e_rangecheck);

	if (string->qstring == Qnil) {
		string->qstring = hg_mem_alloc_with_flags(string->o.mem,
							  string->length + 1,
							  HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE,
							  NULL);
	} else {
		hg_quark_t q = hg_mem_realloc(string->o.mem,
					      string->qstring,
					      string->length + 1,
					      NULL);
		if (q == Qnil)
			return FALSE;
		string->qstring = q;
	}

	return TRUE;
}

/*< public >*/
/**
 * hg_string_new:
 * @mem:
 * @requisition_size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_string_new(hg_mem_t     *mem,
	      hg_usize_t    requisition_size,
	      hg_pointer_t *ret)
{
	hg_string_t *s = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (requisition_size <= HG_STRING_MAX_SIZE, Qnil, HG_e_limitcheck);

	retval = hg_object_new(mem, (hg_pointer_t *)&s, HG_TYPE_STRING, 0, NULL, 0, requisition_size);
	if (retval != Qnil) {
		if (ret)
			*ret = s;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_string_new_with_value:
 * @mem:
 * @string:
 * @length:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_string_new_with_value(hg_mem_t        *mem,
			 const hg_char_t *string,
			 hg_size_t        length,
			 hg_pointer_t    *ret)
{
	hg_string_t *s = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (string != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (length <= HG_STRING_MAX_SIZE, Qnil, HG_e_limitcheck);

	if (length < 0)
		length = strlen(string);

	retval = hg_object_new(mem, (hg_pointer_t *)&s, HG_TYPE_STRING, 0, string, 0, length);
	if (retval != Qnil) {
		if (ret)
			*ret = s;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_string_free:
 * @string:
 * @free_segment:
 *
 * FIXME
 */
void
hg_string_free(hg_string_t *string,
	       hg_bool_t    free_segment)
{
	if (string == NULL)
		return;

	hg_return_if_fail (string->o.type == HG_TYPE_STRING, HG_e_typecheck);

	hg_mem_unref(string->o.mem, string->o.self);

	if (free_segment)
		hg_mem_free(string->o.mem, string->qstring);
	hg_object_free(string->o.mem, string->o.self);
}

/**
 * hg_string_length:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_string_length(const hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, -1, HG_e_typecheck);

	return string->length;
}

/**
 * hg_string_maxlength:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_string_maxlength(const hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, -1, HG_e_typecheck);

	return string->allocated_size - 1;
}

/**
 * hg_string_clear:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_clear(hg_string_t *string)
{
	hg_char_t *s;

	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);

	if (string->qstring == Qnil ||
	    string->length == 0)
		return TRUE;

	hg_return_val_if_lock_fail (s, string->o.mem, string->qstring, FALSE);

	string->length = 0;
	s[0] = 0;
	hg_mem_unlock_object(string->o.mem, string->qstring);

	return TRUE;
}

/**
 * hg_string_append_c:
 * @string:
 * @c:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_append_c(hg_string_t *string,
		   hg_char_t    c)
{
	hg_char_t *s;
	hg_bool_t retval = FALSE;
	hg_usize_t old_length;

	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);

	if (string->length < hg_string_maxlength(string)) {
		old_length = string->length;
		string->length++;
		if (!_hg_string_maybe_expand(string)) {
			string->length = old_length;
			return FALSE;
		}
		hg_return_val_if_lock_fail (s,
					    string->o.mem,
					    string->qstring,
					    FALSE);

		s[old_length] = c;
		hg_mem_unlock_object(string->o.mem, string->qstring);
		retval = TRUE;
	} else {
		if (!string->is_fixed_size) {
			hg_quark_t new_qstr = Qnil;

			hg_return_val_if_fail (hg_string_maxlength(string) <= HG_STRING_MAX_SIZE, FALSE, HG_e_limitcheck);

			new_qstr = hg_mem_realloc(string->o.mem,
						  string->qstring,
						  MIN (string->allocated_size + HG_STRING_ALLOC_SIZE + 1, HG_STRING_MAX_SIZE + 1),
						  (hg_pointer_t *)&s);
			if (new_qstr == Qnil)
				return FALSE;

			string->qstring = new_qstr;
			string->allocated_size = MIN (string->allocated_size + HG_STRING_ALLOC_SIZE, HG_STRING_MAX_SIZE);
			s[string->length++] = c;
			hg_mem_unlock_object(string->o.mem, new_qstr);
			retval = TRUE;
		}
	}

	return retval;
}

/**
 * hg_string_append:
 * @string:
 * @str:
 * @length:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_append(hg_string_t     *string,
		 const hg_char_t *str,
		 hg_size_t        length)
{
	hg_char_t *s;
	hg_bool_t retval = FALSE;
	hg_size_t i, old_length;

	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (str != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);

	if (length < 0)
		length = strlen(str);
	if (string->length == 0 ||
	    (string->length + length) < hg_string_maxlength(string)) {
		old_length = string->length;
		string->length += length;
		if (!_hg_string_maybe_expand(string)) {
			string->length = old_length;
			return FALSE;
		}

		hg_return_val_if_lock_fail (s,
					    string->o.mem,
					    string->qstring,
					    FALSE);

		memcpy(&s[old_length], str, length);

		hg_mem_unlock_object(string->o.mem, string->qstring);
		retval = TRUE;
	} else {
		if (!string->is_fixed_size) {
			hg_quark_t new_qstr = Qnil;

			hg_return_val_if_fail ((hg_string_maxlength(string) + length) <= HG_STRING_MAX_SIZE, FALSE, HG_e_limitcheck);

			new_qstr = hg_mem_realloc(string->o.mem,
						  string->qstring,
						  MIN (string->allocated_size + length + HG_STRING_ALLOC_SIZE + 1, HG_STRING_MAX_SIZE + 1),
						  (hg_pointer_t *)&s);
			if (new_qstr == Qnil)
				return FALSE;

			string->qstring = new_qstr;
			string->allocated_size = MIN (string->allocated_size + length + HG_STRING_ALLOC_SIZE, HG_STRING_MAX_SIZE);
			for (i = 0; i < length; i++)
				s[string->length++] = str[i];
			hg_mem_unlock_object(string->o.mem, new_qstr);
			retval = TRUE;
		}
	}

	return retval;
}

/**
 * hg_string_overwrite_c:
 * @string:
 * @c:
 * @index:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_overwrite_c(hg_string_t *string,
		      hg_char_t    c,
		      hg_uint_t    index)
{
	hg_char_t *s;
	hg_usize_t old_length;

	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (index < hg_string_maxlength(string), FALSE, HG_e_rangecheck);

	if (string->length <= index) {
		old_length = string->length;
		string->length = index + 1;
		if (!_hg_string_maybe_expand(string)) {
			string->length = old_length;
			return FALSE;
		}
	}

	hg_return_val_if_lock_fail (s,
				    string->o.mem,
				    string->qstring,
				    FALSE);

	s[index] = c;

	hg_mem_unlock_object(string->o.mem, string->qstring);

	return TRUE;
}

/**
 * hg_string_erase:
 * @string:
 * @pos:
 * @length:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_erase(hg_string_t *string,
		hg_size_t    pos,
		hg_size_t    length)
{
	hg_char_t *s;
	hg_size_t i, j;

	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (pos > 0, FALSE, HG_e_rangecheck);

	if (string->qstring == Qnil) {
		/* no data yet */
		return TRUE;
	}
	if (length < 0)
		length = string->length - pos;

	hg_return_val_if_fail ((pos + length) <= string->length, FALSE, HG_e_rangecheck);
	hg_return_val_if_lock_fail (s,
				    string->o.mem,
				    string->qstring,
				    FALSE);

	for (i = pos, j = (pos + length); j < string->length; i++, j++) {
		s[i] = s[j];
	}
	string->length -= length;
	hg_mem_unlock_object(string->o.mem, string->qstring);

	return TRUE;
}

/**
 * hg_string_concat:
 * @string1:
 * @string2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_concat(hg_string_t *string1,
		 hg_string_t *string2)
{
	const hg_char_t *s;
	hg_bool_t retval;

	hg_return_val_if_fail (string1 != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string1->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string2 != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string2->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_lock_fail (s,
				    string2->o.mem,
				    string2->qstring,
				    FALSE);

	retval = hg_string_append(string1, s, string2->length);
	hg_mem_unlock_object(string2->o.mem, string2->qstring);

	return retval;
}

/**
 * hg_string_index:
 * @string:
 * @index:
 *
 * FIXME
 *
 * Returns:
 */
hg_char_t
hg_string_index(hg_string_t *string,
		hg_uint_t    index)
{
	const hg_char_t *s;
	hg_char_t retval;

	hg_return_val_if_fail (string != NULL, 0, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, 0, HG_e_typecheck);
	hg_return_val_if_fail (index < string->length, 0, HG_e_rangecheck);

	if (string->qstring == Qnil) {
		/* no data yet */
		return 0;
	}
	hg_return_val_if_lock_fail (s,
				    string->o.mem,
				    string->qstring,
				    0);

	retval = s[index];
	hg_mem_unlock_object(string->o.mem, string->qstring);

	return retval;
}

/**
 * hg_string_get_cstr:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
hg_char_t *
hg_string_get_cstr(hg_string_t *string)
{
	hg_char_t *retval, *cstr;

	hg_return_val_if_fail (string != NULL, NULL, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, NULL, HG_e_typecheck);

	if (string->qstring == Qnil)
		return NULL;

	hg_return_val_if_lock_fail (cstr,
				    string->o.mem,
				    string->qstring,
				    NULL);

	retval = g_new0(hg_char_t, string->length + 1);
	memcpy(retval, &cstr[string->offset], string->length);
	retval[string->length] = 0;

	hg_mem_unlock_object(string->o.mem, string->qstring);

	return retval;
}

/**
 * hg_string_fix_string_size:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_fix_string_size(hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);

	if (!string->is_fixed_size) {
		hg_quark_t new_qstr;

		new_qstr = hg_mem_realloc(string->o.mem, string->qstring, string->length + 1, NULL);
		if (new_qstr == Qnil)
			return FALSE;
		string->qstring = new_qstr;
		string->is_fixed_size = TRUE;
		string->allocated_size = string->length + 1;
	}

	return TRUE;
}

/**
 * hg_string_compare:
 * @a:
 * @b:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_compare(hg_string_t *a,
		  hg_string_t *b)
{
	hg_return_val_if_fail (b != NULL, FALSE, HG_e_typecheck);

	return hg_string_ncompare(a, b, b->length);
}

/**
 * hg_string_ncompare:
 * @a:
 * @b:
 * @length:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_ncompare(hg_string_t *a,
		   hg_string_t *b,
		   hg_uint_t    length)
{
	hg_char_t *sb;
	hg_bool_t retval;

	hg_return_val_if_fail (a != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (a->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (b != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (b->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);

	if (a->qstring == Qnil && b->qstring == Qnil &&
	    a->length == length)
		return TRUE;

	sb = hg_string_get_cstr(b);

	retval = hg_string_ncompare_with_cstr(a, sb, length);

	g_free(sb);

	return retval;
}

/**
 * hg_string_ncompare_with_cstr:
 * @a:
 * @b:
 * @length:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_ncompare_with_cstr(hg_string_t     *a,
			     const hg_char_t *b,
			     hg_size_t        length)
{
	hg_char_t *sa;
	hg_bool_t retval;

	hg_return_val_if_fail (a != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (a->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (b != NULL, FALSE, HG_e_VMerror);

	if (length < 0)
		length = strlen(b);
	if (length != a->length)
		return FALSE;

	sa = hg_string_get_cstr(a);

	retval = memcmp(sa, b, length) == 0;

	g_free(sa);

	return retval;
}

/**
 * hg_string_append_printf:
 * @string:
 * @format:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_append_printf(hg_string_t     *string,
			const hg_char_t *format,
			...)
{
	va_list ap;
	hg_char_t *ret;
	hg_bool_t retval;

	hg_return_val_if_fail (string != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (format != NULL, FALSE, HG_e_VMerror);

	va_start(ap, format);

	ret = g_strdup_vprintf(format, ap);
	retval = hg_string_append(string, ret, -1);
	g_free(ret);

	va_end(ap);

	return retval;
}

/**
 * hg_string_make_substring:
 * @string:
 * @start_index:
 * @end_index:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_string_make_substring(hg_string_t  *string,
			 hg_size_t     start_index,
			 hg_size_t     end_index,
			 hg_pointer_t *ret)
{
	hg_string_t *s;
	hg_quark_t retval;

	hg_return_val_if_fail (string != NULL, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (string->o.type == HG_TYPE_STRING, Qnil, HG_e_typecheck);

	retval = hg_string_new(string->o.mem, 0, (hg_pointer_t *)&s);
	if (retval != Qnil) {
		if (end_index > HG_STRING_MAX_SIZE) {
			/* make an empty string */
		} else {
			if (!hg_string_copy_as_substring(string, s,
							 start_index,
							 end_index)) {
				hg_object_free(string->o.mem, retval);
				retval = Qnil;
			}
			if (ret)
				*ret = s;
			else
				hg_mem_unlock_object(string->o.mem, retval);
		}
	}

	return retval;
}

/**
 * hg_string_copy_as_substring:
 * @src:
 * @dest:
 * @start_index:
 * @end_index:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_string_copy_as_substring(hg_string_t *src,
			    hg_string_t *dest,
			    hg_size_t    start_index,
			    hg_size_t    end_index)
{
	hg_return_val_if_fail (src != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (src->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (dest != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (dest->o.type == HG_TYPE_STRING, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (start_index < hg_string_maxlength(src) || (end_index - start_index + 1) == 0, FALSE, HG_e_rangecheck);
	hg_return_val_if_fail (end_index < hg_string_maxlength(src), FALSE, HG_e_rangecheck);
	hg_return_val_if_fail ((end_index - start_index + 1) >= 0, FALSE, HG_e_rangecheck);

	/* destroy the unnecessary destination's container */
	hg_mem_free(dest->o.mem, dest->qstring);
	/* make a substring */
	dest->qstring = src->qstring;
	dest->length = end_index - start_index + 1;
	dest->allocated_size = dest->length + 1;
	dest->is_fixed_size = TRUE;
	dest->offset = src->offset + start_index;

	return TRUE;
}
