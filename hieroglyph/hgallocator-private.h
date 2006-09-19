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


#define HG_MEM_POOL_FLAGS_GET_FLAGS(__value__, __mask__)	\
	((__value__) & (__mask__))
#define HG_MEM_POOL_GET_FLAGS(__pool__, __mask__)	\
	HG_MEM_POOL_FLAGS_GET_FLAGS ((__pool__)->flags, __mask__)
#define HG_MEM_POOL_SET_FLAGS(__pool__, __mask__, __flags__)	\
	(__pool__)->flags = (((__pool__)->flags & ~(__mask__)) | ((__flags__) ? (__mask__) : 0))
#define HG_MEM_POOL_FLAGS_HAS_FLAGS(__value__, __mask__)	\
	(HG_MEM_POOL_FLAGS_GET_FLAGS (__value__, __mask__) ? TRUE : FALSE)
#define HG_MEM_POOL_HAS_FLAGS(__pool__, __mask__)	\
	(HG_MEM_POOL_FLAGS_HAS_FLAGS ((__pool__)->flags, __mask__))


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
	guint        flags;
	gshort       gc_threshold;
	guint8       age_of_gc_mark;
	gboolean     destroyed : 1;
	gboolean     periodical_gc : 1;
	gboolean     gc_checked : 1;
	gboolean     use_gc : 1;
	gboolean     is_processing : 1;
	gboolean     is_collecting : 1;
};

struct _HieroGlyphMemSnapshot {
	HgObject   object;
	gsize      id;
	GPtrArray *heap_list;
	gint       n_heaps;
	gint       age;
	gpointer   private;
};


extern gpointer _hg_stack_start;
extern gpointer _hg_stack_end;


G_END_DECLS


#endif /* __HG_ALLOCATOR_PRIVATE_H__ */
