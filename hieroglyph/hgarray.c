/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
 * Copyright (C) 2005-2010 Akira TAGOH
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
#include <config.h>
#endif

#include <string.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgnull.h"
#include "hgreal.h"
#include "hgarray.h"

#define HG_ARRAY_ALLOC_SIZE	65535
#define HG_ARRAY_MAX_SIZE	65535 /* defined as PostScript spec */


static gboolean _hg_array_maybe_expand(hg_array_t *array,
				       gsize       length);

HG_DEFINE_VTABLE_WITH (array, NULL, NULL, NULL);

/*< private >*/
static gsize
_hg_object_array_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_array_t));
}

static guint
_hg_object_array_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static gboolean
_hg_object_array_initialize(hg_object_t *object,
			    va_list      args)
{
	hg_array_t *array = (hg_array_t *)object;
	gsize offset, size;
	hg_quark_t qcontainer;

	qcontainer = va_arg(args, hg_quark_t);
	offset = va_arg(args, gsize);
	size = va_arg(args, gsize);

	array->offset = offset;
	array->allocated_size = size;
	array->qcontainer = qcontainer;
	if (array->qcontainer != Qnil) {
		array->length = size - array->offset;
		array->is_fixed_size = TRUE;
	} else {
		array->length = 0;
		array->is_fixed_size = FALSE;
	}
	/* not allocating any containers here */
	array->qname = Qnil;

	return TRUE;
}

static hg_quark_t
_hg_object_array_copy(hg_object_t              *object,
		      hg_quark_iterate_func_t   func,
		      gpointer                  user_data,
		      gpointer                 *ret,
		      GError                  **error)
{
	hg_array_t *array = (hg_array_t *)object, *a;
	hg_quark_t retval, q, qr;
	gsize i, len;
	GError *err = NULL;

	hg_return_val_if_fail (object->type == HG_TYPE_ARRAY, Qnil);

	if (object->on_copying != Qnil)
		return object->on_copying;

	len = hg_array_maxlength(array);
	object->on_copying = retval = hg_array_new(array->o.mem,
						   len,
						   (gpointer *)&a);
	if (retval != Qnil) {
		for (i = 0; i < len; i++) {
			q = hg_array_get(array, i, &err);
			if (err)
				goto finalize;
			qr = func(q, user_data, NULL, &err);
			if (err)
				goto finalize;
			hg_array_set(a, qr, i, &err);
			if (err)
				goto finalize;
		}
		a->qname = func(array->qname, user_data, NULL, &err);
		if (err)
			goto finalize;
		if (ret)
			*ret = a;
		else
			hg_mem_unlock_object(a->o.mem, retval);
	} else {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "Out of memory");
	}
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
		if (a)
			hg_object_free(a->o.mem, retval);

		retval = Qnil;
	}
	object->on_copying = Qnil;

	return retval;
}

static gchar *
_hg_object_array_to_cstr(hg_object_t              *object,
			 hg_quark_iterate_func_t   func,
			 gpointer                  user_data,
			 GError                  **error)
{
	hg_array_t *array = (hg_array_t *)object;
	hg_quark_t q, qr;
	gsize i, len;
	GString *retval = g_string_new(NULL);
	GError *err = NULL;
	gchar *s;

	hg_return_val_if_fail (object->type == HG_TYPE_ARRAY, NULL);

	if (array->qname != Qnil) {
		const gchar *p;

		p = HG_MEM_LOCK (array->o.mem, array->qname, error);
		s = g_strdup(p);
		hg_mem_unlock_object(array->o.mem, array->qname);

		return s;
	}

	if (object->on_to_cstr)
		return g_strdup("[...]");
	object->on_to_cstr = TRUE;

	g_string_append_c(retval, '[');
	len = hg_array_length(array);
	for (i = 0; i < len; i++) {
		if (i != 0)
			g_string_append_c(retval, ' ');
		q = hg_array_get(array, i, &err);
		if (err) {
			g_string_append(retval, "...");
			g_clear_error(&err);
			continue;
		}
		qr = func(q, user_data, NULL, NULL);
		s = (gchar *)HGQUARK_TO_POINTER (qr);
		if (s == NULL) {
			g_string_append(retval, "...");
			continue;
		} else {
			g_string_append(retval, s);
			g_free(s);
		}
	}
	g_string_append_c(retval, ']');

	object->on_to_cstr = FALSE;

	return g_string_free(retval, FALSE);
}

static gboolean
_hg_object_array_gc_mark(hg_object_t           *object,
			 hg_gc_iterate_func_t   func,
			 gpointer               user_data,
			 GError               **error)
{
	hg_array_t *array = (hg_array_t *)object;
	hg_quark_t q;
	gsize i, len;
	GError *err = NULL;
	gboolean retval = TRUE;

	hg_return_val_if_fail (object->type == HG_TYPE_ARRAY, FALSE);

#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: (array) marking container\n");
#endif
	if (!hg_mem_gc_mark(array->o.mem, array->qcontainer, &err))
		goto finalize;
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: (array) marking name\n");
#endif
	if (!hg_mem_gc_mark(array->o.mem, array->qname, &err))
		goto finalize;

	len = hg_array_length(array);

	for (i = 0; i < len; i++) {
		q = hg_array_get(array, i, &err);
		if (err)
			goto finalize;
		if (!func(q, user_data, &err))
			goto finalize;
	}
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

static gboolean
_hg_object_array_compare(hg_object_t             *o1,
			 hg_object_t             *o2,
			 hg_quark_compare_func_t  func,
			 gpointer                 user_data)
{
	gsize i;
	gboolean retval = TRUE;
	gsize len;
	hg_array_t *a1 = (hg_array_t *)o1, *a2 = (hg_array_t *)o2;

	hg_return_val_if_fail (o1 != NULL, FALSE);
	hg_return_val_if_fail (o2 != NULL, FALSE);
	hg_return_val_if_fail (func != NULL, FALSE);

	len = hg_array_maxlength(a1);
	if (len != hg_array_maxlength(a2))
		return FALSE;

	for (i = 0; i < len; i++) {
		hg_quark_t q1, q2;

		q1 = hg_array_get(a1, i, NULL);
		q2 = hg_array_get(a2, i, NULL);
		if ((retval = func(q1, q2, user_data)) == FALSE)
			break;
	}

	return retval;
}

static gboolean
_hg_array_maybe_expand(hg_array_t *array,
		       gsize       length)
{
	gsize i;
	hg_quark_t *container;

	if (array->is_fixed_size ||
	    array->offset != 0)
		return TRUE;

	hg_return_val_if_fail ((length + 1) <= array->allocated_size, FALSE);

	if (array->qcontainer == Qnil) {
		array->qcontainer = hg_mem_alloc(array->o.mem,
						 sizeof (hg_quark_t) * (length + 1),
						 (gpointer *)&container);
		for (i = 0; i < length; i++) {
			container[i] = HG_QNULL;
		}
		hg_mem_unlock_object(array->o.mem, array->qcontainer);
	} else {
		hg_quark_t q;

		q = hg_mem_realloc(array->o.mem,
				   array->qcontainer,
				   sizeof (hg_quark_t) * (length + 1),
				   (gpointer *)&container);

		if (q == Qnil)
			return FALSE;
		array->qcontainer = q;

		for (i = array->length; i <= length; i++) {
			container[i] = HG_QNULL;
		}
		hg_mem_unlock_object(array->o.mem, array->qcontainer);
	}
	array->length = length + 1;

	return TRUE;
}

G_INLINE_FUNC void
_hg_array_convert_to_matrix(hg_array_t  *array,
			    hg_matrix_t *matrix)
{
	hg_quark_t q;
	gsize i;

	for (i = 0; i < 6; i++) {
		q = hg_array_get(array, i, NULL);
		if (HG_IS_QINT (q)) {
			matrix->d[i] = (gdouble)HG_INT (q);
		} else {
			matrix->d[i] = HG_REAL (q);
		}
	}
}

G_INLINE_FUNC void
_hg_array_convert_from_matrix(hg_array_t  *array,
			      hg_matrix_t *matrix)
{
	gsize i;

	for (i = 0; i < 6; i++) {
		hg_array_set(array, HG_QREAL (matrix->d[i]), i, NULL);
	}
}

/*< public >*/
/**
 * hg_array_new:
 * @mem:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_array_new(hg_mem_t *mem,
	     gsize     size,
	     gpointer *ret)
{
	hg_quark_t retval;
	hg_array_t *array = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (size < (HG_ARRAY_MAX_SIZE + 1), Qnil);

	retval = hg_object_new(mem, (gpointer *)&array, HG_TYPE_ARRAY, 0, Qnil, 0LL, size);
	if (ret)
		*ret = array;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_array_free:
 * @array:
 *
 * FIXME
 */
void
hg_array_free(hg_array_t *array)
{
	hg_return_if_fail (array != NULL);
	hg_return_if_fail (array->o.type == HG_TYPE_ARRAY);

	hg_mem_reserved_spool_remove(array->o.mem, array->o.self);

	hg_mem_free(array->o.mem, array->qname);
	hg_mem_free(array->o.mem, array->o.self);
}

/**
 * hg_array_set:
 * @array:
 * @quark:
 * @index:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_set(hg_array_t  *array,
	     hg_quark_t   quark,
	     gsize        index,
	     GError     **error)
{
	hg_quark_t *container;
	gsize new_length;
	GError *err = NULL;
	gboolean retval = TRUE;

	hg_return_val_with_gerror_if_fail (array != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (array->o.type == HG_TYPE_ARRAY, FALSE, error);
	hg_return_val_with_gerror_if_fail ((array->offset + index) < array->allocated_size, FALSE, error);

	new_length = MAX (index, array->length);
	if (index >= new_length && new_length >= array->length) {
		if (!_hg_array_maybe_expand(array, new_length)) {
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "Out of memory");
			retval = FALSE;
			goto finalize;
		}
	}
	hg_return_val_with_gerror_if_lock_fail (container,
						array->o.mem,
						array->qcontainer,
						error,
						FALSE);

	container[array->offset + index] = quark;

	hg_mem_unlock_object(array->o.mem, array->qcontainer);

	hg_mem_reserved_spool_remove(hg_mem_get(hg_quark_get_mem_id(quark)),
				     quark);
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

/**
 * hg_array_get:
 * @array:
 * @index:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_array_get(hg_array_t  *array,
	     gsize        index,
	     GError     **error)
{
	hg_quark_t retval;
	hg_quark_t *container;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (array != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail (array->o.type == HG_TYPE_ARRAY, Qnil, error);
	hg_return_val_with_gerror_if_fail (index < array->allocated_size, Qnil, error);

	if (array->qcontainer == Qnil) {
		/* emulate containing null */
		return HG_QNULL;
	}

	if (index >= array->length) {
		if (!_hg_array_maybe_expand(array, index)) {
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "Out of memory");
			retval = Qnil;
			goto finalize;
		}
	}
	hg_return_val_with_gerror_if_lock_fail (container,
						array->o.mem,
						array->qcontainer,
						error,
						Qnil);

	retval = container[array->offset + index];

	hg_mem_unlock_object(array->o.mem, array->qcontainer);
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

/**
 * hg_array_insert:
 * @array:
 * @quark:
 * @pos:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_insert(hg_array_t  *array,
		hg_quark_t   quark,
		gssize       pos,
		GError     **error)
{
	gssize i;
	hg_quark_t *container;

	hg_return_val_with_gerror_if_fail (array != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (array->o.type == HG_TYPE_ARRAY, FALSE, error);
	hg_return_val_with_gerror_if_fail (array->offset == 0, FALSE, error);

	if (pos < 0)
		pos = array->length;

	hg_return_val_with_gerror_if_fail (pos < array->allocated_size, FALSE, error);

	if (pos < array->length) {
		hg_return_val_with_gerror_if_fail ((array->length + 1) < array->allocated_size, FALSE, error);
		hg_return_val_with_gerror_if_fail (array->qcontainer != Qnil, FALSE, error); /* unlikely */
		hg_return_val_with_gerror_if_lock_fail (container,
							array->o.mem,
							array->qcontainer,
							error,
							FALSE);

		array->length++;
		for (i = array->length; i > pos; i--) {
			container[i] = container[i - 1];
		}
		container[pos] = quark;

		hg_mem_unlock_object(array->o.mem, array->qcontainer);

		hg_mem_reserved_spool_remove(hg_mem_get(hg_quark_get_mem_id(quark)),
					     quark);
	} else {
		return hg_array_set(array, quark, pos, error);
	}

	return TRUE;
}

/**
 * hg_array_remove:
 * @array:
 * @pos:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_remove(hg_array_t *array,
		gsize       pos)
{
	gsize i;
	hg_quark_t *container;

	hg_return_val_if_fail (array != NULL, FALSE);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, FALSE);
	hg_return_val_if_fail (array->offset == 0, FALSE);
	hg_return_val_if_fail (pos < array->length, FALSE);
	hg_return_val_if_lock_fail (container,
				    array->o.mem,
				    array->qcontainer,
				    FALSE);

	for (i = pos; i < array->length; i++) {
		container[i] = container[i + 1];
	}
	array->length--;

	hg_mem_unlock_object(array->o.mem, array->qcontainer);

	return TRUE;
}

/**
 * hg_array_length:
 * @array:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_array_length(hg_array_t *array)
{
	hg_return_val_if_fail (array != NULL, 0);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, 0);

	return array->length;
}

/**
 * hg_array_maxlength:
 * @array:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_array_maxlength(hg_array_t *array)
{
	hg_return_val_if_fail (array != NULL, 0);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, 0);

	return array->allocated_size;
}

/**
 * hg_array_foreach:
 * @array:
 * @func:
 * @data:
 *
 * FIXME
 */
void
hg_array_foreach(hg_array_t                *array,
		 hg_array_traverse_func_t   func,
		 gpointer                   data,
		 GError                   **error)
{
	hg_quark_t *container;
	gsize i;

	hg_return_with_gerror_if_fail (array != NULL, error);
	hg_return_with_gerror_if_fail (array->o.type == HG_TYPE_ARRAY, error);
	hg_return_with_gerror_if_fail (func != NULL, error);
	hg_return_with_gerror_if_lock_fail (container,
					    array->o.mem,
					    array->qcontainer,
					    error);

	for (i = 0; i < array->length; i++) {
		if (!func(array->o.mem, container[i], data, error))
			break;
	}
	hg_mem_unlock_object(array->o.mem, array->qcontainer);
}

/**
 * hg_array_set_name:
 * @array:
 * @name:
 *
 * FIXME
 */
void
hg_array_set_name(hg_array_t  *array,
		  const gchar *name)
{
	gchar *p;
	gsize len;

	hg_return_if_fail (array != NULL);
	hg_return_if_fail (array->o.type == HG_TYPE_ARRAY);
	hg_return_if_fail (name != NULL);

	len = strlen(name);
	array->qname = hg_mem_alloc(array->o.mem, len, (gpointer *)&p);
	if (array->qname == Qnil) {
		g_warning("%s: Unable to allocate memory.", __PRETTY_FUNCTION__);
	} else {
		memcpy(p, name, len);
	}
	hg_mem_unlock_object(array->o.mem, array->qname);
}

/**
 * hg_array_make_subarray:
 * @array:
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
hg_array_make_subarray(hg_array_t  *array,
		       gsize        start_index,
		       gsize        end_index,
		       gpointer    *ret,
		       GError     **error)
{
	hg_quark_t retval;
	GError *err = NULL;
	hg_array_t *a;

	hg_return_val_with_gerror_if_fail (array != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail (array->o.type == HG_TYPE_ARRAY, Qnil, error);

	retval = hg_array_new(array->o.mem, 0, (gpointer *)&a);
	if (retval == Qnil) {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "Out of memory");
		goto error;
	}
	if (end_index > HG_ARRAY_MAX_SIZE) {
		/* make an empty array */
	} else {
		if (!hg_array_copy_as_subarray(array, a,
					       start_index,
					       end_index,
					       &err)) {
			hg_mem_free(array->o.mem, retval);
			retval = Qnil;
		}
		if (ret)
			*ret = a;
		else
			hg_mem_unlock_object(array->o.mem, retval);
	}
  error:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

/**
 * hg_array_copy_as_subarray:
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
hg_array_copy_as_subarray(hg_array_t  *src,
			  hg_array_t  *dest,
			  gsize        start_index,
			  gsize        end_index,
			  GError     **error)
{
	hg_return_val_with_gerror_if_fail (src != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (src->o.type == HG_TYPE_ARRAY, FALSE, error);
	hg_return_val_with_gerror_if_fail (dest != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (dest->o.type == HG_TYPE_ARRAY, FALSE, error);
	hg_return_val_with_gerror_if_fail (start_index < hg_array_maxlength(src), FALSE, error);
	hg_return_val_with_gerror_if_fail (end_index < hg_array_maxlength(src), FALSE, error);
	hg_return_val_with_gerror_if_fail (start_index <= end_index, FALSE, error);

	/* destroy the unnecessary destination's container */
	hg_mem_free(dest->o.mem, dest->qcontainer);
	/* make a subarray */
	dest->qcontainer = src->qcontainer;
	dest->allocated_size = dest->length = end_index - start_index + 1;
	dest->offset = start_index;
	dest->is_fixed_size = TRUE;
	dest->is_subarray = TRUE;

	return TRUE;
}

/**
 * hg_array_matrix_new:
 * @mem:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_array_matrix_new(hg_mem_t *mem,
		    gpointer *ret)
{
	hg_quark_t retval;
	hg_array_t *a = NULL;

	retval = hg_array_new(mem, 6, (gpointer *)&a);
	if (retval != Qnil) {
		hg_array_matrix_ident(a);

		if (ret) {
			*ret = a;
		} else {
			hg_mem_unlock_object(mem, retval);
		}
	}

	return retval;
}

/**
 * hg_array_is_matrix:
 * @array:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_is_matrix(hg_array_t *array)
{
	gsize i;
	hg_quark_t q;

	hg_return_val_if_fail (array != NULL, FALSE);

	if (hg_array_maxlength(array) != 6)
		return FALSE;

	for (i = 0; i < 6; i++) {
		q = hg_array_get(array, i, NULL);
		if (!HG_IS_QINT (q) &&
		    !HG_IS_QREAL (q))
			return FALSE;
	}

	return TRUE;
}

/**
 * hg_array_from_matrix:
 * @array:
 * @matrix:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_from_matrix(hg_array_t  *array,
		     hg_matrix_t *matrix)
{
	hg_return_val_if_fail (array != NULL, FALSE);
	hg_return_val_if_fail (matrix != NULL, FALSE);
	hg_return_val_if_fail (hg_array_maxlength(array) == 6, FALSE);

	_hg_array_convert_from_matrix(array, matrix);

	return TRUE;
}

/**
 * hg_array_to_matrix:
 * @array:
 * @matrix:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_to_matrix(hg_array_t  *array,
		   hg_matrix_t *matrix)
{
	hg_return_val_if_fail (array != NULL, FALSE);
	hg_return_val_if_fail (matrix != NULL, FALSE);
	hg_return_val_if_fail (hg_array_is_matrix(array), FALSE);

	_hg_array_convert_to_matrix(array, matrix);

	return TRUE;
}

/**
 * hg_array_matrix_ident:
 * @matrix:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_matrix_ident(hg_array_t *matrix)
{
	hg_matrix_t m;

	hg_return_val_if_fail (matrix != NULL, FALSE);
	hg_return_val_if_fail (hg_array_maxlength(matrix) == 6, FALSE);

	m.mtx.xx = 1;
	m.mtx.xy = 0;
	m.mtx.yx = 0;
	m.mtx.yy = 1;
	m.mtx.x0 = 0;
	m.mtx.y0 = 0;

	_hg_array_convert_from_matrix(matrix, &m);

	return TRUE;
}

/**
 * hg_array_matrix_multiply:
 * @matrix1:
 * @matrix2:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_matrix_multiply(hg_array_t *matrix1,
			 hg_array_t *matrix2,
			 hg_array_t *ret)
{
	hg_matrix_t m1, m2, m3;

	hg_return_val_if_fail (hg_array_is_matrix(matrix1), FALSE);
	hg_return_val_if_fail (hg_array_is_matrix(matrix2), FALSE);
	hg_return_val_if_fail (hg_array_maxlength(ret) == 6, FALSE);

	_hg_array_convert_to_matrix(matrix1, &m1);
	_hg_array_convert_to_matrix(matrix2, &m2);

	m3.mtx.xx = m1.mtx.xx * m2.mtx.xx + m1.mtx.yx * m2.mtx.xy;
	m3.mtx.yx = m1.mtx.xx * m2.mtx.yx + m1.mtx.yx * m2.mtx.yy;
	m3.mtx.xy = m1.mtx.xy * m2.mtx.xx + m1.mtx.yy * m2.mtx.xy;
	m3.mtx.yy = m1.mtx.xy * m2.mtx.yx + m1.mtx.yy * m2.mtx.yy;
	m3.mtx.x0 = m1.mtx.x0 * m2.mtx.xx + m1.mtx.y0 * m2.mtx.xy + m2.mtx.x0;
	m3.mtx.y0 = m1.mtx.x0 * m2.mtx.yx + m1.mtx.y0 * m2.mtx.yy + m2.mtx.y0;

	_hg_array_convert_from_matrix(ret, &m3);

	return TRUE;
}

/**
 * hg_array_matrix_invert:
 * @matrix:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_matrix_invert(hg_array_t *matrix,
		       hg_array_t *ret)
{
	hg_matrix_t m1, m2;
	gdouble det;

	hg_return_val_if_fail (hg_array_is_matrix(matrix), FALSE);
	hg_return_val_if_fail (hg_array_maxlength(ret) == 6, FALSE);

	_hg_array_convert_to_matrix(matrix, &m1);

	/*
	 * detA = a11a22a33 + a21a32a13 + a31a12a23
	 *      - a11a32a23 - a31a22a13 - a21a12a33
	 *
	 *      xx xy 0
	 * A = {yx yy 0}
	 *      x0 y0 1
	 * detA = xx * yy * 1 + yx * y0 * 0 + x0 * xy * 0 - xx * y0 * 0 - x0 * yy * 0 - yx * xy * 1
	 * detA = xx * yy - yx * xy
	 */
	det = m1.mtx.xx * m1.mtx.yy - m1.mtx.yx * m1.mtx.xy;
	if (det == 0)
		return FALSE;

	/*
	 *                    d      -b   0
	 * A-1 = 1/detA {    -c       a   0}
	 *               -dx0+by0 cx0-ay0 1
	 */
	m2.mtx.xx =  m1.mtx.yy / det;
	m2.mtx.xy = -m1.mtx.xy / det;
	m2.mtx.yx = -m1.mtx.yx / det;
	m2.mtx.yy =  m1.mtx.xx / det;
	m2.mtx.x0 = (-m1.mtx.yy * m1.mtx.x0 + m1.mtx.xy * m1.mtx.y0) / det;
	m2.mtx.y0 = ( m1.mtx.yx * m1.mtx.x0 - m1.mtx.xx * m1.mtx.y0) / det;

	_hg_array_convert_from_matrix(ret, &m2);

	return TRUE;
}
