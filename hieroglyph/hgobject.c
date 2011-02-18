/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.c
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
#include "config.h"
#endif

#include <string.h>
#include "hgerror.h"
#include "hgmark.h"
#include "hgmem.h"
#include "hgobject.h"

#include "hgobject.proto.h"

static hg_object_vtable_t *__hg_object_vtables[HG_TYPE_END];
static hg_bool_t __hg_object_is_initialized = FALSE;

/*< private >*/
static hg_quark_t
_hg_object_new(hg_mem_t     *mem,
	       hg_type_t     type,
	       hg_usize_t    size,
	       hg_object_t **ret)
{
	hg_quark_t retval;
	hg_object_t *object;
	hg_object_vtable_t *v;
	hg_uint_t flags;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (size > 0, Qnil, HG_e_VMerror);

	if (hg_type_is_simple(type))
		return Qnil;

	v = __hg_object_vtables[type];
	flags = v->get_allocation_flags();
	retval = hg_mem_alloc_with_flags(mem,
					 sizeof (hg_object_t) > size ? sizeof (hg_object_t) : size,
					 flags,
					 (hg_pointer_t *)&object);
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

static hg_quark_t
_hg_object_quark_iterate_copy(hg_quark_t    qdata,
			      hg_pointer_t  user_data,
			      hg_pointer_t *ret)
{
	return hg_object_quark_copy((hg_mem_t *)user_data,
				    qdata, ret);
}

static void
_hg_object_finalizer(hg_pointer_t p)
{
	hg_object_t *o = (hg_object_t *)p;
	hg_object_vtable_t *v;

	hg_return_if_fail (__hg_object_is_initialized, HG_e_VMerror);
	hg_return_if_fail (o->type < HG_TYPE_END, HG_e_VMerror);
	hg_return_if_fail (__hg_object_vtables[o->type] != NULL, HG_e_VMerror);

	v = __hg_object_vtables[o->type];
	if (v->free) {
		v->free(o);
	}
	hg_mem_reserved_spool_remove(o->mem, o->self);
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
hg_bool_t
hg_object_register(hg_type_t           type,
		   hg_object_vtable_t *vtable)
{
	hg_return_val_if_fail (__hg_object_is_initialized, FALSE, HG_e_VMerror);

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
hg_object_new(hg_mem_t     *mem,
	      hg_pointer_t *ret,
	      hg_type_t     type,
	      hg_usize_t    preallocated_size,
	      ...)
{
	hg_object_vtable_t *v;
	hg_object_t *retval = NULL;
	hg_quark_t index_;
	hg_usize_t size;
	va_list ap;
	hg_int_t id;

	hg_return_val_if_fail (__hg_object_is_initialized, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (type < HG_TYPE_END, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (__hg_object_vtables[type] != NULL, Qnil, HG_e_VMerror);

	v = __hg_object_vtables[type];
	size = v->get_capsulated_size();
	index_ = _hg_object_new(mem, type, size + HG_ALIGNED_TO_POINTER (preallocated_size), &retval);
	if (index_ == Qnil)
		return Qnil;

	hg_mem_reserved_spool_add(mem, index_);

	va_start(ap, preallocated_size);

	if (!v->initialize(retval, ap)) {
		hg_error_t e = hg_errno;

		hg_mem_reserved_spool_remove(mem, index_);
		hg_mem_free(mem, index_);

		hg_errno = e;

		return Qnil;
	}
	id = hg_mem_spool_add_finalizer(mem, _hg_object_finalizer);
	hg_mem_set_finalizer(mem, index_, id);
	if (ret)
		*ret = retval;
	else
		hg_mem_unlock_object(mem, index_);

	va_end(ap);

	return index_;
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
hg_object_copy(hg_object_t             *object,
	       hg_quark_iterate_func_t  func,
	       hg_pointer_t             user_data,
	       hg_pointer_t            *ret)
{
	hg_object_vtable_t *v;

	hg_return_val_if_fail (__hg_object_is_initialized, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (object != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (object->type < HG_TYPE_END, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (__hg_object_vtables[object->type] != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, Qnil, HG_e_VMerror);

	v = __hg_object_vtables[object->type];

	return v->copy(object, func, user_data, ret);
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
hg_char_t *
hg_object_to_cstr(hg_object_t             *object,
		  hg_quark_iterate_func_t  func,
		  hg_pointer_t             user_data)
{
	hg_object_vtable_t *v;

	hg_return_val_if_fail (__hg_object_is_initialized, NULL, HG_e_VMerror);
	hg_return_val_if_fail (object != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (object->type < HG_TYPE_END, NULL, HG_e_VMerror);
	hg_return_val_if_fail (__hg_object_vtables[object->type] != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, NULL, HG_e_VMerror);

	v = __hg_object_vtables[object->type];

	return v->to_cstr(object, func, user_data);
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
hg_bool_t
hg_object_gc_mark(hg_object_t          *object,
		  hg_gc_iterate_func_t  func,
		  hg_pointer_t          user_data)
{
	hg_object_vtable_t *v;
	hg_bool_t retval = TRUE;

	hg_return_val_if_fail (__hg_object_is_initialized, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (object != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (object->type < HG_TYPE_END, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (__hg_object_vtables[object->type] != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, FALSE, HG_e_VMerror);

	if (object->on_gc)
		return TRUE;

	object->on_gc = TRUE;

	if ((retval = hg_mem_gc_mark(object->mem, object->self))) {
		v = __hg_object_vtables[object->type];

		retval = v->gc_mark(object, func, user_data);
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
hg_bool_t
hg_object_compare(hg_object_t             *o1,
		  hg_object_t             *o2,
		  hg_quark_compare_func_t  func,
		  hg_pointer_t             user_data)
{
	hg_object_vtable_t *v;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (__hg_object_is_initialized, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (o1 != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (o1->type < HG_TYPE_END, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (o2 != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (o2->type < HG_TYPE_END, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (__hg_object_vtables[o1->type] != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, FALSE, HG_e_VMerror);

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
 * hg_object_set_acl:
 * @object:
 * @readable:
 * @writable:
 * @executable:
 * @editable:
 *
 * FIXME
 */
void
hg_object_set_acl(hg_object_t *object,
		  hg_int_t     readable,
		  hg_int_t     writable,
		  hg_int_t     executable,
		  hg_int_t     editable)
{
	hg_object_vtable_t *v;

	hg_return_if_fail (__hg_object_is_initialized, HG_e_VMerror);
	hg_return_if_fail (object != NULL, HG_e_VMerror);
	hg_return_if_fail (object->type < HG_TYPE_END, HG_e_VMerror);
	hg_return_if_fail (__hg_object_vtables[object->type] != NULL, HG_e_VMerror);

	v = __hg_object_vtables[object->type];

	if (v->set_acl)
		v->set_acl(object, readable, writable, executable, editable);
}

/**
 * hg_object_get_acl:
 * @object:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_acl_t
hg_object_get_acl(hg_object_t *object)
{
	hg_object_vtable_t *v;
	hg_quark_acl_t retval = -1;

	hg_return_val_if_fail (__hg_object_is_initialized, -1, HG_e_VMerror);
	hg_return_val_if_fail (object != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (object->type < HG_TYPE_END, -1, HG_e_VMerror);
	hg_return_val_if_fail (__hg_object_vtables[object->type] != NULL, -1, HG_e_VMerror);

	v = __hg_object_vtables[object->type];

	if (v->get_acl)
		retval = v->get_acl(object);

	return retval;
}

/**
 * hg_object_quark_copy:
 * @mem:
 * @qdata:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_object_quark_copy(hg_mem_t     *mem,
		     hg_quark_t    qdata,
		     hg_pointer_t *ret)
{
	hg_quark_t retval = Qnil;
	hg_object_t *o;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (qdata != Qnil, Qnil, HG_e_VMerror);

	if (hg_quark_is_simple_object(qdata) ||
	    hg_quark_get_type(qdata) == HG_TYPE_OPER)
		return qdata;

	o = HG_MEM_LOCK (mem, qdata);
	if (o) {
		hg_quark_acl_t acl = 0;

		retval = hg_object_copy(o, _hg_object_quark_iterate_copy, mem, ret);
		if (retval != Qnil) {
			hg_mem_unlock_object(mem, qdata);
			if (hg_quark_is_readable(qdata))
				acl |= HG_ACL_READABLE;
			if (hg_quark_is_writable(qdata))
				acl |= HG_ACL_WRITABLE;
			if (hg_quark_is_executable(qdata))
				acl |= HG_ACL_EXECUTABLE;
			if (hg_quark_is_accessible(qdata))
				acl |= HG_ACL_ACCESSIBLE;
			hg_quark_set_acl(&retval, acl);
		}
	}

	return retval;
}
