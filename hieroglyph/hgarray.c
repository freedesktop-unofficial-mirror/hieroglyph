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
#include "hgarray.h"

#define HG_ARRAY_ALLOC_SIZE	65535
#define HG_ARRAY_MAX_SIZE	65535 /* defined as PostScript spec */


HG_DEFINE_VTABLE (array)

/*< private >*/
static gsize
_hg_object_array_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_array_t));
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
	array->length = 0;
	array->qcontainer = qcontainer;
	if (array->qcontainer == Qnil && size > 0) {
		hg_quark_t *container = NULL;

		array->qcontainer = hg_mem_alloc(array->o.mem, sizeof (hg_quark_t) * size, (gpointer *)&container);
		hg_return_val_if_fail (array->qcontainer != Qnil, FALSE);

		memset(container, 0, sizeof (hg_quark_t) * size);

		hg_mem_unlock_object(array->o.mem, array->qcontainer);
	}

	return TRUE;
}

static void
_hg_object_array_free(hg_object_t *object)
{
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
 * hg_array_set:
 * @array:
 * @quark:
 * @index:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_set(hg_array_t *array,
	     hg_quark_t  quark,
	     gsize       index)
{
	hg_quark_t *container;

	hg_return_val_if_fail (array != NULL, FALSE);
	hg_return_val_if_fail ((array->offset + index) < array->allocated_size, FALSE);

	container = hg_mem_lock_object(array->o.mem, array->qcontainer);
	if (container == NULL)
		return FALSE;

	container[array->offset + index] = quark;
	if (array->length < (array->offset + index + 1))
		array->length = array->offset + index + 1;

	hg_mem_unlock_object(array->o.mem, array->qcontainer);

	return TRUE;
}

/**
 * hg_array_get:
 * @array:
 * @index:
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

	hg_return_val_with_gerror_if_fail (array != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail ((array->offset + index) < array->length, Qnil, error);

	container = hg_mem_lock_object(array->o.mem, array->qcontainer);
	hg_return_val_with_gerror_if_fail (container != NULL, Qnil, error);

	retval = container[array->offset + index];

	hg_mem_unlock_object(array->o.mem, array->qcontainer);

	return retval;
}

/**
 * hg_array_insert:
 * @array:
 * @quark:
 * @pos:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_array_insert(hg_array_t *array,
		hg_quark_t  quark,
		gsize       pos)
{
	gssize i;
	hg_quark_t *container;

	hg_return_val_if_fail (array != NULL, FALSE);
	hg_return_val_if_fail ((array->offset + pos) < array->allocated_size, FALSE);

	if ((array->offset + pos) < array->length) {
		hg_return_val_if_fail ((array->length + 1) < array->allocated_size, FALSE);

		container = hg_mem_lock_object(array->o.mem, array->qcontainer);
		if (container == NULL)
			return FALSE;

		array->length++;
		for (i = array->length; i > array->offset + pos; i--) {
			container[i] = container[i - 1];
		}
		container[array->offset + pos] = quark;

		hg_mem_unlock_object(array->o.mem, array->qcontainer);
	} else {
		return hg_array_set(array, quark, pos);
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
	hg_return_val_if_fail (array->offset == 0, FALSE);
	hg_return_val_if_fail ((array->offset + pos) < array->length, FALSE);

	container = hg_mem_lock_object(array->o.mem, array->qcontainer);
	if (container == NULL)
		return FALSE;

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
	GError *err = NULL;

	hg_return_with_gerror_if_fail (array != NULL, error);
	hg_return_with_gerror_if_fail (func != NULL, error);

	container = hg_mem_lock_object(array->o.mem, array->qcontainer);
	if (container == NULL) {
		g_set_error(&err, HG_ERROR, EINVAL,
			    "%s: Invalid quark to obtain the container object: mem: %p, quark: %" G_GSIZE_FORMAT,
			    __PRETTY_FUNCTION__, array->o.mem, array->qcontainer);
		goto finalize;
	}
	for (i = 0; i < array->length; i++) {
		if (!func(array->o.mem, container[i], data, error))
			break;
	}
	hg_mem_unlock_object(array->o.mem, array->qcontainer);
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)", err->message, err->code);
		}
		g_error_free(err);
	}
}
