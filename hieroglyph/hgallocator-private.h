/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-private.h
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#ifndef __HG_ALLOCATOR_PRIVATE_H__
#define __HG_ALLOCATOR_PRIVATE_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS

struct _HieroGlyphHeap {
	gpointer heaps;
	gint     serial;
	gsize    total_heap_size;
	gsize    used_heap_size;
};

struct _HieroGlyphMemPool {
	gchar       *name;
	GPtrArray   *heap_list;
	gint         n_heaps;
	gsize        initial_heap_size;
	gsize        total_heap_size;
	gsize        used_heap_size;
	HgAllocator *allocator;
	GList       *root_node;
	GList       *other_pool_ref_list;
	guint        access_mode;
	gshort       gc_threshold;
	guint8       age_of_gc_mark;
	gboolean     allow_resize : 1;
	gboolean     destroyed : 1;
	gboolean     periodical_gc : 1;
	gboolean     gc_checked : 1;
	gboolean     use_gc : 1;
	gboolean     is_global_mode : 1;
	gboolean     is_processing : 1;
	gboolean     is_collecting : 1;
};

struct _HieroGlyphMemSnapshot {
	HgObject   object;
	gsize      id;
	GPtrArray *heap_list;
	gint       n_heaps;
	gpointer   private;
};


extern gpointer _hg_stack_start;
extern gpointer _hg_stack_end;


G_END_DECLS


#endif /* __HG_ALLOCATOR_PRIVATE_H__ */
