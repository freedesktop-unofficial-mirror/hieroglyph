/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.h
 * Copyright (C) 2005-2006 Akira TAGOH
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
#ifndef __HG_MEM_H__
#define __HG_MEM_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

/* initializer */
void hg_mem_init_stack_start(gpointer mark);
void hg_mem_init            (void);
void hg_mem_finalize        (void);
void hg_mem_set_stack_end   (gpointer mark);

/* allocator */
HgAllocator *hg_allocator_new    (const HgAllocatorVTable *vtable);
void         hg_allocator_destroy(HgAllocator             *allocator);
HgHeap      *hg_heap_new         (HgMemPool               *pool,
				  gsize                    size);
void         hg_heap_free        (HgHeap                  *heap);

/* memory pool */
#define hg_mem_get_object__inline_nocheck(__data__)			\
	((HgMemObject *)((gsize)(__data__) - sizeof (HgMemObject)))
#define hg_mem_get_object__inline(__data__, __retval__)			\
	G_STMT_START {							\
		(__retval__) = hg_mem_get_object__inline_nocheck(__data__); \
		if ((__retval__)->id != HG_MEM_HEADER)			\
			(__retval__) = NULL;				\
	} G_STMT_END
#define HG_MEMOBJ_GET_HEAP_ID(_obj)		(((_obj)->flags >> 24) & 0xff)
#define HG_MEMOBJ_SET_HEAP_ID(_obj,_id)		((_obj)->flags = HG_MEMOBJ_GET_FLAGS (_obj) | ((_id) << 24))
#define HG_MEMOBJ_GET_FLAGS(_obj)		((_obj)->flags & 0xffffff)
#define HG_MEMOBJ_SET_FLAGS(_obj,_flags)	(_obj)->flags = (HG_MEMOBJ_GET_HEAP_ID(_obj) << 24) | _flags
#define HG_MEMOBJ_INIT_FLAGS(_obj)		(_obj)->flags = 0;


HgMemPool     *hg_mem_pool_new                    (HgAllocator   *allocator,
						   const gchar   *identity,
						   gsize          prealloc,
						   gboolean       allow_resize);
void           hg_mem_pool_destroy                (HgMemPool     *pool);
const gchar   *hg_mem_pool_get_name               (HgMemPool     *pool);
gboolean       hg_mem_pool_allow_resize           (HgMemPool     *pool,
						   gboolean       flag);
gsize          hg_mem_pool_get_used_heap_size     (HgMemPool     *pool);
gsize          hg_mem_pool_get_free_heap_size     (HgMemPool     *pool);
void           hg_mem_pool_add_heap               (HgMemPool     *pool,
						   HgHeap        *heap);
guint          hg_mem_pool_get_default_access_mode(HgMemPool     *pool);
void           hg_mem_pool_set_default_access_mode(HgMemPool     *pool,
						   guint          state);
gboolean       hg_mem_pool_is_global_mode         (HgMemPool     *pool);
void           hg_mem_pool_use_global_mode        (HgMemPool     *pool,
						   gboolean       flag);
gboolean       hg_mem_pool_is_own_object          (HgMemPool     *pool,
						   gpointer       data);
HgMemSnapshot *hg_mem_pool_save_snapshot          (HgMemPool     *pool);
gboolean       hg_mem_pool_restore_snapshot       (HgMemPool     *pool,
						   HgMemSnapshot *snapshot);
gboolean       hg_mem_garbage_collection          (HgMemPool     *pool);
gpointer       hg_mem_alloc                       (HgMemPool     *pool,
						   gsize          size);
gpointer       hg_mem_alloc_with_flags            (HgMemPool     *pool,
						   gsize          size,
						   guint          flags);
gboolean       hg_mem_free                        (gpointer       data);
gpointer       hg_mem_resize                      (gpointer       data,
						   gsize          size);
gsize          hg_mem_get_object_size             (gpointer       data);

/* internal use */
gboolean       _hg_mem_pool_is_own_memobject      (HgMemPool     *pool,
						   HgMemObject   *obj);

/* GC */
#define hg_mem_is_flags__inline(__obj__, __flags__)			\
	((HG_MEMOBJ_GET_FLAGS (__obj__) & (__flags__)) == (__flags__))
#define hg_mem_get_flags__inline(__obj__)				\
	(HG_MEMOBJ_GET_FLAGS (__obj__))
#define hg_mem_set_flags__inline(__obj__, __flags__, __notify__)	\
	G_STMT_START {							\
		HgObject *__hg_mem_hobj__ = (HgObject *)(__obj__)->data; \
		const HgObjectVTable const *__hg_obj_vtable__;		\
									\
		HG_MEMOBJ_SET_FLAGS ((__obj__), (__flags__));		\
		if ((__notify__) &&					\
		    __hg_mem_hobj__->id == HG_OBJECT_ID &&		\
		    (__hg_obj_vtable__ = hg_object_get_vtable(__hg_mem_hobj__)) != NULL && \
		    __hg_obj_vtable__->set_flags) {			\
			guint __hg_mem_flags__ = (__flags__) & (HG_FL_MARK); \
			__hg_obj_vtable__->set_flags(__hg_mem_hobj__, __hg_mem_flags__); \
		}							\
	} G_STMT_END
#define hg_mem_add_flags__inline(__obj__, __flags__, __notify__)	\
	hg_mem_set_flags__inline((__obj__),				\
				 (__flags__) | hg_mem_get_flags__inline(__obj__), \
				 (__notify__))

#define hg_mem_gc_mark(obj)		hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) | HG_FL_MARK, TRUE)
#define hg_mem_gc_unmark(obj)		hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) & ~HG_FL_MARK, FALSE)
#define hg_mem_is_gc_mark(obj)		hg_mem_is_flags__inline(obj, HG_FL_MARK)
#define hg_mem_restorable(obj)		hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) | HG_FL_RESTORABLE, FALSE)
#define hg_mem_unrestorable(obj)	hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) & ~HG_FL_RESTORABLE, FALSE)
#define hg_mem_is_restorable(obj)	hg_mem_is_flags__inline(obj, HG_FL_RESTORABLE)
#define hg_mem_complex_mark(obj)	hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) | HG_FL_COMPLEX, FALSE)
#define hg_mem_complex_unmark(obj)	hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) & ~HG_FL_COMPLEX, FALSE)
#define hg_mem_is_complex_mark(obj)	hg_mem_is_flags__inline(obj, HG_FL_COMPLEX)
#define hg_mem_set_lock(obj)		hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) | HG_FL_LOCK, FALSE)
#define hg_mem_set_unlock(obj)		hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) & ~HG_FL_LOCK, FALSE)
#define hg_mem_is_locked(obj)		hg_mem_is_flags__inline(obj, HG_FL_LOCK)
#define hg_mem_set_copying(obj)		hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) | HG_FL_COPYING, FALSE)
#define hg_mem_unset_copying(obj)	hg_mem_set_flags__inline(obj, hg_mem_get_flags__inline(obj) & ~HG_FL_COPYING, FALSE)
#define hg_mem_is_copying(obj)		hg_mem_is_flags__inline(obj, HG_FL_COPYING)

void     hg_mem_gc_mark_array_region       (HgMemPool *pool,
					    gpointer   start,
					    gpointer   end);
void     hg_mem_add_root_node              (HgMemPool *pool,
					    gpointer   data);
void     hg_mem_remove_root_node           (HgMemPool *pool,
					    gpointer   data);
void     hg_mem_add_pool_reference         (HgMemPool *pool,
					    HgMemPool *other_pool);
void     hg_mem_remove_pool_reference      (HgMemPool *pool,
					    HgMemPool *other_pool);
void     hg_mem_pool_use_periodical_gc     (HgMemPool *pool,
					    gboolean   flag);
void     hg_mem_pool_use_garbage_collection(HgMemPool *pool,
					    gboolean   flag);

/* HgObject */
#define HG_OBJECT_GET_VTABLE_ID(_obj)		(((_obj)->state >> 24) & 0xff)
#define HG_OBJECT_SET_VTABLE_ID(_obj,_id)	((_obj)->state = HG_OBJECT_GET_STATE (_obj) | ((_id) << 24))
#define HG_OBJECT_GET_STATE(_obj)		((_obj)->state & 0xffffff)
#define HG_OBJECT_SET_STATE(_obj,_state)	((_obj)->state = (HG_OBJECT_GET_VTABLE_ID (_obj) << 24) | _state)
#define HG_OBJECT_INIT_STATE(_obj)		((_obj)->state = 0)
#define hg_object_readable(obj)		hg_object_add_state(obj, HG_ST_READABLE)
#define hg_object_unreadable(obj)	hg_object_set_state(obj, hg_object_get_state(obj) & ~HG_ST_READABLE)
#define hg_object_is_readable(obj)	hg_object_is_state(obj, HG_ST_READABLE)
#define hg_object_writable(obj)		hg_object_add_state(obj, HG_ST_WRITABLE)
#define hg_object_is_writable(obj)	hg_object_is_state(obj, HG_ST_WRITABLE)
#define hg_object_unwritable(obj)	hg_object_set_state(obj, hg_object_get_state(obj) & ~HG_ST_WRITABLE)
#define hg_object_executable(obj)	hg_object_add_state(obj, HG_ST_EXECUTABLE)
#define hg_object_unexecutable(obj)	hg_object_set_state(obj, hg_object_get_state(obj) & ~HG_ST_EXECUTABLE)
#define hg_object_is_executable(obj)	hg_object_is_state(obj, HG_ST_EXECUTABLE)

guint                       hg_object_get_state (HgObject                   *object);
void                        hg_object_set_state (HgObject                   *object,
						 guint                       state);
void                        hg_object_add_state (HgObject                   *object,
						 guint                       state);
gboolean                    hg_object_is_state  (HgObject                   *object,
						 guint                       state);
gpointer                    hg_object_dup       (HgObject                   *object);
gpointer                    hg_object_copy      (HgObject                   *object);
const HgObjectVTable const *hg_object_get_vtable(HgObject                   *object);
void                        hg_object_set_vtable(HgObject                   *object,
						 const HgObjectVTable const *vtable);


G_END_DECLS

#endif /* __HG_MEM_H__ */
