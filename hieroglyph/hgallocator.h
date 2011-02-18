/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator.h
 * Copyright (C) 2006-2011 Akira TAGOH
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGALLOCATOR_H__
#define __HIEROGLYPH_HGALLOCATOR_H__

#include <hieroglyph/hgtypes.h>
/* XXX: GLib is still needed to satisfy the GHashTable dependency */
#include <glib.h>

HG_BEGIN_DECLS

typedef struct _hg_allocator_data_t		hg_allocator_data_t;
typedef struct _hg_allocator_snapshot_data_t	hg_allocator_snapshot_data_t;
typedef struct _hg_mem_vtable_t			hg_mem_vtable_t;
typedef enum _hg_mem_flags_t			hg_mem_flags_t;

typedef void (* hg_finalizer_func_t)	(hg_pointer_t object);

/**
 * hg_allocator_data_t:
 * @total_size:
 * @used_size:
 * @resizable:
 *
 * FIXME
 */
struct _hg_allocator_data_t {
	hg_usize_t total_size;
	hg_usize_t used_size;
	hg_bool_t  resizable;
};
/**
 * hg_allocator_snapshot_data_t:
 * @total_size:
 * @used_size:
 *
 * FIXME
 */
struct _hg_allocator_snapshot_data_t {
	hg_usize_t total_size;
	hg_usize_t used_size;
};
/**
 * hg_mem_flags_t:
 * @HG_MEM_RESTORABLE:
 *
 * FIXME
 */
enum _hg_mem_flags_t {
	HG_MEM_FLAG_RESTORABLE    = 1 << 0,
	HG_MEM_FLAG_HAS_FINALIZER = 1 << 1,
	HG_MEM_FLAGS_END
};
/**
 * hg_mem_vtable_t:
 * @initialize:
 * @finalize:
 * @expand_heap:
 * @get_max_heap_size:
 * @alloc:
 * @realloc:
 * @free:
 * @lock_object:
 * @unlock_object:
 * @gc_init:
 * @gc_mark:
 * @gc_finish:
 * @save_snapshot:
 * @restore_snapshot:
 * @destroy_snapshot:
 *
 * FIXME
 */
struct _hg_mem_vtable_t {
	hg_pointer_t                   (* initialize)        (void);
	void                           (* finalize)          (hg_allocator_data_t           *data);
	hg_bool_t                      (* expand_heap)       (hg_allocator_data_t           *data,
							      hg_usize_t                     size);
	hg_usize_t                     (* get_max_heap_size) (hg_allocator_data_t           *data);
	hg_quark_t                     (* alloc)             (hg_allocator_data_t           *data,
							      hg_usize_t                     size,
							      hg_uint_t                      flags,
							      hg_pointer_t                  *ret);
	hg_quark_t                     (* realloc)           (hg_allocator_data_t           *data,
							      hg_quark_t                     quark,
							      hg_usize_t                     size,
							      hg_pointer_t                  *ret);
	void                           (* free)              (hg_allocator_data_t           *data,
							      hg_quark_t                     quark);
	hg_pointer_t                   (* lock_object)       (hg_allocator_data_t           *data,
							      hg_quark_t                     quark);
	void                           (* unlock_object)     (hg_allocator_data_t           *data,
							      hg_quark_t                     quark);
	hg_int_t                       (* add_finalizer)     (hg_allocator_data_t           *data,
							      hg_finalizer_func_t            func);
	void                           (* remove_finalizer)  (hg_allocator_data_t           *data,
							      hg_int_t                       finalizer_id);
	void                           (* set_finalizer)     (hg_allocator_data_t           *data,
							      hg_quark_t                     quark,
							      hg_int_t                       finalizer_id);
	hg_bool_t                      (* gc_init)           (hg_allocator_data_t           *data);
	hg_bool_t                      (* gc_mark)           (hg_allocator_data_t           *data,
							      hg_quark_t                     quark);
	hg_bool_t                      (* gc_finish)         (hg_allocator_data_t           *data);
	hg_allocator_snapshot_data_t * (* save_snapshot)     (hg_allocator_data_t           *data);
	hg_bool_t                      (* restore_snapshot)  (hg_allocator_data_t           *data,
							      hg_allocator_snapshot_data_t  *snapshot,
							      GHashTable                    *references);
	void                           (* destroy_snapshot)  (hg_allocator_data_t           *data,
							      hg_allocator_snapshot_data_t  *snapshot);
};


hg_mem_vtable_t *hg_allocator_get_vtable   (void) G_GNUC_CONST;

HG_END_DECLS

#endif /* __HIEROGLYPH_HGALLOCATOR_H__ */
