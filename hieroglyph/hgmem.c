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
#include <glib.h>
#include "hgallocator.h"
#include "hgquark.h"
#include "hgtypebit-private.h"
#include "hgmem.h"
#include "hgmem-private.h"

#include "hgmem.proto.h"

#define HG_MAX_MEM	256

static gint __hg_mem_id = 0;
static hg_mem_t *__hg_mem_spool[HG_MAX_MEM];
static hg_mem_t *__hg_mem_master = NULL;

/*< private >*/
HG_INLINE_FUNC void
_hg_mem_gc_init(hg_mem_t *mem)
{
	if (mem->slave_finalizer_table) {
		hg_warning("%s: slave instance already created.", __PRETTY_FUNCTION__);
		g_hash_table_destroy(mem->slave_finalizer_table);
	}
	mem->slave_finalizer_table = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static void
_hg_mem_call_gc_finalizer(gpointer key,
			  gpointer val,
			  gpointer user_data)
{
	hg_quark_t qdata = HGPOINTER_TO_QUARK (key);
	hg_mem_finalizer_func_t func = val;
	hg_mem_t *mem = user_data;

	hg_debug(HG_MSGCAT_GC, "invoking finalizer for %lx", qdata);
	func(mem, qdata);
}

HG_INLINE_FUNC void
_hg_mem_gc_finish(hg_mem_t   *mem,
		  hg_error_t  error)
{
	if (!mem->slave_finalizer_table) {
		hg_warning("%s: no slave instance created.", __PRETTY_FUNCTION__);
		return;
	}
	if (HG_ERROR_IS_SUCCESS (error)) {
		GList *lk, *lv, *llk, *llv;

		/* NOTE: do not use g_hash_foreach().
		 * each finalizers possibly may breaks the condition
		 * of the hash table.
		 */
		lk = g_hash_table_get_keys(mem->finalizer_table);
		lv = g_hash_table_get_values(mem->finalizer_table);
		for (llk = lk, llv = lv;
		     llk != NULL && llv != NULL;
		     llk = g_list_next(llk), llv = g_list_next(llv)) {
			_hg_mem_call_gc_finalizer(llk->data,
						  llv->data,
						  mem);
		}
		g_list_free(lk);
		g_list_free(lv);

		g_hash_table_destroy(mem->finalizer_table);
		mem->finalizer_table = mem->slave_finalizer_table;
	} else {
		g_hash_table_destroy(mem->slave_finalizer_table);
	}
	mem->slave_finalizer_table = NULL;
}

/*< public >*/

/* memory management */
/**
 * hg_mem_get:
 * @id:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_get(gint id)
{
	hg_return_val_if_fail (id < __hg_mem_id, NULL);
	hg_return_val_if_fail (id < HG_MAX_MEM, NULL);

	return __hg_mem_spool[id];
}

/**
 * hg_mem_new:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_new(hg_mem_type_t type,
	   gsize         size)
{
	return hg_mem_new_with_allocator(hg_allocator_get_vtable(), type, size);
}

/**
 * hg_mem_new_with_allocator:
 * @allocator:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_new_with_allocator(hg_mem_vtable_t *allocator,
			  hg_mem_type_t    type,
			  gsize            size)
{
	hg_mem_t *retval;
	gint id;

	hg_return_val_if_fail (allocator != NULL, NULL);
	hg_return_val_if_fail (allocator->initialize != NULL, NULL);
	hg_return_val_if_fail (type < HG_MEM_TYPE_END, NULL);
	if (type == HG_MEM_TYPE_MASTER)
		hg_return_val_if_fail (__hg_mem_master == NULL, NULL);
	hg_return_val_if_fail (size > 0, NULL);
	hg_return_val_if_fail (__hg_mem_id >= 0, NULL);
	hg_return_val_if_fail (__hg_mem_id < HG_MAX_MEM, NULL);

	retval = g_new0(hg_mem_t, 1);
	retval->allocator = allocator;
	retval->type = type;
	id = retval->id = __hg_mem_id++;
	if (_hg_typebit_round_value(retval->id,
				    HG_TYPEBIT_MEM_ID,
				    HG_TYPEBIT_MEM_ID_END) != retval->id) {
		hg_warning("too many memory spooler being created.");
		g_free(retval);

		return NULL;
	}
	retval->data = allocator->initialize();
	if (!retval->data) {
		hg_mem_destroy(retval);
		retval = NULL;
	} else {
		retval->data->resizable = FALSE;
		hg_mem_expand_heap(retval, size);
		retval->finalizer_table = g_hash_table_new(g_direct_hash, g_direct_equal);
		retval->reserved_spool = g_hash_table_new(g_direct_hash, g_direct_equal);
		retval->reference_table = NULL;
	}
	retval->enable_gc = TRUE;
	__hg_mem_spool[id] = retval;
	if (type == HG_MEM_TYPE_MASTER)
		__hg_mem_master = retval;

	return retval;
}

/**
 * hg_mem_destroy:
 * @mem:
 *
 * FIXME
 */
void
hg_mem_destroy(gpointer data)
{
	hg_mem_t *mem;

	if (!data)
		return;

	mem = (hg_mem_t *)data;

	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->finalize != NULL);

	__hg_mem_spool[mem->id] = NULL;
	mem->allocator->finalize(mem->data);
	if (mem->finalizer_table)
		g_hash_table_destroy(mem->finalizer_table);
	if (mem->reserved_spool)
		g_hash_table_destroy(mem->reserved_spool);
	if (mem->type == HG_MEM_TYPE_MASTER)
		__hg_mem_master = NULL;

	g_free(mem);
}

/**
 * hg_mem_expand_heap:
 * @mem:
 * @size:
 *
 * FIXME
 */
gboolean
hg_mem_expand_heap(hg_mem_t  *mem,
		   gsize      size)
{
	hg_return_val_if_fail (mem != NULL, FALSE);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE);
	hg_return_val_if_fail (mem->allocator->expand_heap != NULL, FALSE);
	hg_return_val_if_fail (mem->data != NULL, FALSE);
	hg_return_val_if_fail (size > 0, FALSE);

	return mem->allocator->expand_heap(mem->data, size);
}

/**
 * hg_mem_set_resizable:
 * @mem:
 * @flag:
 *
 * FIXME
 */
void
hg_mem_set_resizable(hg_mem_t *mem,
		     gboolean  flag)
{
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->data != NULL);

	mem->data->resizable = (flag == TRUE);
}

/**
 * hg_mem_add_gc_finalizer:
 * @mem:
 * @index:
 * @func:
 *
 * FIXME
 */
void
hg_mem_add_gc_finalizer(hg_mem_t                *mem,
			hg_quark_t               index,
			hg_mem_finalizer_func_t  func)
{
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (index != Qnil);
	hg_return_if_fail (func != NULL);

	g_hash_table_insert(mem->finalizer_table,
			    HGQUARK_TO_POINTER (hg_quark_get_hash(index)),
			    func);
}

/**
 * hg_mem_remove_gc_finalizer:
 * @mem:
 * @index:
 *
 * FIXME
 */
void
hg_mem_remove_gc_finalizer(hg_mem_t   *mem,
			   hg_quark_t  index)
{
	g_hash_table_remove(mem->finalizer_table,
			    HGQUARK_TO_POINTER (hg_quark_get_hash(index)));
}

hg_mem_type_t
hg_mem_get_type(hg_mem_t *mem)
{
	return mem->type;
}

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
	     gsize     size,
	     gpointer *ret)
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
hg_mem_alloc_with_flags(hg_mem_t *mem,
			gsize     size,
			guint     flags,
			gpointer *ret)
{
	hg_quark_t retval;
	gboolean fgc = FALSE, fheap = FALSE;
	gsize hsize;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator->alloc != NULL, Qnil);
	hg_return_val_if_fail (mem->data != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

  retry:
	retval = mem->allocator->alloc(mem->data, size, flags, ret);
	if (retval != Qnil) {
		hg_quark_set_mem_id(&retval, mem->id);
	} else {
		if (!fgc) {
			if (hg_mem_collect_garbage(mem) > 0) {
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
		hg_warning("Out of memory");
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
hg_mem_realloc(hg_mem_t   *mem,
	       hg_quark_t  qdata,
	       gsize       size,
	       gpointer   *ret)
{
	hg_quark_t retval;
	gboolean fgc = FALSE, fheap = FALSE;
	gsize hsize;

	if (qdata == Qnil)
		return hg_mem_alloc(mem, size, ret);

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator->realloc != NULL, Qnil);
	hg_return_val_if_fail (mem->data != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);
	hg_return_val_if_fail (hg_quark_has_mem_id(qdata, mem->id), Qnil);

  retry:
	retval = mem->allocator->realloc(mem->data,
					 hg_quark_get_value(qdata),
					 size,
					 ret);
	if (retval != Qnil) {
		retval = _hg_typebit_get_value_(qdata, HG_TYPEBIT_BIT0, HG_TYPEBIT_END) | retval;
	} else {
		if (!fgc) {
			if (hg_mem_collect_garbage(mem) > 0) {
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
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->free != NULL);
	hg_return_if_fail (mem->data != NULL);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id));

	mem->allocator->free(mem->data,
			     hg_quark_get_value (qdata));
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
gpointer
hg_mem_lock_object(hg_mem_t   *mem,
		   hg_quark_t  qdata)
{
	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (mem->allocator != NULL, NULL);
	hg_return_val_if_fail (mem->allocator->lock_object != NULL, NULL);
	hg_return_val_if_fail (mem->data != NULL, NULL);

	if (qdata == Qnil)
		return NULL;

	hg_return_val_if_fail (hg_quark_has_mem_id(qdata, mem->id), NULL);

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
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->unlock_object != NULL);
	hg_return_if_fail (mem->data != NULL);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id));

	return mem->allocator->unlock_object(mem->data,
					     hg_quark_get_value (qdata));
}

/**
 * hg_mem_enable_garbage_collector:
 * @mem:
 * @flag:
 *
 * FIXME
 */
void
hg_mem_enable_garbage_collector(hg_mem_t *mem,
				gboolean  flag)
{
	hg_return_if_fail (mem != NULL);

	mem->enable_gc = (flag == TRUE);
}

/**
 * hg_mem_set_garbage_collector:
 * @mem:
 * @func:
 * @data:
 *
 * FIXME
 */
void
hg_mem_set_garbage_collector(hg_mem_t     *mem,
			     hg_gc_func_t  func,
			     gpointer      user_data)
{
	hg_return_if_fail (mem != NULL);

	mem->gc_func = func;
	mem->gc_data = user_data;
}

/**
 * hg_mem_collect_garbage:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
gssize
hg_mem_collect_garbage(hg_mem_t *mem)
{
	gssize retval;

	hg_return_val_if_fail (mem != NULL, -1);
	hg_return_val_if_fail (mem->allocator != NULL, -1);
	hg_return_val_if_fail (mem->data != NULL, -1);

	retval = mem->data->used_size;

	if (mem->allocator->gc_init &&
	    mem->allocator->gc_mark &&
	    mem->allocator->gc_finish &&
	    mem->enable_gc) {
		if (mem->allocator->gc_init(mem->data)) {
			hg_error_t error = 0;

			hg_debug(HG_MSGCAT_GC, "starting [mem_id: %d]", mem->id);
			_hg_mem_gc_init(mem);
			if (mem->gc_func)
				error = mem->gc_func(mem, mem->gc_data);
			if (HG_ERROR_IS_SUCCESS (error) &&
			    mem->rs_gc_func) {
				error = hg_mem_reserved_spool_foreach(mem,
								      mem->rs_gc_func,
								      mem->rs_gc_data);
			}
			_hg_mem_gc_finish(mem, error);
			error = mem->allocator->gc_finish(mem->data, error);
			if (!HG_ERROR_IS_SUCCESS (error))
				return -1;
		}
	}

	return retval - mem->data->used_size;
}

/**
 * hg_mem_gc_mark:
 * @mem:
 * @qdata:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_error_t
hg_mem_gc_mark(hg_mem_t    *mem,
	       hg_quark_t   qdata)
{
	hg_mem_finalizer_func_t func;

	hg_return_val_if_fail (mem != NULL, HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror));
	hg_return_val_if_fail (mem->allocator != NULL, HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror));
	hg_return_val_if_fail (mem->allocator->gc_mark != NULL, HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror));
	hg_return_val_if_fail (mem->data != NULL, HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror));

	if (qdata == Qnil)
		return HG_ERROR_ (HG_STATUS_SUCCESS, 0);

	hg_return_val_if_fail (hg_quark_has_mem_id(qdata, mem->id), HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror));

	if (mem->slave_finalizer_table &&
	    (func = g_hash_table_lookup(mem->finalizer_table,
					HGQUARK_TO_POINTER (hg_quark_get_hash(qdata))))) {
		g_hash_table_insert(mem->slave_finalizer_table,
				    HGQUARK_TO_POINTER (hg_quark_get_hash(qdata)),
				    func);
		g_hash_table_remove(mem->finalizer_table,
				    HGQUARK_TO_POINTER (hg_quark_get_hash(qdata)));
	}

	return mem->allocator->gc_mark(mem->data,
				       hg_quark_get_value(qdata));
}

/**
 * hg_mem_save_snapshot:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_snapshot_data_t *
hg_mem_save_snapshot(hg_mem_t *mem)
{
	hg_mem_snapshot_data_t *retval;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (mem->allocator != NULL, NULL);
	hg_return_val_if_fail (mem->allocator->save_snapshot != NULL, NULL);
	hg_return_val_if_fail (mem->data != NULL, NULL);

	/* clean up to obtain the certain information to be restored */
	if (hg_mem_collect_garbage(mem) < 0)
		return NULL;

	retval = mem->allocator->save_snapshot(mem->data);
	if (retval) {
		retval->total_size = mem->data->total_size;
		retval->used_size = mem->data->used_size;
	}

	return retval;
}

/**
 * hg_mem_restore_snapshot:
 * @mem:
 * @snapshot:
 * @func:
 * @data:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_mem_restore_snapshot(hg_mem_t               *mem,
			hg_mem_snapshot_data_t *snapshot,
			hg_gc_func_t            func,
			gpointer                data)
{
	gboolean retval = FALSE;

	hg_return_val_if_fail (mem != NULL, FALSE);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE);
	hg_return_val_if_fail (mem->allocator->restore_snapshot != NULL, FALSE);
	hg_return_val_if_fail (mem->data != NULL, FALSE);
	hg_return_val_if_fail (mem->reference_table == NULL, FALSE);

	mem->reference_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	if (!func(mem, data))
		goto finalize;

	retval = mem->allocator->restore_snapshot(mem->data,
						  snapshot,
						  mem->reference_table);

  finalize:
	g_hash_table_destroy(mem->reference_table);
	mem->reference_table = NULL;

	return retval;
}

/**
 * hg_mem_restore_mark:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_restore_mark(hg_mem_t    *mem,
		    hg_quark_t   qdata)
{
	hg_return_if_fail (mem != NULL);

	if (qdata == Qnil)
		return;
	if (mem->reference_table == NULL)
		return;

	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id));

	g_hash_table_insert(mem->reference_table,
			    HGQUARK_TO_POINTER (hg_quark_get_value(qdata)),
			    HGQUARK_TO_POINTER (hg_quark_get_value(qdata)));
}

/**
 * hg_mem_snapshot_free:
 * @mem:
 * @snapshot:
 *
 * FIXME
 */
void
hg_mem_snapshot_free(hg_mem_t               *mem,
		     hg_mem_snapshot_data_t *snapshot)
{
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->destroy_snapshot != NULL);
	hg_return_if_fail (mem->data != NULL);

	mem->allocator->destroy_snapshot(mem->data, snapshot);
}

/**
 * hg_mem_get_id:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
gint
hg_mem_get_id(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, -1);

	return mem->id;
}

/**
 * hg_mem_get_total_size:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_mem_get_total_size(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, 0);
	hg_return_val_if_fail (mem->data != NULL, 0);

	return mem->data->total_size;
}

/**
 * hg_mem_get_used_size:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_mem_get_used_size(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, 0);
	hg_return_val_if_fail (mem->data != NULL, 0);

	return mem->data->used_size;
}

/**
 * hg_mem_reserved_spool_add:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_reserved_spool_add(hg_mem_t     *mem,
			  hg_quark_t    qdata)
{
	gpointer p;
	gint count;

	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (qdata != Qnil);
	hg_return_if_fail (hg_quark_has_mem_id(qdata, mem->id));

	p = HGQUARK_TO_POINTER (hg_quark_get_hash(qdata));
	count = GPOINTER_TO_INT (g_hash_table_lookup(mem->reserved_spool, p));
	if ((count + 1) < 0)
		hg_warning("[BUG] the reference count of %lx on the reserved_spool being overflowed",
			   qdata);
	g_hash_table_replace(mem->reserved_spool, p, GINT_TO_POINTER (count + 1));
}

/**
 * hg_mem_reserved_spool_remove:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_reserved_spool_remove(hg_mem_t   *mem,
			     hg_quark_t  qdata)
{
	gpointer p;
	gint count;

	hg_return_if_fail (mem != NULL);

	if (qdata == Qnil)
		return;

	p = HGQUARK_TO_POINTER (hg_quark_get_hash(qdata));
	count = GPOINTER_TO_INT (g_hash_table_lookup(mem->reserved_spool, p));
	if (count > 0) {
		if (count == 1) {
			g_hash_table_remove(mem->reserved_spool, p);
		} else {
			g_hash_table_replace(mem->reserved_spool, p, GINT_TO_POINTER (count - 1));
		}
	}
}

/**
 * hg_mem_reserved_spool_set_garbage_collector:
 * @mem:
 * @func:
 * @user_data:
 *
 * FIXME
 */
void
hg_mem_reserved_spool_set_garbage_collector(hg_mem_t        *mem,
					    hg_rs_gc_func_t  func,
					    gpointer         user_data)
{
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (func != NULL);

	mem->rs_gc_func = func;
	mem->rs_gc_data = user_data;
}

/**
 * hg_mem_reserved_spool_foreach:
 * @mem:
 * @func:
 * @user_data:
 *
 * FIXME
 *
 * Returns:
 */
hg_error_t
hg_mem_reserved_spool_foreach(hg_mem_t         *mem,
			      hg_rs_gc_func_t   func,
			      hg_pointer_t      user_data)
{
	GList *lk, *lv, *llk, *llv;
	hg_error_t error = 0;

	hg_return_val_if_fail (mem != NULL, HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror));

	lk = g_hash_table_get_keys(mem->reserved_spool);
	lv = g_hash_table_get_values(mem->reserved_spool);

	for (llk = lk, llv = lv;
	     llk != NULL && llv != NULL;
	     llk = g_list_next(llk), llv = g_list_next(llv)) {
		error = func(mem,
			     HGPOINTER_TO_QUARK (llk->data),
			     HGPOINTER_TO_QUARK (llv->data),
			     user_data);
		if (!HG_ERROR_IS_SUCCESS (error))
			break;
	}

	g_list_free(lk);
	g_list_free(lv);

	return error;
}
