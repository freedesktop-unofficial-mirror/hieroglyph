/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.c
 * Copyright (C) 2010 Akira TAGOH
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

#include <string.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgquark.h"
#include "hgstring.h"

#define HG_STRING_ALLOC_SIZE	256
#define HG_STRING_MAX_SIZE	65535 /* defined as PostScript spec */


HG_DEFINE_VTABLE (string)

/*< private >*/
static gsize
_hg_object_string_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_string_t));
}

static gboolean
_hg_object_string_initialize(hg_object_t *object,
			     va_list      args)
{
	hg_string_t *str = (hg_string_t *)object;
	const gchar *string = NULL;
	gchar *p = NULL;
	gsize length;

	str->qstring = Qnil;
	str->length = 0;

	string = va_arg(args, const gchar *);
	str->offset = va_arg(args, gsize);
	hg_return_val_if_fail (str->offset >= 0, FALSE);

	length = va_arg(args, gsize);
	if (length < 0)
		length = strlen(string);

	str->qstring = hg_mem_alloc(object->mem, length, (gpointer *)&p);
	str->allocated_size = length;
	str->is_fixed_size = FALSE;
	hg_return_val_if_fail (str->qstring != Qnil, FALSE);

	if (string) {
		memcpy(p, string + str->offset, length);
		str->length = length;
	}
	/* unlock the memory spaces for strings */
	hg_mem_unlock_object(object->mem, str->qstring);

	return TRUE;
}

static void
_hg_object_string_free(hg_object_t *object)
{
}

/*< public >*/
/**
 * hg_string_new:
 * @mem:
 * @ret:
 * @requisition_size:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_string_new(hg_mem_t *mem,
	      gpointer *ret,
	      gsize     requisition_size)
{
	hg_string_t *s = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (requisition_size > 0 && requisition_size <= HG_STRING_MAX_SIZE, Qnil);

	retval = hg_object_new(mem, (gpointer *)&s, HG_TYPE_STRING, 0, NULL, 0, requisition_size);
	if (ret)
		*ret = s;

	return retval;
}

/**
 * hg_string_new_with_value:
 * @mem:
 * @ret:
 * @string:
 * @length:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_string_new_with_value(hg_mem_t    *mem,
			 gpointer    *ret,
			 const gchar *string,
			 gsize        length)
{
	hg_string_t *s = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (string != NULL, Qnil);
	hg_return_val_if_fail (length > 0 && length <= HG_STRING_MAX_SIZE, Qnil);

	retval = hg_object_new(mem, (gpointer *)&s, HG_TYPE_STRING, 0, string, 0, length);
	if (ret)
		*ret = s;

	return retval;
}

/**
 * hg_string_length:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
guint
hg_string_length(const hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, -1);

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
guint
hg_string_maxlength(const hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, -1);

	return string->allocated_size;
}

/**
 * hg_string_clear:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_string_clear(hg_string_t *string)
{
	gchar *s;

	hg_return_val_if_fail (string != NULL, FALSE);

	s = hg_mem_lock_object(string->o.mem, string->qstring);
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
gboolean
hg_string_append_c(hg_string_t *string,
		   gchar        c)
{
	gchar *s;
	gboolean retval = FALSE;

	hg_return_val_if_fail (string != NULL, FALSE);

	if (string->length < string->allocated_size) {
		s = hg_mem_lock_object(string->o.mem, string->qstring);
		s[string->length++] = c;
		hg_mem_unlock_object(string->o.mem, string->qstring);
		retval = TRUE;
	} else {
		if (!string->is_fixed_size) {
			hg_quark_t new_qstr = Qnil;

			hg_return_val_if_fail (string->allocated_size <= HG_STRING_MAX_SIZE, FALSE);

			new_qstr = hg_mem_realloc(string->o.mem,
						  string->qstring,
						  MIN (string->allocated_size + HG_STRING_ALLOC_SIZE + 1, HG_STRING_MAX_SIZE + 1),
						  (gpointer *)&s);
			if (new_qstr == Qnil)
				return FALSE;

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
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_string_append(hg_string_t *string,
		 const gchar *str,
		 gssize       length)
{
	gchar *s;
	gboolean retval = FALSE;
	gssize i;

	hg_return_val_if_fail (string != NULL, FALSE);

	if (length < 0)
		length = strlen(str);
	if ((string->length + length) < string->allocated_size) {
		s = hg_mem_lock_object(string->o.mem, string->qstring);
		for (i = 0; i < length; i++)
			s[string->length++] = str[i];
		hg_mem_unlock_object(string->o.mem, string->qstring);
		retval = TRUE;
	} else {
		if (!string->is_fixed_size) {
			hg_quark_t new_qstr = Qnil;

			hg_return_val_if_fail ((string->allocated_size + length) <= HG_STRING_MAX_SIZE, FALSE);

			new_qstr = hg_mem_realloc(string->o.mem,
						  string->qstring,
						  MIN (string->allocated_size + length + HG_STRING_ALLOC_SIZE + 1, HG_STRING_MAX_SIZE + 1),
						  (gpointer *)&s);
			if (new_qstr == Qnil)
				return FALSE;

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
 * hg_string_insert_c:
 * @string:
 * @c:
 * @index:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_string_insert_c(hg_string_t *string,
		   gchar        c,
		   guint        index)
{
	gchar *s;
	gboolean retval = FALSE;
	gsize i;

	hg_return_val_if_fail (string != NULL, FALSE);
	hg_return_val_if_fail (index < string->allocated_size, FALSE);

	s = hg_mem_lock_object(string->o.mem, string->qstring);
	s[index] = c;

	for (i = 0; i < string->allocated_size; i++) {
		if (s[i] == 0) {
			string->length = i;
			retval = TRUE;
		}
	}
	if (!retval)
		string->length = string->allocated_size;
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
gboolean
hg_string_erase(hg_string_t *string,
		gssize       pos,
		gssize       length)
{
	gchar *s;
	gssize i, j;

	hg_return_val_if_fail (string != NULL, FALSE);
	hg_return_val_if_fail (pos > 0, FALSE);

	if (length < 0)
		length = string->length - pos;

	hg_return_val_if_fail ((pos + length) <= string->length, FALSE);

	s = hg_mem_lock_object(string->o.mem, string->qstring);
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
gboolean
hg_string_concat(hg_string_t *string1,
		 hg_string_t *string2)
{
	const gchar *s;
	gboolean retval;

	hg_return_val_if_fail (string1 != NULL, FALSE);
	hg_return_val_if_fail (string2 != NULL, FALSE);

	s = hg_mem_lock_object(string2->o.mem, string2->qstring);
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
gchar
hg_string_index(hg_string_t *string,
		guint        index)
{
	const gchar *s;
	gchar retval;

	hg_return_val_if_fail (string != NULL, 0);
	hg_return_val_if_fail (index < string->length, 0);

	s = hg_mem_lock_object(string->o.mem, string->qstring);
	retval = s[index];
	hg_mem_unlock_object(string->o.mem, string->qstring);
	
	return retval;
}

/**
 * hg_string_get_static_cstr:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
const gchar *
hg_string_get_static_cstr(hg_string_t *string)
{
	gchar *retval;

	hg_return_val_if_fail (string != NULL, NULL);

	retval = hg_mem_lock_object(string->o.mem, string->qstring);
	if (string->offset == 0)
		retval[string->length] = 0;

	return retval;
}

/**
 * hg_string_get_cstr:
 * @string:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_string_get_cstr(hg_string_t  *string,
		   gpointer     *ret)
{
	hg_quark_t q;
	gchar *retval = NULL, *s;
	gsize i;

	hg_return_val_if_fail (string != NULL, Qnil);

	s = hg_mem_lock_object(string->o.mem, string->qstring);
	q = hg_mem_alloc(string->o.mem, string->length + 1, (gpointer *)&retval);
	if (q != Qnil) {
		for (i = 0; i < string->length; i++) {
			retval[i] = s[string->offset + i];
		}
		retval[i] = 0;
	}
	if (ret)
		*ret = retval;

	return q;
}

/**
 * hg_string_fix_string_size:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_string_fix_string_size(hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, FALSE);

	if (!string->is_fixed_size) {
		hg_quark_t new_qstr;

		new_qstr = hg_mem_realloc(string->o.mem, string->qstring, string->length + 1, NULL);
		if (new_qstr == Qnil)
			return FALSE;
		string->is_fixed_size = TRUE;
		string->allocated_size = string->length;
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
gboolean
hg_string_compare(const hg_string_t *a,
		  const hg_string_t *b)
{
	hg_return_val_if_fail (b != NULL, FALSE);

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
gboolean
hg_string_ncompare(const hg_string_t *a,
		   const hg_string_t *b,
		   guint              length)
{
	const gchar *sb;
	gboolean retval;

	hg_return_val_if_fail (a != NULL, FALSE);
	hg_return_val_if_fail (b != NULL, FALSE);

	sb = hg_mem_lock_object(b->o.mem, b->qstring);

	retval = hg_string_ncompare_with_cstr(a, sb, length);

	hg_mem_unlock_object(b->o.mem, b->qstring);

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
gboolean
hg_string_ncompare_with_cstr(const hg_string_t *a,
			     const gchar       *b,
			     gssize             length)
{
	const gchar *sa;
	gboolean retval;

	hg_return_val_if_fail (a != NULL, FALSE);
	hg_return_val_if_fail (b != NULL, FALSE);

	if (length < 0)
		length = strlen(b);
	if (length != a->length)
		return FALSE;

	sa = hg_mem_lock_object(a->o.mem, a->qstring);
	retval = memcmp(sa, b, length) == 0;

	hg_mem_unlock_object(a->o.mem, a->qstring);

	return retval;
}
