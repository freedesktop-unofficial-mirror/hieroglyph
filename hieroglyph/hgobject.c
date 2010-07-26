/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.c
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
#include "config.h"
#endif

#include <string.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgobject.h"


static hg_quark_t _hg_object_new(hg_mem_t    *mem,
                                 hg_type_t    type,
                                 gsize        size,
                                 hg_object_t **ret);


static hg_object_vtable_t *__hg_object_vtables[HG_TYPE_END];
static gboolean __hg_object_is_initialized = FALSE;

/*< private >*/
static hg_quark_t
_hg_object_new(hg_mem_t     *mem,
	       hg_type_t     type,
	       gsize         size,
	       hg_object_t **ret)
{
	hg_quark_t retval;
	hg_object_t *object;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	if (hg_type_is_simple(type))
		return Qnil;

	retval = hg_mem_alloc(mem, sizeof (hg_object_t) > size ? sizeof (hg_object_t) : size, (gpointer *)&object);
	if (retval == Qnil)
		return retval;

	memset(object, 0, sizeof (hg_object_t));
	object->type = type;
	object->mem = mem;
	object->self = hg_quark_new(type, retval);

	if (ret)
		*ret = object;

	return object->self;
}

/*< public >*/
/**
 * hg_object_init:
 *
 * FIXME
 */
void
hg_object_init(void)
{
	memset(__hg_object_vtables, 0, sizeof (hg_quark_t) * (HG_TYPE_END - 1));
	__hg_object_is_initialized = TRUE;
}

/**
 * hg_object_tini:
 *
 * FIXME
 */
void
hg_object_tini(void)
{
	__hg_object_is_initialized = FALSE;
}

/**
 * hg_object_register:
 * @type:
 * @vtable:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_object_register(hg_type_t           type,
		   hg_object_vtable_t *vtable)
{
	hg_return_val_if_fail (__hg_object_is_initialized, FALSE);

	__hg_object_vtables[type] = vtable;

	return TRUE;
}

/**
 * hg_object_new:
 * @mem:
 * @type:
 * @preallocated_size:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_object_new(hg_mem_t  *mem,
	      gpointer  *ret,
	      hg_type_t  type,
	      gsize      preallocated_size,
	      ...)
{
	hg_object_vtable_t *v;
	hg_object_t *retval = NULL;
	hg_quark_t index;
	gsize size;
	va_list ap;

	hg_return_val_if_fail (__hg_object_is_initialized, Qnil);
	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (type < HG_TYPE_END, Qnil);
	hg_return_val_if_fail (__hg_object_vtables[type] != NULL, Qnil);

	v = __hg_object_vtables[type];
	size = v->get_capsulated_size();
	index = _hg_object_new(mem, type, size + hg_mem_aligned_size (preallocated_size), &retval);
	if (index == Qnil)
		return Qnil;

	va_start(ap, preallocated_size);

	if (!v->initialize(retval, ap)) {
		hg_mem_free(mem, index);
		return Qnil;
	}
	if (ret)
		*ret = retval;
	else
		hg_mem_unlock_object(mem, index);

	va_end(ap);

	return index;
}

/**
 * hg_object_free:
 *
 * FIXME
 */
void
hg_object_free(hg_mem_t   *mem,
	       hg_quark_t  index)
{
	hg_object_t *object;
	hg_object_vtable_t *v;

	hg_return_if_fail (__hg_object_is_initialized);
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (index != Qnil);
	hg_return_if_lock_fail (object, mem, index);
	hg_return_if_fail (object->type < HG_TYPE_END);
	hg_return_if_fail (__hg_object_vtables[object->type] != NULL);

	v = __hg_object_vtables[object->type];
	v->free(object);

	hg_mem_unlock_object(mem, index);
	hg_mem_free(mem, index);
}

/**
 * hg_object_copy:
 * @object:
 * @func:
 * @user_data:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_object_copy(hg_object_t              *object,
	       hg_quark_iterate_func_t   func,
	       gpointer                  user_data,
	       gpointer                 *ret,
	       GError                  **error)
{
	hg_object_vtable_t *v;

	hg_return_val_with_gerror_if_fail (__hg_object_is_initialized, Qnil, error);
	hg_return_val_with_gerror_if_fail (object != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail (object->type < HG_TYPE_END, Qnil, error);
	hg_return_val_with_gerror_if_fail (__hg_object_vtables[object->type] != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail (func != NULL, Qnil, error);

	v = __hg_object_vtables[object->type];

	return v->copy(object, func, user_data, ret, error);
}

/**
 * hg_object_to_cstr:
 * @object:
 * @func:
 * @user_data:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gchar *
hg_object_to_cstr(hg_object_t              *object,
		  hg_quark_iterate_func_t   func,
		  gpointer                  user_data,
		  GError                  **error)
{
	hg_object_vtable_t *v;

	hg_return_val_with_gerror_if_fail (__hg_object_is_initialized, NULL, error);
	hg_return_val_with_gerror_if_fail (object != NULL, NULL, error);
	hg_return_val_with_gerror_if_fail (object->type < HG_TYPE_END, NULL, error);
	hg_return_val_with_gerror_if_fail (__hg_object_vtables[object->type] != NULL, NULL, error);
	hg_return_val_with_gerror_if_fail (func != NULL, NULL, error);

	v = __hg_object_vtables[object->type];

	return v->to_cstr(object, func, user_data, error);
}
