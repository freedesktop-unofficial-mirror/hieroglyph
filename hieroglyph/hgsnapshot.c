/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgsnapshot.c
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

#include "hgmem.h"
#include "hgsnapshot.h"


struct _hg_snapshot_t {
	hg_object_t             o;
	hg_mem_snapshot_data_t *snapshot;
};

HG_DEFINE_VTABLE (snapshot);

/*< private >*/
static gsize
_hg_object_snapshot_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_snapshot_t));
}

static guint
_hg_object_snapshot_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE;
}

static gboolean
_hg_object_snapshot_initialize(hg_object_t *object,
			       va_list      args)
{
	return TRUE;
}

static void
_hg_object_snapshot_free(hg_object_t *object)
{
	hg_snapshot_t *snapshot = (hg_snapshot_t *)object;

	hg_return_if_fail (object->type == HG_TYPE_SNAPSHOT);

	if (snapshot->snapshot)
		hg_mem_snapshot_free(snapshot->o.mem,
				     snapshot->snapshot);
}

static hg_quark_t
_hg_object_snapshot_copy(hg_object_t              *object,
			 hg_quark_iterate_func_t   func,
			 gpointer                  user_data,
			 gpointer                 *ret,
			 GError                  **error)
{
	hg_return_val_if_fail (object->type == HG_TYPE_SNAPSHOT, Qnil);

	return object->self;
}

static gchar *
_hg_object_snapshot_to_cstr(hg_object_t              *object,
			    hg_quark_iterate_func_t   func,
			    gpointer                  user_data,
			    GError                  **error)
{
	hg_return_val_if_fail (object->type == HG_TYPE_SNAPSHOT, NULL);

	return g_strdup("-save-");
}

static gboolean
_hg_object_snapshot_gc_mark(hg_object_t           *object,
			    hg_gc_iterate_func_t   func,
			    gpointer               user_data,
			    GError               **error)
{
	hg_return_val_if_fail (object->type == HG_TYPE_SNAPSHOT, FALSE);

	return TRUE;
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
hg_snapshot_new(hg_mem_t *mem,
		gpointer *ret)
{
	hg_quark_t retval;
	hg_snapshot_t *s = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil);

	retval = hg_object_new(mem, (gpointer *)&s, HG_TYPE_SNAPSHOT, 0);
	if (retval != Qnil) {
		s->snapshot = hg_mem_save_snapshot(mem);
		if (s->snapshot == NULL) {
			hg_object_free(mem, retval);
			return Qnil;
		}

		if (ret)
			*ret = s;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_snapshot_restore:
 * @snapshot:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_snapshot_restore(hg_snapshot_t *snapshot)
{
	gboolean retval;

	hg_return_val_if_fail (snapshot != NULL, FALSE);
	hg_return_val_if_fail (snapshot->snapshot != NULL, FALSE);

	retval = hg_mem_restore_snapshot(snapshot->o.mem,
					 snapshot->snapshot);
	if (retval)
		snapshot->snapshot = NULL;

	return retval;
}
