/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.c
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
#endif /* HAVE_CONFIG_H */

#include <math.h>
#include "hgallocator.h"
#include "hgquark.h"
#include "hgtypebit-private.h"
#include "hgmem.h"
#include "hgmem-private.h"

#include "hgmem.proto.h"

#define HG_MAX_MEM	256

static hg_int_t __hg_mem_id = 0;
static hg_mem_t *__hg_mem_spool[HG_MAX_MEM];
static hg_mem_t *__hg_mem_master = NULL;

/*< private >*/

/*< public >*/

/** memory spool management **/

/**
 * hg_mem_get_spool:
 * @id:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_spool_get(hg_int_t id)
{
	hg_return_val_if_fail (id < __hg_mem_id, NULL, HG_e_VMerror);
	hg_return_val_if_fail (id < HG_MAX_MEM, NULL, HG_e_VMerror);

	hg_error_return_val (__hg_mem_spool[id], HG_STATUS_SUCCESS, 0);
}

/**
 * hg_mem_spool_new:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_spool_new(hg_mem_type_t type,
		 hg_usize_t    size)
{
	return hg_mem_spool_new_with_allocator(hg_allocator_get_vtable(), type, size);
}

/**
 * hg_mem_spool_new_with_allocator:
 * @allocator:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_spool_new_with_allocator(hg_mem_vtable_t *allocator,
				hg_mem_type_t    type,
				hg_usize_t       size)
{
	hg_mem_t *retval;
	hg_int_t id;

	hg_return_val_if_fail (allocator != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (allocator->initialize != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (type < HG_MEM_TYPE_END, NULL, HG_e_VMerror);
	if (type == HG_MEM_TYPE_MASTER)
		hg_return_val_if_fail (__hg_mem_master == NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (size > 0, NULL, HG_e_VMerror);
	hg_return_val_if_fail (__hg_mem_id >= 0, NULL, HG_e_VMerror);
	hg_return_val_if_fail (__hg_mem_id < HG_MAX_MEM, NULL, HG_e_VMerror);

	retval = g_new0(hg_mem_t, 1);
	retval->allocator = allocator;
	retval->type = type;
	id = retval->id = __hg_mem_id++;
	if (_hg_typebit_round_value(retval->id,
				    HG_TYPEBIT_MEM_ID,
				    HG_TYPEBIT_MEM_ID_END) != retval->id) {
		hg_warning("too many memory spooler being created.");
		g_free(retval);

		hg_error_return_val (NULL, HG_STATUS_FAILED, HG_e_VMerror);
	}
	retval->data = allocator->initialize();
	if (!retval->data) {
		hg_mem_spool_destroy(retval);
		retval = NULL;
	} else {
		retval->data->resizable = FALSE;
		hg_mem_spool_expand_heap(retval, size);
		retval->reference_table = NULL;
	}
	retval->enable_gc = TRUE;
	__hg_mem_spool[id] = retval;
	if (type == HG_MEM_TYPE_MASTER)
		__hg_mem_master = retval;

	return retval;
}

/**
 * hg_mem_spool_destroy:
 * @mem:
 *
 * FIXME
 */
void
hg_mem_spool_destroy(hg_pointer_t data)
{
	hg_mem_t *mem;

	if (!data)
		return;

	mem = (hg_mem_t *)data;

	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->finalize != NULL, HG_e_VMerror);

	__hg_mem_spool[mem->id] = NULL;
	mem->allocator->finalize(mem->data);
	if (mem->type == HG_MEM_TYPE_MASTER)
		__hg_mem_master = NULL;

	g_free(mem);
}

/**
 * hg_mem_spool_expand_heap:
 * @mem:
 * @size:
 *
 * FIXME
 */
hg_bool_t
hg_mem_spool_expand_heap(hg_mem_t   *mem,
			 hg_usize_t  size)
{
	hg_return_val_if_fail (mem != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->expand_heap != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (size > 0, FALSE, HG_e_VMerror);

	return mem->allocator->expand_heap(mem->data, size);
}

/**
 * hg_mem_spool_set_resizable:
 * @mem:
 * @flag:
 *
 * FIXME
 */
void
hg_mem_spool_set_resizable(hg_mem_t  *mem,
			   hg_bool_t  flag)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	mem->data->resizable = (flag == TRUE);
}

/**
 * hg_mem_spool_add_gc_marker:
 * @mem:
 * @func:
 *
 * FIXME
 *
 * Returns:
 */
hg_int_t
hg_mem_spool_add_gc_marker(hg_mem_t          *mem,
			   hg_gc_mark_func_t  func)
{
	hg_return_val_if_fail (mem != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->add_gc_marker != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, -1, HG_e_VMerror);

	return mem->allocator->add_gc_marker(mem->data, func);
}

/**
 * hg_mem_spool_remove_gc_marker:
 * @mem:
 * @marker_id:
 *
 * FIXME
 */
void
hg_mem_spool_remove_gc_marker(hg_mem_t   *mem,
			      hg_int_t    marker_id)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->remove_gc_marker != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);
	hg_return_if_fail (marker_id >= 0, HG_e_VMerror);

	return mem->allocator->remove_gc_marker(mem->data, marker_id);
}

/**
 * hg_mem_spool_add_finalizer:
 * @mem:
 * @func:
 *
 * FIXME
 *
 * Returns:
 */
hg_int_t
hg_mem_spool_add_finalizer(hg_mem_t            *mem,
			   hg_finalizer_func_t  func)
{
	hg_return_val_if_fail (mem != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->add_finalizer != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, -1, HG_e_VMerror);

	return mem->allocator->add_finalizer(mem->data, func);
}

/**
 * hg_mem_spool_remove_finalizer:
 * @mem:
 * @index:
 *
 * FIXME
 */
void
hg_mem_spool_remove_finalizer(hg_mem_t   *mem,
			      hg_int_t    finalizer_id)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->remove_finalizer != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);
	hg_return_if_fail (finalizer_id >= 0, HG_e_VMerror);

	return mem->allocator->remove_finalizer(mem->data, finalizer_id);
}

hg_mem_type_t
hg_mem_spool_get_type(hg_mem_t *mem)
{
	return mem->type;
}

/**
 * hg_mem_spool_enable_gc:
 * @mem:
 * @flag:
 *
 * FIXME
 */
void
hg_mem_spool_enable_gc(hg_mem_t  *mem,
		       hg_bool_t  flag)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);

	mem->enable_gc = (flag == TRUE);
}

/**
 * hg_mem_spool_set_gc_procedure:
 * @mem:
 * @func:
 * @data:
 *
 * FIXME
 */
void
hg_mem_spool_set_gc_procedure(hg_mem_t     *mem,
			      hg_gc_func_t  func,
			      hg_pointer_t  user_data)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);

	mem->gc_func = func;
	mem->gc_data = user_data;
}

/**
 * hg_mem_spool_run_gc:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_mem_spool_run_gc(hg_mem_t *mem)
{
	hg_size_t retval;

	hg_return_val_if_fail (mem != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, -1, HG_e_VMerror);

	retval = mem->data->used_size;

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	if (mem->allocator->gc_init &&
	    mem->allocator->gc_mark &&
	    mem->allocator->gc_finish &&
	    mem->enable_gc) {
		if (mem->allocator->gc_init(mem->data)) {
			hg_debug(HG_MSGCAT_GC, "starting [mem_id: %d]", mem->id);
			if (mem->gc_func)
				mem->gc_func(mem, mem->gc_data);
			if (!mem->allocator->gc_finish(mem->data))
				return -1;
		}
	}

	return retval - mem->data->used_size;
}

/**
 * hg_mem_spool_get_total_size:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_usize_t
hg_mem_spool_get_total_size(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, 0, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, 0, HG_e_VMerror);

	return mem->data->total_size;
}

/**
 * hg_mem_spool_get_used_size:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_usize_t
hg_mem_spool_get_used_size(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, 0, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, 0, HG_e_VMerror);

	return mem->data->used_size;
}

/** memory management **/

/**
 * hg_mem_alloc:
 * @mem:
 * @size:
 * @ret:
 *
 * FIXME:
 *
 * Returns:
 */
hg_quark_t
hg_mem_alloc(hg_mem_t *mem,
	     hg_usize_t     size,
	     hg_pointer_t *ret)
{
	return hg_mem_alloc_with_flags(mem, size,
				       HG_MEM_FLAGS_DEFAULT,
				       ret);
}

/**
 * hg_mem_alloc_with_flags:
 * @mem:
 * @size:
 * @flags:
 * @ret:
 *
 * FIXME:
 *
 * Returns:
 */
hg_quark_t
hg_mem_alloc_with_flags(hg_mem_t     *mem,
			hg_usize_t    size,
			hg_uint_t     flags,
			hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_bool_t fgc = FALSE, fheap = FALSE;
	hg_usize_t hsize;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->alloc != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (size > 0, Qnil, HG_e_VMerror);

  retry:
	retval = mem->allocator->alloc(mem->data, size, flags, ret);
	if (retval != Qnil) {
		hg_quark_set_mem_id(&retval, mem->id);
	} else {
		if (!fgc) {
			if (hg_mem_spool_run_gc(mem) > 0) {
				fgc = TRUE;
				goto retry;
			}
		}
		if (!fheap &&
		    mem->data->resizable &&
		    mem->allocator->expand_heap &&
		    mem->allocator->get_max_heap_size) {
			hsize = mem->allocator->get_max_heap_size(mem->data);
			if (mem->allocator->expand_heap(mem->data, MAX (hsize, size))) {
				fheap = TRUE;
				goto retry;
			}
		}
		hg_debug(HG_MSGCAT_MEM, "Out of memory");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	return retval;
}

/**
 * hg_mem_realloc:
 * @mem:
 * @qdata:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_mem_realloc(hg_mem_t     *mem,
	       hg_quark_t    qdata,
	       hg_usize_t    size,
	       hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_bool_t fgc = FALSE, fheap = FALSE;
	hg_usize_t hsize;

	if (qdata == Qnil)
		return hg_mem_alloc(mem, size, ret);

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->realloc != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (size > 0, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (hg_quark_has_mem_id(qdata, mem->id), Qnil, HG_e_VMerror);

  retry:
	retval = mem->allocator->realloc(mem->data,
					 hg_quark_get_value(qdata),
					 size,
					 ret);
	if (retval != Qnil) {
		retval = _hg_typebit_get_value_(qdata, HG_TYPEBIT_BIT0, HG_TYPEBIT_END) | retval;
	} else {
		if (!fgc) {
			if (hg_mem_spool_run_gc(mem) > 0) {
				fgc = TRUE;
				goto retry;
			}
		}
		if (!fheap &&
		    mem->data->resizable &&
		    mem->allocator->expand_heap &&
		    mem->allocator->get_max_heap_size) {
			hsize = mem->allocator->get_max_heap_size(mem->data);
			if (mem->allocator->expand_heap(mem->data, MAX (hsize, size))) {
				fheap = TRUE;
				goto retry;
			}
		}
		hg_debug(HG_MSGCAT_MEM, "Out of memory");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	return retval;
}

/**
 * hg_mem_free:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_free(hg_mem_t   *mem,
	    hg_quark_t  qdata)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->free != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id), HG_e_VMerror);

	mem->allocator->free(mem->data,
			     hg_quark_get_value (qdata));
}

/**
 * hg_mem_set_gc_marker:
 * @mem:
 * @qdata:
 * @marker_id:
 *
 * FIXME
 */
void
hg_mem_set_gc_marker(hg_mem_t   *mem,
		     hg_quark_t  qdata,
		     hg_int_t    marker_id)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->set_gc_marker != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id), HG_e_VMerror);

	mem->allocator->set_gc_marker(mem->data,
				      hg_quark_get_value (qdata),
				      marker_id);
}

/**
 * hg_mem_set_finalizer:
 * @mem:
 * @qdata:
 * @finalizer_id:
 *
 * FIXME
 */
void
hg_mem_set_finalizer(hg_mem_t   *mem,
		     hg_quark_t  qdata,
		     hg_int_t    finalizer_id)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->set_finalizer != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id), HG_e_VMerror);

	mem->allocator->set_finalizer(mem->data,
				      hg_quark_get_value (qdata),
				      finalizer_id);
}

/**
 * hg_mem_lock_object:
 * @mem:
 * @qdata:
 *
 * FIXME
 *
 * Return:
 */
hg_pointer_t
hg_mem_lock_object(hg_mem_t   *mem,
		   hg_quark_t  qdata)
{
	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->lock_object != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, NULL, HG_e_VMerror);

	if (qdata == Qnil)
		return NULL;

	hg_return_val_if_fail (hg_quark_has_mem_id(qdata, mem->id), NULL, HG_e_VMerror);

	return mem->allocator->lock_object(mem->data,
					   hg_quark_get_value(qdata));
}

/**
 * hg_mem_unlock_object:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_unlock_object(hg_mem_t   *mem,
		     hg_quark_t  qdata)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->unlock_object != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id), HG_e_VMerror);

	return mem->allocator->unlock_object(mem->data,
					     hg_quark_get_value (qdata));
}

/**
 * hg_mem_gc_mark:
 * @mem:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_mem_gc_mark(hg_mem_t   *mem,
	       hg_quark_t  qdata)
{
	hg_return_val_if_fail (mem != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->gc_mark != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, FALSE, HG_e_VMerror);

	if (qdata == Qnil)
		return TRUE;

	hg_return_val_if_fail (hg_quark_has_mem_id(qdata, mem->id), FALSE, HG_e_VMerror);

	return mem->allocator->gc_mark(mem->data,
				       hg_quark_get_value(qdata));
}

/**
 * hg_mem_get_id:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_int_t
hg_mem_get_id(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, -1, HG_e_VMerror);

	return mem->id;
}

/**
 * hg_mem_ref:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_ref(hg_mem_t   *mem,
	   hg_quark_t  qdata)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->block_ref != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);
	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id), HG_e_VMerror);

	if (qdata == Qnil ||
	    hg_quark_is_simple_object(qdata) ||
	    hg_quark_get_type(qdata) == HG_TYPE_OPER)
		return;

	mem->allocator->block_ref(mem->data, qdata);
}

/**
 * hg_mem_unref:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_unref(hg_mem_t   *mem,
	     hg_quark_t  qdata)
{
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->block_unref != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	if (qdata == Qnil ||
	    hg_quark_is_simple_object(qdata) ||
	    hg_quark_get_type(qdata) == HG_TYPE_OPER)
		return;

	mem->allocator->block_unref(mem->data, qdata);
}

/**
 * hg_mem_foreach:
 * @mem:
 * @func:
 * @flags:
 * @user_data:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_mem_foreach(hg_mem_t              *mem,
	       hg_block_iter_flags_t  flags,
	       hg_block_iter_func_t   func,
	       hg_pointer_t           user_data)
{
	hg_return_val_if_fail (mem != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->block_foreach != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, FALSE, HG_e_VMerror);

	return mem->allocator->block_foreach(mem->data, flags, func, user_data);
}
