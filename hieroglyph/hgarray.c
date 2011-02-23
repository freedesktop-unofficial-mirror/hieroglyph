/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
 * Copyright (C) 2005-2011 Akira TAGOH
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
#include "hgmatrix.h"
#include "hgmem.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgnull.h"
#include "hgreal.h"
#include "hgarray.h"

#include "hgarray.proto.h"

#define HG_ARRAY_ALLOC_SIZE	65535
#define HG_ARRAY_MAX_SIZE	65535 /* defined as PostScript spec */


HG_DEFINE_VTABLE_WITH (array, NULL, NULL, NULL);

/*< private >*/
static hg_usize_t
_hg_object_array_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_array_t));
}

static hg_uint_t
_hg_object_array_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_array_initialize(hg_object_t *object,
			    va_list      args)
{
	hg_array_t *array = (hg_array_t *)object;
	hg_usize_t offset, size;
	hg_quark_t qcontainer;

	qcontainer = va_arg(args, hg_quark_t);
	offset = va_arg(args, hg_usize_t);
	size = va_arg(args, hg_usize_t);

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
		      hg_pointer_t              user_data,
		      hg_pointer_t             *ret)
{
	hg_array_t *array = (hg_array_t *)object, *a = NULL;
	hg_quark_t retval, q, qr;
	hg_usize_t i, len;

	hg_return_val_if_fail (object->type == HG_TYPE_ARRAY, Qnil, HG_e_typecheck);

	if (object->on_copying != Qnil)
		return object->on_copying;

	len = hg_array_maxlength(array);
	object->on_copying = retval = hg_array_new(array->o.mem,
						   len,
						   (hg_pointer_t *)&a);
	if (retval != Qnil) {
		for (i = 0; i < len; i++) {
			q = hg_array_get(array, i);
			if (q == Qnil) {
				hg_debug(HG_MSGCAT_ARRAY, "Unable to obtain the object during copying the object: [%ld/%ld]",
					 i, len);
				goto bail;
			}
			qr = func(q, user_data, NULL);
			if (qr == Qnil) {
				hg_debug(HG_MSGCAT_ARRAY, "Unable to copy the object: [%ld/%ld]",
					 i, len);
				goto bail;
			}
			if (!hg_array_set(a, qr, i, TRUE)) {
				hg_debug(HG_MSGCAT_ARRAY, "Unable to store the object during copying the object: [%ld/%ld]",
					 i, len);
				goto bail;
			}
		}
		if (array->qname == Qnil) {
			a->qname = Qnil;
		} else {
			a->qname = func(array->qname, user_data, NULL);
			if (a->qname == Qnil) {
				hg_debug(HG_MSGCAT_ARRAY, "Unable to copy the name of the object.");
				goto bail;
			}
		}
		if (ret)
			*ret = a;
		else
			hg_mem_unlock_object(a->o.mem, retval);
	}
	goto finalize;
  bail:
	if (a)
		hg_object_free(a->o.mem, retval);

	retval = Qnil;
  finalize:
	object->on_copying = Qnil;

	return retval;
}

static hg_char_t *
_hg_object_array_to_cstr(hg_object_t             *object,
			 hg_quark_iterate_func_t  func,
			 hg_pointer_t             user_data)
{
	hg_array_t *array = (hg_array_t *)object;
	hg_quark_t q, qr;
	hg_usize_t i, len;
	GString *retval = g_string_new(NULL);
	hg_char_t *s;

	hg_return_val_if_fail (object->type == HG_TYPE_ARRAY, NULL, HG_e_typecheck);

	if (array->qname != Qnil) {
		const hg_char_t *p;

		p = HG_MEM_LOCK (array->o.mem, array->qname);
		if (!HG_ERROR_IS_SUCCESS0 ())
			return g_strdup("[FAILED]");
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
		q = hg_array_get(array, i);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			g_string_append(retval, "...");
			continue;
		}
		qr = func(q, user_data, NULL);
		s = (hg_char_t *)HGQUARK_TO_POINTER (qr);
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

static hg_bool_t
_hg_object_array_gc_mark(hg_object_t *object)
{
	hg_array_t *array = (hg_array_t *)object;
	hg_quark_t q;
	hg_usize_t i, len;

	hg_return_val_if_fail (object->type == HG_TYPE_ARRAY, FALSE, HG_e_typecheck);

	hg_debug(HG_MSGCAT_GC, "array: marking container");
	if (!hg_mem_gc_mark(array->o.mem, array->qcontainer))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "array: marking name");
	if (!hg_mem_gc_mark(array->o.mem, array->qname))
		return FALSE;

	len = hg_array_length(array);

	for (i = 0; i < len; i++) {
		hg_mem_t *m;

		q = hg_array_get(array, i);
		m = hg_mem_spool_get(hg_quark_get_mem_id(q));
		if (!hg_mem_gc_mark(m, q))
			return FALSE;
	}

	return TRUE;
}

static hg_bool_t
_hg_object_array_compare(hg_object_t             *o1,
			 hg_object_t             *o2,
			 hg_quark_compare_func_t  func,
			 hg_pointer_t             user_data)
{
	hg_usize_t i, len;
	hg_bool_t retval = TRUE;
	hg_array_t *a1 = (hg_array_t *)o1, *a2 = (hg_array_t *)o2;

	hg_return_val_if_fail (o1 != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (o2 != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (func != NULL, FALSE, HG_e_VMerror);

	len = hg_array_maxlength(a1);
	if (len != hg_array_maxlength(a2))
		return FALSE;

	for (i = 0; i < len; i++) {
		hg_quark_t q1, q2;

		q1 = hg_array_get(a1, i);
		q2 = hg_array_get(a2, i);
		if ((retval = func(q1, q2, user_data)) == FALSE)
			break;
	}

	return retval;
}

static hg_bool_t
_hg_array_maybe_expand(hg_array_t *array,
		       hg_usize_t  length)
{
	hg_usize_t i;
	hg_quark_t *container;

	if (array->is_fixed_size ||
	    array->offset != 0)
		return TRUE;

	hg_return_val_if_fail ((length + 1) <= array->allocated_size, FALSE, HG_e_rangecheck);

	if (array->qcontainer == Qnil) {
		array->qcontainer = hg_mem_alloc(array->o.mem,
						 sizeof (hg_quark_t) * (length + 1),
						 (hg_pointer_t *)&container);
		if (array->qcontainer == Qnil)
			return FALSE;
		for (i = 0; i < length; i++) {
			container[i] = HG_QNULL;
		}
		hg_mem_unlock_object(array->o.mem, array->qcontainer);
	} else {
		hg_quark_t q;

		q = hg_mem_realloc(array->o.mem,
				   array->qcontainer,
				   sizeof (hg_quark_t) * (length + 1),
				   (hg_pointer_t *)&container);

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

HG_INLINE_FUNC void
_hg_array_convert_to_matrix(hg_array_t  *array,
			    hg_matrix_t *matrix)
{
	hg_quark_t q;
	hg_usize_t i;

	for (i = 0; i < 6; i++) {
		q = hg_array_get(array, i);
		if (HG_IS_QINT (q)) {
			matrix->d[i] = (hg_real_t)HG_INT (q);
		} else {
			matrix->d[i] = HG_REAL (q);
		}
	}
}

HG_INLINE_FUNC void
_hg_array_convert_from_matrix(hg_array_t  *array,
			      hg_matrix_t *matrix)
{
	hg_usize_t i;

	for (i = 0; i < 6; i++) {
		hg_array_set(array, HG_QREAL (matrix->d[i]), i, TRUE);
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
hg_array_new(hg_mem_t     *mem,
	     hg_usize_t    size,
	     hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_array_t *array = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (size < (HG_ARRAY_MAX_SIZE + 1), Qnil, HG_e_limitcheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	retval = hg_object_new(mem, (hg_pointer_t *)&array, HG_TYPE_ARRAY, 0, Qnil, 0LL, size);
	if (ret)
		*ret = array;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_array_set:
 * @array:
 * @quark:
 * @index_:
 * @force:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_array_set(hg_array_t  *array,
	     hg_quark_t   quark,
	     hg_usize_t   index_,
	     hg_bool_t    force)
{
	hg_quark_t *container;
	hg_usize_t new_length;

	hg_return_val_if_fail (array != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	if (index_ > hg_array_maxlength(array)) {
		hg_debug(HG_MSGCAT_ARRAY, "Wrong index to access the array: index: %ld, size: %ld",
			   index_, hg_array_maxlength(array));

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	if (!force &&
	    !(hg_quark_is_simple_object(quark) ||
	      hg_quark_get_type(quark) == HG_TYPE_OPER) &&
	    hg_mem_spool_get_type(array->o.mem) == HG_MEM_TYPE_GLOBAL &&
	    hg_mem_spool_get_type(hg_mem_spool_get(hg_quark_get_mem_id(quark))) == HG_MEM_TYPE_LOCAL) {
		hg_debug(HG_MSGCAT_ARRAY, "Unable to store the object allocated in the local memory into the global memory");

		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	new_length = MAX (index_, array->length);
	if (index_ >= new_length && new_length >= array->length) {
		if (!_hg_array_maybe_expand(array, new_length))
			return FALSE;
	}
	hg_return_val_if_lock_fail (container,
				    array->o.mem,
				    array->qcontainer,
				    FALSE);

	container[array->offset + index_] = quark;

	hg_mem_unlock_object(array->o.mem, array->qcontainer);

	hg_mem_unref(hg_mem_spool_get(hg_quark_get_mem_id(quark)),
		     quark);

	return TRUE;
}

/**
 * hg_array_get:
 * @array:
 * @index_:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_array_get(hg_array_t *array,
	     hg_usize_t  index_)
{
	hg_quark_t retval = Qnil;
	hg_quark_t *container;

	hg_return_val_if_fail (array != NULL, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, Qnil, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	if (index_ >= hg_array_maxlength(array)) {
		hg_debug(HG_MSGCAT_ARRAY, "Wrong index to access the array: index: %ld, size: %ld",
			 index_, hg_array_maxlength(array));

		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_rangecheck);
	}

	if (array->qcontainer == Qnil) {
		/* emulate containing null */
		return HG_QNULL;
	}

	if (index_ >= array->length) {
		if (!_hg_array_maybe_expand(array, index_))
			return Qnil;
	}
	hg_return_val_if_lock_fail (container,
				    array->o.mem,
				    array->qcontainer,
				    Qnil);

	retval = container[array->offset + index_];

	hg_mem_unlock_object(array->o.mem, array->qcontainer);

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
hg_bool_t
hg_array_insert(hg_array_t *array,
		hg_quark_t  quark,
		hg_size_t   pos)
{
	hg_size_t i;
	hg_quark_t *container;

	hg_return_val_if_fail (array != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (array->offset == 0, FALSE, HG_e_invalidaccess);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	if (pos < 0)
		pos = array->length;

	hg_return_val_if_fail (pos < array->allocated_size, FALSE, HG_e_rangecheck);

	if (pos < array->length) {
		hg_return_val_if_fail ((array->length + 1) < array->allocated_size, FALSE, HG_e_rangecheck);
		hg_return_val_if_fail (array->qcontainer != Qnil, FALSE, HG_e_VMerror); /* unlikely */

		if (!_hg_array_maybe_expand(array, array->length))
			return FALSE;

		hg_return_val_if_lock_fail (container,
					    array->o.mem,
					    array->qcontainer,
					    FALSE);

		for (i = array->length; i > pos; i--) {
			container[i] = container[i - 1];
		}
		container[pos] = quark;

		hg_mem_unlock_object(array->o.mem, array->qcontainer);

		hg_mem_unref(hg_mem_spool_get(hg_quark_get_mem_id(quark)),
			     quark);
	} else {
		return hg_array_set(array, quark, pos, FALSE);
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
hg_bool_t
hg_array_remove(hg_array_t *array,
		hg_usize_t  pos)
{
	hg_usize_t i;
	hg_quark_t *container;

	hg_return_val_if_fail (array != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (array->offset == 0, FALSE, HG_e_invalidaccess);
	hg_return_val_if_fail (pos < array->length, FALSE, HG_e_rangecheck);
	hg_return_val_if_lock_fail (container,
				    array->o.mem,
				    array->qcontainer,
				    FALSE);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

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
hg_usize_t
hg_array_length(hg_array_t *array)
{
	hg_return_val_if_fail (array != NULL, 0, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, 0, HG_e_typecheck);

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
hg_usize_t
hg_array_maxlength(hg_array_t *array)
{
	hg_return_val_if_fail (array != NULL, 0, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, 0, HG_e_typecheck);

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
hg_array_foreach(hg_array_t               *array,
		 hg_array_traverse_func_t  func,
		 hg_pointer_t              data)
{
	hg_quark_t *container;
	hg_usize_t i;

	hg_return_if_fail (array != NULL, HG_e_typecheck);
	hg_return_if_fail (array->o.type == HG_TYPE_ARRAY, HG_e_typecheck);
	hg_return_if_fail (func != NULL, HG_e_VMerror);
	hg_return_if_lock_fail (container,
				array->o.mem,
				array->qcontainer);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	for (i = 0; i < array->length; i++) {
		if (!func(array->o.mem, container[i], data))
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
hg_array_set_name(hg_array_t      *array,
		  const hg_char_t *name)
{
	hg_char_t *p;
	hg_usize_t len;

	hg_return_if_fail (array != NULL, HG_e_typecheck);
	hg_return_if_fail (array->o.type == HG_TYPE_ARRAY, HG_e_typecheck);
	hg_return_if_fail (name != NULL, HG_e_VMerror);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	len = strlen(name);
	array->qname = hg_mem_alloc(array->o.mem, len, (hg_pointer_t *)&p);
	if (array->qname != Qnil) {
		memcpy(p, name, len);
		hg_mem_unlock_object(array->o.mem, array->qname);
	}
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
hg_array_make_subarray(hg_array_t   *array,
		       hg_usize_t    start_index,
		       hg_usize_t    end_index,
		       hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_array_t *a;

	hg_return_val_if_fail (array != NULL, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (array->o.type == HG_TYPE_ARRAY, Qnil, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	retval = hg_array_new(array->o.mem, 0, (hg_pointer_t *)&a);
	if (retval == Qnil) {
		return Qnil;
	}
	if (end_index > HG_ARRAY_MAX_SIZE) {
		/* make an empty array */
	} else {
		if (!hg_array_copy_as_subarray(array, a,
					       start_index,
					       end_index)) {
			hg_mem_free(array->o.mem, retval);
			retval = Qnil;
		}
		if (ret)
			*ret = a;
		else
			hg_mem_unlock_object(array->o.mem, retval);
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
hg_bool_t
hg_array_copy_as_subarray(hg_array_t *src,
			  hg_array_t *dest,
			  hg_usize_t  start_index,
			  hg_usize_t  end_index)
{
	hg_return_val_if_fail (src != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (src->o.type == HG_TYPE_ARRAY, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (dest != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (dest->o.type == HG_TYPE_ARRAY, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (start_index < hg_array_maxlength(src), FALSE, HG_e_rangecheck);
	hg_return_val_if_fail (end_index < hg_array_maxlength(src), FALSE, HG_e_rangecheck);
	hg_return_val_if_fail (start_index <= end_index, FALSE, HG_e_rangecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

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
 * hg_array_is_matrix:
 * @array:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_array_is_matrix(hg_array_t *array)
{
	hg_usize_t i;
	hg_quark_t q;

	hg_return_val_if_fail (array != NULL, FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	if (hg_array_maxlength(array) != 6) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}

	for (i = 0; i < 6; i++) {
		q = hg_array_get(array, i);
		if (!HG_IS_QINT (q) &&
		    !HG_IS_QREAL (q)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
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
hg_bool_t
hg_array_from_matrix(hg_array_t  *array,
		     hg_matrix_t *matrix)
{
	hg_return_val_if_fail (array != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (matrix != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (hg_array_maxlength(array) == 6, FALSE, HG_e_rangecheck);

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
hg_bool_t
hg_array_to_matrix(hg_array_t  *array,
		   hg_matrix_t *matrix)
{
	hg_return_val_if_fail (array != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (matrix != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (hg_array_is_matrix(array), FALSE, HG_e_rangecheck);

	_hg_array_convert_to_matrix(array, matrix);

	return TRUE;
}
