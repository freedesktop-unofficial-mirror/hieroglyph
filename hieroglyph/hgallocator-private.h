/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-private.h
 * Copyright (C) 2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGALLOCATOR_PRIVATE_H__
#define __HIEROGLYPH_HGALLOCATOR_PRIVATE_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

#define BLOCK_SIZE		32

#define hg_get_allocated_object(x)		\
	(gpointer)((gchar *)(x) + hg_mem_aligned_size(sizeof (hg_allocator_block_t)))
#define hg_get_allocator_block(x)		\
	(hg_allocator_block_t *)((gchar *)(x) - hg_mem_aligned_size(sizeof (hg_allocator_block_t)))


struct _hg_allocator_bitmap_t {
	guint32    *bitmaps;
	gsize       size;
	gsize       last_allocated_size;
	hg_quark_t  last_index;
};
struct _hg_allocator_block_t {
	gsize          size;
	volatile guint lock_count;
	gint           age;
	gboolean       is_restorable:1;
	gboolean       drop_on_restore:1;
};
struct _hg_allocator_private_t {
	hg_allocator_data_t    parent;
	hg_allocator_data_t    slave;
	hg_allocator_bitmap_t *bitmap;
	hg_allocator_bitmap_t *slave_bitmap;
	gpointer               heap;
	gint                   snapshot_age;
};
struct _hg_allocator_snapshot_private_t {
	hg_mem_snapshot_data_t  parent;
	hg_allocator_bitmap_t  *bitmap;
	gpointer                heap;
	gint                    age;
};

G_END_DECLS

#endif /* __HIEROGLYPH_HGALLOCATOR_PRIVATE_H__ */
