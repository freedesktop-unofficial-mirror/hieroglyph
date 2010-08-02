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

#include <ctype.h>
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
	gsize length;

	str->qstring = Qnil;
	str->length = 0;

	string = va_arg(args, const gchar *);
	str->offset = va_arg(args, gsize);
	hg_return_val_if_fail (str->offset >= 0, FALSE);

	length = va_arg(args, gsize);

	/* not allocating any containers here */
	str->allocated_size = length + 1;
	str->is_fixed_size = FALSE;

	if (string) {
		return hg_string_append(str, string, length, NULL);
	}

	return TRUE;
}

static void
_hg_object_string_free(hg_object_t *object)
{
}

static hg_quark_t
_hg_object_string_copy(hg_object_t              *object,
		       hg_quark_iterate_func_t   func,
		       gpointer                  user_data,
		       gpointer                 *ret,
		       GError                  **error)
{
	hg_string_t *s = (hg_string_t *)object;
	hg_quark_t retval;
	GError *err = NULL;
	gchar *cstr = hg_string_get_cstr(s);

	retval = hg_string_new_with_value(s->o.mem, NULL,
					  cstr, -1);
	g_free(cstr);
	if (retval == Qnil) {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "Out of memory");
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

static gchar *
_hg_object_string_to_cstr(hg_object_t              *object,
			  hg_quark_iterate_func_t   func,
			  gpointer                  user_data,
			  GError                  **error)
{
	GString *retval = g_string_new(NULL);
	hg_string_t *s = (hg_string_t *)object;
	gchar *cstr = hg_string_get_cstr(s);
	gchar buffer[8];
	gsize i;

	g_string_append_c(retval, '(');
	for (i = s->offset; i < hg_string_maxlength(s); i++) {
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

static gboolean
_hg_object_string_gc_mark(hg_object_t           *object,
			  hg_gc_iterate_func_t   func,
			  gpointer               user_data,
			  GError               **error)
{
	return FALSE;
}

static gboolean
_hg_string_maybe_expand(hg_string_t *string)
{
	if (string->is_fixed_size ||
	    string->offset != 0)
		return TRUE;

	hg_return_val_if_fail (string->length < string->allocated_size, FALSE);

	if (string->qstring == Qnil) {
		string->qstring = hg_mem_alloc(string->o.mem,
					       string->length + 1,
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
	hg_return_val_if_fail (requisition_size <= HG_STRING_MAX_SIZE, Qnil);

	retval = hg_object_new(mem, (gpointer *)&s, HG_TYPE_STRING, 0, NULL, 0, requisition_size);
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
			 gssize       length)
{
	hg_string_t *s = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (string != NULL, Qnil);
	hg_return_val_if_fail (length <= HG_STRING_MAX_SIZE, Qnil);

	if (length < 0)
		length = strlen(string);

	retval = hg_object_new(mem, (gpointer *)&s, HG_TYPE_STRING, 0, string, 0, length);
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
	       gboolean     free_segment)
{
	if (string == NULL)
		return;

	if (free_segment)
		hg_mem_free(string->o.mem, string->qstring);
	hg_mem_free(string->o.mem, string->o.self);
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
gboolean
hg_string_clear(hg_string_t *string)
{
	gchar *s;

	hg_return_val_if_fail (string != NULL, FALSE);

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
gboolean
hg_string_append_c(hg_string_t  *string,
		   gchar         c,
		   GError      **error)
{
	gchar *s;
	gboolean retval = FALSE;
	gsize old_length;
	GError *err = NULL;

	hg_return_val_if_fail (string != NULL, FALSE);

	if (string->length < hg_string_maxlength(string)) {
		old_length = string->length;
		string->length++;
		if (!_hg_string_maybe_expand(string)) {
			string->length = old_length;
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "Out of memory");
			goto finalize;
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

			hg_return_val_if_fail (hg_string_maxlength(string) <= HG_STRING_MAX_SIZE, FALSE);

			new_qstr = hg_mem_realloc(string->o.mem,
						  string->qstring,
						  MIN (string->allocated_size + HG_STRING_ALLOC_SIZE + 1, HG_STRING_MAX_SIZE + 1),
						  (gpointer *)&s);
			if (new_qstr == Qnil)
				return FALSE;

			string->qstring = new_qstr;
			string->allocated_size = MIN (string->allocated_size + HG_STRING_ALLOC_SIZE, HG_STRING_MAX_SIZE);
			s[string->length++] = c;
			hg_mem_unlock_object(string->o.mem, new_qstr);
			retval = TRUE;
		}
	}
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
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
hg_string_append(hg_string_t  *string,
		 const gchar  *str,
		 gssize        length,
		 GError      **error)
{
	gchar *s;
	gboolean retval = FALSE;
	gssize i, old_length;
	GError *err = NULL;

	hg_return_val_if_fail (string != NULL, FALSE);
	hg_return_val_if_fail (str != NULL, FALSE);

	if (length < 0)
		length = strlen(str);
	if (string->length == 0 ||
	    (string->length + length) < hg_string_maxlength(string)) {
		old_length = string->length;
		string->length += length;
		if (!_hg_string_maybe_expand(string)) {
			string->length = old_length;
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "Out of memory");
			goto finalize;
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

			hg_return_val_if_fail ((hg_string_maxlength(string) + length) <= HG_STRING_MAX_SIZE, FALSE);

			new_qstr = hg_mem_realloc(string->o.mem,
						  string->qstring,
						  MIN (string->allocated_size + length + HG_STRING_ALLOC_SIZE + 1, HG_STRING_MAX_SIZE + 1),
						  (gpointer *)&s);
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
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
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
gboolean
hg_string_overwrite_c(hg_string_t  *string,
		      gchar         c,
		      guint         index,
		      GError      **error)
{
	gchar *s;
	gboolean retval = FALSE;
	gsize i, old_length;
	GError *err = NULL;

	hg_return_val_if_fail (string != NULL, FALSE);
	hg_return_val_if_fail (index < hg_string_maxlength(string), FALSE);

	if (string->length <= index) {
		old_length = string->length;
		string->length = index;
		if (!_hg_string_maybe_expand(string)) {
			string->length = old_length;
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "Out of memory");
			goto error;
		}
	}

	hg_return_val_if_lock_fail (s,
				    string->o.mem,
				    string->qstring,
				    FALSE);

	s[index] = c;

	for (i = index; i < string->allocated_size; i++) {
		if (s[i] == 0) {
			string->length = i;
			retval = TRUE;
		}
	}
	if (!retval)
		string->length = hg_string_maxlength(string);
	hg_mem_unlock_object(string->o.mem, string->qstring);

	return TRUE;
  error:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return FALSE;
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

	if (string->qstring == Qnil) {
		/* no data yet */
		return TRUE;
	}
	if (length < 0)
		length = string->length - pos;

	hg_return_val_if_fail ((pos + length) <= string->length, FALSE);
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
gboolean
hg_string_concat(hg_string_t *string1,
		 hg_string_t *string2)
{
	const gchar *s;
	gboolean retval;

	hg_return_val_if_fail (string1 != NULL, FALSE);
	hg_return_val_if_fail (string2 != NULL, FALSE);
	hg_return_val_if_lock_fail (s,
				    string2->o.mem,
				    string2->qstring,
				    FALSE);

	retval = hg_string_append(string1, s, string2->length, NULL);
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
gchar *
hg_string_get_cstr(hg_string_t *string)
{
	gchar *retval, *cstr;

	hg_return_val_if_fail (string != NULL, NULL);

	if (string->qstring == Qnil)
		return NULL;

	hg_return_val_if_lock_fail (cstr,
				    string->o.mem,
				    string->qstring,
				    NULL);

	retval = g_strndup(&cstr[string->offset], string->length);

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
gboolean
hg_string_fix_string_size(hg_string_t *string)
{
	hg_return_val_if_fail (string != NULL, FALSE);

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
	hg_return_val_if_lock_fail (sb,
				    b->o.mem,
				    b->qstring,
				    FALSE);

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

	hg_return_val_if_lock_fail (sa,
				    a->o.mem,
				    a->qstring,
				    FALSE);

	retval = memcmp(sa, b, length) == 0;

	hg_mem_unlock_object(a->o.mem, a->qstring);

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
gboolean
hg_string_append_printf(hg_string_t *string,
			const gchar *format,
			...)
{
	va_list ap;
	gchar *ret;
	gboolean retval;

	hg_return_val_if_fail (string != NULL, FALSE);
	hg_return_val_if_fail (format != NULL, FALSE);

	va_start(ap, format);

	ret = g_strdup_vprintf(format, ap);
	retval = hg_string_append(string, ret, -1, NULL);
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
			 gsize         start_index,
			 gsize         end_index,
			 gpointer     *ret,
			 GError      **error)
{
	hg_string_t *s;
	hg_quark_t retval;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (string != NULL, Qnil, error);

	retval = hg_string_new(string->o.mem, (gpointer *)&s, 0);
	if (retval == Qnil) {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "Out of memory");
		goto error;
	}
	if (end_index > HG_STRING_MAX_SIZE) {
		/* make an empty string */
	} else {
		if (!hg_string_copy_as_substring(string, s,
						 start_index,
						 end_index,
						 &err)) {
			hg_mem_free(string->o.mem, retval);
			retval = Qnil;
		}
		if (ret)
			*ret = s;
		else
			hg_mem_unlock_object(string->o.mem, retval);
	}
  error:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
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
gboolean
hg_string_copy_as_substring(hg_string_t  *src,
			    hg_string_t  *dest,
			    gsize         start_index,
			    gsize         end_index,
			    GError      **error)
{
	hg_return_val_with_gerror_if_fail (src != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (dest != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (start_index < hg_string_maxlength(src), FALSE, error);
	hg_return_val_with_gerror_if_fail (end_index < hg_string_maxlength(src), FALSE, error);
	hg_return_val_with_gerror_if_fail (start_index <= end_index, FALSE, error);

	/* destroy the unnecessary destination's container */
	hg_mem_free(dest->o.mem, dest->qstring);
	/* make a substring */
	dest->qstring = src->qstring;
	dest->length = end_index - start_index + 1;
	dest->allocated_size = dest->length + 1;
	dest->is_fixed_size = TRUE;
	dest->offset = start_index;

	return TRUE;
}
