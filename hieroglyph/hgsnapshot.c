/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgsnapshot.c
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

/* XXX: GLib is needed for GHashTable */
#include <glib.h>
#include "hgmem.h"
#include "hgmem-private.h"
#include "hgsnapshot.h"

#include "hgsnapshot.proto.h"

struct _hg_snapshot_t {
	hg_object_t             o;
	hg_mem_snapshot_data_t *snapshot;
	GHashTable             *ref_table;
};

HG_DEFINE_VTABLE (snapshot);

/*< private >*/
static hg_usize_t
_hg_object_snapshot_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_snapshot_t));
}

static hg_uint_t
_hg_object_snapshot_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE;
}

static hg_bool_t
_hg_object_snapshot_initialize(hg_object_t *object,
			       va_list      args)
{
	return TRUE;
}

static void
_hg_object_snapshot_free(hg_object_t *object)
{
	hg_snapshot_t *snapshot = (hg_snapshot_t *)object;
	hg_mem_t *mem;

	hg_return_if_fail (object->type == HG_TYPE_SNAPSHOT, HG_e_typecheck);
	hg_return_if_fail (snapshot->o.mem != NULL, HG_e_VMerror);

	mem = snapshot->o.mem;

	hg_return_if_fail (mem->allocator != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->allocator->destroy_snapshot != NULL, HG_e_VMerror);
	hg_return_if_fail (mem->data != NULL, HG_e_VMerror);

	if (snapshot->snapshot) {
		mem->allocator->destroy_snapshot(mem->data,
						 snapshot->snapshot);
	}
}

static hg_quark_t
_hg_object_snapshot_copy(hg_object_t             *object,
			 hg_quark_iterate_func_t  func,
			 hg_pointer_t             user_data,
			 hg_pointer_t            *ret)
{
	hg_return_val_if_fail (object->type == HG_TYPE_SNAPSHOT, Qnil, HG_e_typecheck);

	return object->self;
}

static hg_char_t *
_hg_object_snapshot_to_cstr(hg_object_t             *object,
			    hg_quark_iterate_func_t  func,
			    hg_pointer_t             user_data)
{
	hg_return_val_if_fail (object->type == HG_TYPE_SNAPSHOT, NULL, HG_e_typecheck);

	return g_strdup("-save-");
}

static hg_bool_t
_hg_object_snapshot_gc_mark(hg_object_t *object)
{
	hg_return_val_if_fail (object->type == HG_TYPE_SNAPSHOT, FALSE, HG_e_typecheck);

	return TRUE;
}

static hg_bool_t
_hg_object_snapshot_compare(hg_object_t             *o1,
			    hg_object_t             *o2,
			    hg_quark_compare_func_t  func,
			    hg_pointer_t             user_data)
{
	hg_return_val_if_fail (o1->type == HG_TYPE_SNAPSHOT, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (o2->type == HG_TYPE_SNAPSHOT, FALSE, HG_e_typecheck);

	return o1->self == o2->self;
}

/*< public >*/
/**
 * hg_snapshot_new:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_snapshot_new(hg_mem_t     *mem,
		hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_snapshot_t *s = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);

	retval = hg_object_new(mem, (hg_pointer_t *)&s, HG_TYPE_SNAPSHOT, 0);
	if (retval != Qnil) {
		if (ret)
			*ret = s;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_snapshot_save:
 * @snapshot:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_snapshot_save(hg_snapshot_t *snapshot)
{
	hg_mem_t *mem;

	hg_return_val_if_fail (snapshot != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (snapshot->o.mem != NULL, FALSE, HG_e_VMerror);

	mem = snapshot->o.mem;

	hg_return_val_if_fail (mem != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->save_snapshot != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, FALSE, HG_e_VMerror);

	/* clean up to obtain the certain information to be restored */
	if (hg_mem_spool_run_gc(mem) < 0)
		return FALSE;

	snapshot->snapshot = mem->allocator->save_snapshot(mem->data);
	if (!snapshot->snapshot) {
		snapshot->snapshot->total_size = hg_mem_spool_get_total_size(mem);
		snapshot->snapshot->used_size = hg_mem_spool_get_used_size(mem);
	}

	return snapshot->snapshot ? TRUE : FALSE;
}

/**
 * hg_snapshot_restore:
 * @snapshot:
 * @func:
 * @data:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_snapshot_restore(hg_snapshot_t          *snapshot,
		    hg_restore_prep_func_t  func,
		    hg_pointer_t            data)
{
	hg_bool_t retval = FALSE;
	hg_mem_t *mem;
	static volatile hg_int_t is_running = 0;
	hg_int_t old_val;

	hg_return_val_if_fail (snapshot != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (snapshot->snapshot != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (snapshot->o.mem != NULL, FALSE, HG_e_VMerror);

	mem = snapshot->o.mem;

	hg_return_val_if_fail (mem->allocator != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->allocator->restore_snapshot != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->data != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (mem->reference_table == NULL, FALSE, HG_e_VMerror);

  retry:
	old_val = g_atomic_int_get(&is_running);
	if (old_val > 0) {
		hg_critical("Unable to restore the snapshot during restoring");
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	if (!g_atomic_int_compare_and_exchange(&is_running, old_val, old_val + 1))
		goto retry;

	hg_mem_spool_run_gc(snapshot->o.mem);

	snapshot->ref_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	if (!func(snapshot, data))
		goto finalize;

	retval = mem->allocator->restore_snapshot(mem->data,
						  snapshot->snapshot,
						  snapshot->ref_table);
	if (retval) {
		snapshot->snapshot = NULL;
	}
  finalize:
	g_hash_table_destroy(snapshot->ref_table);
	snapshot->ref_table = NULL;

  retry2:
	old_val = g_atomic_int_get(&is_running);
	if (!g_atomic_int_compare_and_exchange(&is_running, old_val, old_val - 1))
		goto retry2;

	return retval;
}

/**
 * hg_snapshot_add_ref:
 * @snapshot:
 * @qdata:
 *
 * FIXME
 */
void
hg_snapshot_add_ref(hg_snapshot_t *snapshot,
		    hg_quark_t     qdata)
{
	hg_int_t id;

	hg_return_if_fail (snapshot != NULL, HG_e_typecheck);
	hg_return_if_fail (snapshot->o.type == HG_TYPE_SNAPSHOT, HG_e_typecheck);

	if (qdata == Qnil ||
	    snapshot->ref_table == NULL)
		return;

	id = hg_mem_get_id(snapshot->o.mem);
	hg_return_if_fail (hg_quark_has_mem_id(qdata, id), HG_e_VMerror);

	g_hash_table_insert(snapshot->ref_table,
			    HGQUARK_TO_POINTER (hg_quark_get_value(qdata)),
			    HGQUARK_TO_POINTER (hg_quark_get_value(qdata)));
}
