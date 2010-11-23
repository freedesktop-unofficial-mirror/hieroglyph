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
#include "hgmark.h"
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
	hg_object_vtable_t *v;
	guint flags;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	if (hg_type_is_simple(type))
		return Qnil;

	v = __hg_object_vtables[type];
	flags = v->get_allocation_flags();
	retval = hg_mem_alloc_with_flags(mem,
					 sizeof (hg_object_t) > size ? sizeof (hg_object_t) : size,
					 flags,
					 (gpointer *)&object);
	if (retval == Qnil)
		return retval;

	memset(object, 0, sizeof (hg_object_t));
	object->type = type;
	object->mem = mem;
	object->self = hg_quark_new(type, retval);
	object->on_copying = Qnil;
	object->on_gc = FALSE;
	object->on_to_cstr = FALSE;

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

	hg_mem_reserved_spool_add(mem, index);

	va_start(ap, preallocated_size);

	if (!v->initialize(retval, ap)) {
		hg_mem_reserved_spool_remove(mem, index);
		hg_mem_free(mem, index);
		return Qnil;
	}
	if (ret)
		*ret = retval;
	else
		hg_mem_unlock_object(mem, index);

	if (v->free)
		hg_mem_add_gc_finalizer(mem, index, hg_object_free);

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
	hg_return_after_eval_if_fail (object->type < HG_TYPE_END,
				      hg_mem_unlock_object(mem, index));
	hg_return_after_eval_if_fail (__hg_object_vtables[object->type] != NULL,
				      hg_mem_unlock_object(mem, index));

	v = __hg_object_vtables[object->type];
	if (v->free) {
		v->free(object);
		hg_mem_remove_gc_finalizer(mem, index);
	}

	hg_mem_unlock_object(mem, index);
	hg_mem_free(mem, index);

	hg_mem_reserved_spool_remove(mem, index);
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

/**
 * hg_object_gc_mark:
 * @object:
 * @func:
 * @user_data:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_object_gc_mark(hg_object_t           *object,
		  hg_gc_iterate_func_t   func,
		  gpointer               user_data,
		  GError               **error)
{
	hg_object_vtable_t *v;
	gboolean retval = FALSE;

	hg_return_val_with_gerror_if_fail (__hg_object_is_initialized, FALSE, error);
	hg_return_val_with_gerror_if_fail (object != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (object->type < HG_TYPE_END, FALSE, error);
	hg_return_val_with_gerror_if_fail (__hg_object_vtables[object->type] != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (func != NULL, FALSE, error);

	if (object->on_gc)
		return TRUE;

	object->on_gc = TRUE;

	if (hg_mem_gc_mark(object->mem, object->self, error)) {
		v = __hg_object_vtables[object->type];

		retval = v->gc_mark(object, func, user_data, error);
	}

	object->on_gc = FALSE;

	return retval;
}

/**
 * hg_object_compare:
 * @o1:
 * @o2:
 * @func:
 * @user_data:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_object_compare(hg_object_t             *o1,
		  hg_object_t             *o2,
		  hg_quark_compare_func_t  func,
		  gpointer                 user_data)
{
	hg_object_vtable_t *v;
	gboolean retval = FALSE;

	hg_return_val_if_fail (__hg_object_is_initialized, FALSE);
	hg_return_val_if_fail (o1 != NULL, FALSE);
	hg_return_val_if_fail (o1->type < HG_TYPE_END, FALSE);
	hg_return_val_if_fail (o2 != NULL, FALSE);
	hg_return_val_if_fail (o2->type < HG_TYPE_END, FALSE);
	hg_return_val_if_fail (__hg_object_vtables[o1->type] != NULL, FALSE);
	hg_return_val_if_fail (func != NULL, FALSE);

	if (o1->type != o2->type)
		return FALSE;

	if (o1->on_compare != o2->on_compare)
		return FALSE;

	if (o1->on_compare)
		return TRUE;

	o1->on_compare = TRUE;
	o2->on_compare = TRUE;

	v = __hg_object_vtables[o1->type];

	retval = v->compare(o1, o2, func, user_data);

	o1->on_compare = FALSE;
	o2->on_compare = FALSE;

	return retval;
}

/**
 * hg_object_set_attributes:
 * @object:
 * @readable:
 * @writable:
 * @executable:
 * @editable:
 *
 * FIXME
 */
void
hg_object_set_attributes(hg_object_t *object,
			 gint         readable,
			 gint         writable,
			 gint         executable,
			 gint         editable)
{
	hg_object_vtable_t *v;

	hg_return_if_fail (__hg_object_is_initialized);
	hg_return_if_fail (object != NULL);
	hg_return_if_fail (object->type < HG_TYPE_END);
	hg_return_if_fail (__hg_object_vtables[object->type] != NULL);

	v = __hg_object_vtables[object->type];

	if (v->set_attributes)
		v->set_attributes(object, readable, writable, executable, editable);
}

/**
 * hg_object_get_attributes:
 * @object:
 *
 * FIXME
 *
 * Returns:
 */
gint
hg_object_get_attributes(hg_object_t *object)
{
	hg_object_vtable_t *v;
	gint retval = -1;

	hg_return_val_if_fail (__hg_object_is_initialized, -1);
	hg_return_val_if_fail (object != NULL, -1);
	hg_return_val_if_fail (object->type < HG_TYPE_END, -1);
	hg_return_val_if_fail (__hg_object_vtables[object->type] != NULL, -1);

	v = __hg_object_vtables[object->type];

	if (v->get_attributes)
		retval = v->get_attributes(object);

	return retval;
}
