/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-bfit.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <setjmp.h>
#include "hgallocator-bfit.h"
#include "hgallocator-private.h"
#include "hglog.h"
#include "hgmem.h"
#include "ibtree.h"
#include "ilist.h"
#include "hgstring.h"


#define BTREE_N_NODE	6


typedef struct _HieroGlyphAllocatorBFitPrivate		HgAllocatorBFitPrivate;
typedef struct _HieroGlyphMemBFitBlock			HgMemBFitBlock;
typedef struct _HieroGlyphSnapshotChunk			HgSnapshotChunk;


struct _HieroGlyphAllocatorBFitPrivate {
	HgBTree     *free_block_tree;
	GPtrArray   *heap2block_array;
	HgBTree     *obj2block_tree;
	gint         age_of_snapshot;
};

struct _HieroGlyphMemBFitBlock {
	gpointer        heap_fragment;
	gint            heap_id;
	HgMemBFitBlock *prev;
	HgMemBFitBlock *next;
	gsize           length;
	gint            in_use;
};

struct _HieroGlyphSnapshotChunk {
	gint             heap_id;
	gsize            offset;
	gsize            length;
	gpointer         heap_chunks;
	HgSnapshotChunk *next;
};


static gboolean       _hg_allocator_bfit_real_initialize        (HgMemPool         *pool,
								 gsize              prealloc);
static gboolean       _hg_allocator_bfit_real_destroy           (HgMemPool         *pool);
static gboolean       _hg_allocator_bfit_real_resize_pool       (HgMemPool         *pool,
								 gsize              size);
static gpointer       _hg_allocator_bfit_real_alloc             (HgMemPool         *pool,
								 gsize              size,
								 guint              flags);
static void           _hg_allocator_bfit_real_free              (HgMemPool         *pool,
								 gpointer           data);
static gpointer       _hg_allocator_bfit_real_resize            (HgMemObject       *object,
								 gsize              size);
static gsize          _hg_allocator_bfit_real_get_size          (HgMemObject       *object);
static gboolean       _hg_allocator_bfit_real_garbage_collection(HgMemPool         *pool);
static void           _hg_allocator_bfit_real_gc_mark           (HgMemPool         *pool);
static gboolean       _hg_allocator_bfit_real_is_safe_object    (HgMemPool         *pool,
								 HgMemObject       *object);
static HgMemSnapshot *_hg_allocator_bfit_real_save_snapshot     (HgMemPool         *pool);
static gboolean       _hg_allocator_bfit_real_restore_snapshot  (HgMemPool         *pool,
								 HgMemSnapshot     *snapshot);
static void           _hg_allocator_bfit_snapshot_real_free     (gpointer           data);
static void           _hg_allocator_bfit_snapshot_real_set_flags(gpointer           data,
								 guint              flags);
static void           _hg_allocator_bfit_snapshot_real_relocate (gpointer           data,
								 HgMemRelocateInfo *info);
static gpointer       _hg_allocator_bfit_snapshot_real_to_string(gpointer           data);


static HgAllocatorVTable __hg_allocator_bfit_vtable = {
	.initialize         = _hg_allocator_bfit_real_initialize,
	.destroy            = _hg_allocator_bfit_real_destroy,
	.resize_pool        = _hg_allocator_bfit_real_resize_pool,
	.alloc              = _hg_allocator_bfit_real_alloc,
	.free               = _hg_allocator_bfit_real_free,
	.resize             = _hg_allocator_bfit_real_resize,
	.get_size           = _hg_allocator_bfit_real_get_size,
	.garbage_collection = _hg_allocator_bfit_real_garbage_collection,
	.gc_mark            = _hg_allocator_bfit_real_gc_mark,
	.is_safe_object     = _hg_allocator_bfit_real_is_safe_object,
	.save_snapshot      = _hg_allocator_bfit_real_save_snapshot,
	.restore_snapshot   = _hg_allocator_bfit_real_restore_snapshot,
};

static HgObjectVTable __hg_snapshot_vtable = {
	.free      = _hg_allocator_bfit_snapshot_real_free,
	.set_flags = _hg_allocator_bfit_snapshot_real_set_flags,
	.relocate  = _hg_allocator_bfit_snapshot_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = _hg_allocator_bfit_snapshot_real_to_string,
};

/*
 * Private Functions
 */
#define _hg_allocator_get_minimum_aligned_size__inline(__hg_aga_size, __hg_alignment_size, __hg_aligned_ret) \
	G_STMT_START {							\
		(__hg_aligned_ret) = (__hg_aga_size) / (__hg_alignment_size) * (__hg_alignment_size); \
	} G_STMT_END
#define _hg_allocator_get_aligned_size__inline(__hg_aga_size, __hg_alignment_size, __hg_aligned_ret) \
	G_STMT_START {							\
		(__hg_aligned_ret) = ((__hg_aga_size) + (__hg_alignment_size)) / (__hg_alignment_size) * (__hg_alignment_size); \
	} G_STMT_END
#define _hg_allocator_get_object_size__inline(__hg_ago_object)	\
	((HgMemBFitBlock *)(__hg_ago_object)->subid)->length

/* utility functions */
static HgMemBFitBlock *
_hg_bfit_block_new(gpointer heap,
		   gsize    length,
		   gint     heap_id)
{
	HgMemBFitBlock *retval = g_new(HgMemBFitBlock, 1);

	if (retval != NULL) {
		retval->heap_fragment = heap;
		retval->heap_id = heap_id;
		retval->length = length;
		retval->in_use = 0;
		retval->next = NULL;
		retval->prev = NULL;
	}

	return retval;
}

#define _hg_bfit_block_free	g_free

static HgSnapshotChunk *
_hg_bfit_snapshot_chunk_new(gint     heap_id,
			    gsize    offset,
			    gsize    length,
			    gpointer heap_chunks)
{
	HgSnapshotChunk *retval = g_new(HgSnapshotChunk, 1);

	retval->heap_id     = heap_id;
	retval->offset      = offset;
	retval->length      = length;
	retval->heap_chunks = g_new(gchar, length + 1);
	retval->next        = NULL;

	memcpy(retval->heap_chunks, heap_chunks, length);

	return retval;
}

static void
_hg_bfit_snapshot_chunk_free(HgSnapshotChunk *chunk)
{
	if (chunk->heap_chunks)
		g_free(chunk->heap_chunks);
	g_free(chunk);
}

static void
_hg_allocator_bfit_remove_block(HgAllocatorBFitPrivate *priv,
				HgMemBFitBlock         *block)
{
	gsize aligned;
	HgList *l;

	_hg_allocator_get_minimum_aligned_size__inline(block->length,
						       HG_MEM_ALIGNMENT,
						       aligned);
	if ((l = hg_btree_find(priv->free_block_tree, GSIZE_TO_POINTER (aligned))) == NULL) {
		hg_log_warning("[BUG] there are no memory chunks sized %" G_GSIZE_FORMAT " (aligned size: %" G_GSIZE_FORMAT ".\n",
			       block->length, aligned);
	} else {
		HgListIter iter = hg_list_find_iter(l, block);

		if (iter == NULL) {
			hg_log_warning("[BUG] can't find a memory block %p (size: %" G_GSIZE_FORMAT ", aligned size: %" G_GSIZE_FORMAT ".\n",
				       block, block->length, aligned);
		} else {
			l = hg_list_iter_delete_link(iter);
			if (l == NULL) {
				/* remove node from tree */
				hg_btree_remove(priv->free_block_tree,
						GSIZE_TO_POINTER (aligned));
			} else {
				hg_btree_replace(priv->free_block_tree,
						 GSIZE_TO_POINTER (aligned), l);
			}
			hg_list_iter_free(iter);
		}
	}
}

static void
_hg_allocator_bfit_add_free_block(HgAllocatorBFitPrivate *priv,
				  HgMemBFitBlock         *block)
{
	gsize aligned;
	HgList *l;

	block->in_use = 0;
	/* clear data to avoid incomplete header detection */
	if (block->length >= (sizeof (HgMemObject) + sizeof (HgObject))) {
		memset(block->heap_fragment, 0, sizeof (HgMemObject) + sizeof (HgObject));
	} else {
		memset(block->heap_fragment, 0, block->length);
	}
	/* trying to resolve the fragmentation */
	while (block->prev != NULL && block->prev->in_use == 0) {
		HgMemBFitBlock *b = block->prev;

		if (((gsize)b->heap_fragment + b->length) != (gsize)block->heap_fragment) {
			hg_log_warning("[BUG] wrong block chain detected. (block: %p heap: %p length: %" G_GSIZE_FORMAT ") is chained from (block: %p heap: %p length: %" G_GSIZE_FORMAT ")", block, block->heap_fragment, block->length, b, b->heap_fragment, b->length);
			break;
		}
		/* block must be removed from array because available size is increased. */
		_hg_allocator_bfit_remove_block(priv, b);
		/* it could be merged now */
		b->length += block->length;
		b->next = block->next;
		if (block->next)
			block->next->prev = b;
		_hg_bfit_block_free(block);
		block = b;
	}
	while (block->next != NULL && block->next->in_use == 0) {
		HgMemBFitBlock *b = block->next;

		if (((gsize)block->heap_fragment + block->length) != (gsize)b->heap_fragment) {
			hg_log_warning("[BUG] wrong block chain detected. (block: %p heap: %p length: %" G_GSIZE_FORMAT ") is chained to (block: %p heap: %p length: %" G_GSIZE_FORMAT ")", block, block->heap_fragment, block->length, b, b->heap_fragment, b->length);
			break;
		}
		/* it could be merged now */
		block->length += b->length;
		block->next = b->next;
		if (b->next)
			b->next->prev = block;
		/* block must be removed because it's no longer available */
		_hg_allocator_bfit_remove_block(priv, b);
		_hg_bfit_block_free(b);
	}
	_hg_allocator_get_minimum_aligned_size__inline(block->length,
						       HG_MEM_ALIGNMENT,
						       aligned);
	if ((l = hg_btree_find(priv->free_block_tree, GSIZE_TO_POINTER (aligned))) == NULL) {
		l = _hg_list_new();
		l = hg_list_append(l, block);
		hg_btree_add(priv->free_block_tree, GSIZE_TO_POINTER (aligned), l);
	} else {
		l = hg_list_prepend(l, block);
		hg_btree_replace(priv->free_block_tree, GSIZE_TO_POINTER (aligned), l);
	}
}

static HgMemBFitBlock *
_hg_allocator_bfit_get_free_block(HgMemPool              *pool,
				  HgAllocatorBFitPrivate *priv,
				  gsize                   size)
{
	gsize aligned, real_aligned;
	HgList *l;
	HgListIter iter;
	HgMemBFitBlock *retval = NULL;

	g_return_val_if_fail (size % HG_MEM_ALIGNMENT == 0, NULL);

	_hg_allocator_get_minimum_aligned_size__inline(size, HG_MEM_ALIGNMENT, aligned);
	if ((l = hg_btree_find_near(priv->free_block_tree, GSIZE_TO_POINTER (aligned))) == NULL)
		return NULL;
	iter = hg_list_iter_new(l);
	if (iter == NULL)
		return NULL;
	retval = hg_list_iter_get_data(iter);
	_hg_allocator_get_minimum_aligned_size__inline(retval->length,
						       HG_MEM_ALIGNMENT,
						       real_aligned);
	l = hg_list_iter_delete_link(iter);
	hg_list_iter_free(iter);
	if (l == NULL) {
		hg_btree_remove(priv->free_block_tree,
				GSIZE_TO_POINTER (real_aligned));
	} else {
		hg_btree_replace(priv->free_block_tree,
				 GSIZE_TO_POINTER (real_aligned), l);
	}
	if (retval != NULL) {
		gsize unused = retval->length - size, min_size;
		HgMemBFitBlock *block;

		_hg_allocator_get_aligned_size__inline(sizeof (HgMemObject), HG_MEM_ALIGNMENT, min_size);
		if (unused > 0 && unused >= min_size) {
			block = _hg_bfit_block_new((gpointer)((gsize)retval->heap_fragment + size),
						   unused,
						   retval->heap_id);
			if (block != NULL) {
				block->prev = retval;
				block->next = retval->next;
				if (retval->next)
					retval->next->prev = block;
				retval->length = size;
				retval->next = block;
				/* in_use flag must be set to true here.
				 * otherwise warning will appears in _hg_allocator_bfit_add_free_block.
				 */
				retval->in_use = 1;
				_hg_allocator_bfit_add_free_block(priv, block);
			}
		}
		pool->used_heap_size += retval->length;
		retval->in_use = 1;
	}

	return retval;
}

static void
_hg_allocator_bfit_relocate(HgMemPool         *pool,
			    HgMemRelocateInfo *info)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	gsize header_size = sizeof (HgMemObject);
	GList *list, *reflist;
	HgMemObject *obj, *new_obj;
	HgObject *hobj;
	gpointer p;

	if (pool->is_processing)
		return;
	pool->is_processing = TRUE;

	/* relocate the addresses in the root node */
	for (list = pool->root_node; list != NULL; list = g_list_next(list)) {
		if ((gsize)list->data >= info->start &&
		    (gsize)list->data <= info->end) {
			list->data = (gpointer)((gsize)list->data + info->diff);
		} else {
			const HgObjectVTable const *vtable;
			HgMemObject *obj;

			/* object that is targetted for relocation will relocates
			 * their member variables later. so we need to ensure
			 * the relocation for others.
			 */
			hobj = (HgObject *)list->data;
			hg_mem_get_object__inline(hobj, obj);
			if (obj != NULL && HG_MEMOBJ_IS_HGOBJECT (obj) &&
			    (vtable = hg_object_get_vtable(hobj)) != NULL &&
			    vtable->relocate) {
				vtable->relocate(hobj, info);
			}
		}
	}
	/* relocate the addresses in another pool */
	for (reflist = pool->other_pool_ref_list;
	     reflist != NULL;
	     reflist = g_list_next(reflist)) {
		/* recursively invoke relocate in another pool. */
		_hg_allocator_bfit_relocate(reflist->data, info);
	}
	/* relocate the addresses in the stack */
	for (p = _hg_stack_start; p > _hg_stack_end; p--) {
		obj = (HgMemObject *)(*(gsize *)p - header_size);
		if ((gsize)obj >= info->start &&
		    (gsize)obj <= info->end) {
			if (hg_btree_find(priv->obj2block_tree, obj) != NULL) {
				new_obj = (HgMemObject *)((gsize)obj + info->diff);
				*(gsize *)p = (gsize)new_obj->data;
			}
		}
		obj = (HgMemObject *)(*(gsize *)p);
		if ((gsize)obj >= info->start &&
		    (gsize)obj <= info->end) {
			if (hg_btree_find(priv->obj2block_tree, obj) != NULL) {
				new_obj = (HgMemObject *)((gsize)obj + info->diff);
				*(gsize *)p = (gsize)new_obj;
			}
		}
	}

	pool->is_processing = FALSE;
}

/* best fit memory allocator */
static gboolean
_hg_allocator_bfit_real_initialize(HgMemPool *pool,
				   gsize      prealloc)
{
	HgAllocatorBFitPrivate *priv;
	HgHeap *heap;
	gsize total_heap_size;
	HgMemBFitBlock *block;

	_hg_allocator_get_aligned_size__inline(prealloc,
					       HG_MEM_ALIGNMENT,
					       total_heap_size);
	priv = g_new0(HgAllocatorBFitPrivate, 1);
	if (priv == NULL)
		return FALSE;

	heap = hg_heap_new(pool, total_heap_size);
	if (heap == NULL) {
		g_free(priv);

		return FALSE;
	}
	pool->total_heap_size = pool->initial_heap_size = total_heap_size;
	pool->used_heap_size = 0;

	block = _hg_bfit_block_new(heap->heaps, total_heap_size, heap->serial);
	if (block == NULL) {
		hg_heap_free(heap);
		g_free(priv);

		return FALSE;
	}

	priv->free_block_tree = hg_btree_new(BTREE_N_NODE);
	priv->heap2block_array = g_ptr_array_new();
	priv->obj2block_tree = hg_btree_new(BTREE_N_NODE);
	priv->age_of_snapshot = 0;

	g_ptr_array_add(priv->heap2block_array, block);
	_hg_allocator_bfit_add_free_block(priv, block);
	hg_mem_pool_add_heap(pool, heap);

	pool->allocator->private = priv;

	return TRUE;
}

static gboolean
_hg_allocator_bfit_real_free_block_traverse(gpointer key,
					    gpointer val,
					    gpointer data)
{
	hg_list_free(val);

	return TRUE;
}

static gboolean
_hg_allocator_bfit_real_destroy(HgMemPool *pool)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	guint i;

	hg_mem_garbage_collection(pool);
	if (priv->heap2block_array) {
		for (i = 0; i < priv->heap2block_array->len; i++) {
			HgMemBFitBlock *block = g_ptr_array_index(priv->heap2block_array, i);
			HgMemBFitBlock *tmp;
			HgMemObject *obj;

			while (block != NULL) {
				tmp = block->next;
				/* split off the link to avoid freeing of block in hg_mem_free */
				block->next = NULL;
				block->prev = NULL;
				if (block->in_use > 0) {
					obj = block->heap_fragment;
					hg_mem_free(obj->data);
				}
				_hg_bfit_block_free(block);
				block = tmp;
			}
		}
		g_ptr_array_free(priv->heap2block_array, TRUE);
	}
	hg_btree_foreach(priv->free_block_tree,
			 _hg_allocator_bfit_real_free_block_traverse,
			 NULL);
	hg_btree_destroy(priv->free_block_tree);
	hg_btree_destroy(priv->obj2block_tree);
	g_free(priv);

	return TRUE;
}

static gboolean
_hg_allocator_bfit_real_resize_pool(HgMemPool *pool,
				    gsize      size)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	HgHeap *heap;
	gsize block_size;
	HgMemBFitBlock *block;

	_hg_allocator_get_aligned_size__inline(sizeof (HgMemObject) + size,
					       HG_MEM_ALIGNMENT,
					       block_size);
	if (pool->initial_heap_size > block_size) {
		block_size = pool->initial_heap_size;
	} else {
		/* it may be a good idea to allocate much more memory
		 * because block_size will be used soon.
		 * then need to be resized again.
		 */
		block_size *= 2;
	}
	heap = hg_heap_new(pool, block_size);
	if (heap == NULL)
		return FALSE;
	block = _hg_bfit_block_new(heap->heaps, block_size, heap->serial);
	if (block == NULL) {
		hg_heap_free(heap);

		return FALSE;
	}
	pool->total_heap_size += block_size;
	g_ptr_array_add(priv->heap2block_array, block);
	_hg_allocator_bfit_add_free_block(priv, block);
	hg_mem_pool_add_heap(pool, heap);

	return TRUE;
}

static gpointer
_hg_allocator_bfit_real_alloc(HgMemPool *pool,
			      gsize      size,
			      guint      flags)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	gsize min_size, block_size;
	HgMemBFitBlock *block;
	HgMemObject *obj = NULL;

	_hg_allocator_get_aligned_size__inline(sizeof (HgMemObject),
					       HG_MEM_ALIGNMENT,
					       min_size);
	_hg_allocator_get_aligned_size__inline(sizeof (HgMemObject) + size,
					       HG_MEM_ALIGNMENT,
					       block_size);
	block = _hg_allocator_bfit_get_free_block(pool, priv, block_size);
	if (block != NULL) {
		/* clear data to avoid incomplete header detection */
		if (block_size >= (sizeof (HgMemObject) + sizeof (HgObject))) {
			memset(block->heap_fragment, 0, sizeof (HgMemObject) + sizeof (HgObject));
		} else {
			memset(block->heap_fragment, 0, block_size);
		}
		obj = block->heap_fragment;
		HG_SET_MAGIC_CODE (obj, HG_MEM_HEADER);
		obj->subid = block;
		obj->pool = pool;
		HG_MEMOBJ_INIT_FLAGS (obj);
		HG_MEMOBJ_SET_HEAP_ID (obj, block->heap_id);
		if ((flags & HG_FL_HGOBJECT) != 0)
			HG_MEMOBJ_SET_HGOBJECT_ID (obj);
		HG_MEMOBJ_SET_FLAGS (obj, flags);
		hg_btree_add(priv->obj2block_tree, block->heap_fragment, block);

		return obj->data;
	}

	return NULL;
}

static void
_hg_allocator_bfit_real_free(HgMemPool *pool,
			     gpointer   data)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	HgMemBFitBlock *block;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL) {
		hg_log_warning("[BUG] Unknown object %p is going to be destroyed.", data);
		return;
	}
	block = obj->subid;
	if (block == NULL) {
		hg_log_warning("[BUG] Broken object %p is going to be destroyed.", data);
		return;
	}
	if (hg_btree_find(priv->obj2block_tree, block->heap_fragment) == NULL) {
		hg_log_warning("[BUG] Failed to remove an object %p (block: %p) from list.",
			       data, block);
		return;
	}
	hg_btree_remove(priv->obj2block_tree, block->heap_fragment);
	pool->used_heap_size -= block->length;
	_hg_allocator_bfit_add_free_block(priv, block);
}

static gpointer
_hg_allocator_bfit_real_resize(HgMemObject *object,
			       gsize        size)
{
	gsize block_size, min_block_size;
	HgMemPool *pool = object->pool;

	HG_SET_STACK_END;

	_hg_allocator_get_aligned_size__inline(sizeof (HgMemObject),
					       HG_MEM_ALIGNMENT,
					       min_block_size);
	_hg_allocator_get_aligned_size__inline(sizeof (HgMemObject) + size,
					       HG_MEM_ALIGNMENT,
					       block_size);
	if (block_size > _hg_allocator_get_object_size__inline(object)) {
		gpointer p;
		HgMemRelocateInfo info;
		HgObject *hobj;
		HgMemObject *obj;
		const HgObjectVTable const *vtable;

		p = hg_mem_alloc_with_flags(pool, block_size, HG_MEMOBJ_GET_FLAGS (object));
		/* reset the stack bottom here again. it may be broken during allocation
		 * if GC was running.
		 */
		HG_SET_STACK_END_AGAIN;
		if (p == NULL)
			return NULL;
		info.start = (gsize)object;
		info.end = (gsize)object;
		info.diff = (gsize)p - (gsize)object->data;
		memcpy(p, object->data,
		       _hg_allocator_get_object_size__inline(object) - sizeof (HgMemObject));
		/* avoid to call HgObject's free function so that
		 * it will be invoked from copied object.
		 */
		if (HG_MEMOBJ_IS_HGOBJECT (object)) {
			hobj = (HgObject *)object->data;
			HG_OBJECT_SET_VTABLE_ID (hobj, 0);
		}
		/* set HG_FL_DEAD flag instead of freeing a object.
		 * the object may be still used where is in snapshot.
		 */
		/* XXX: in PostScript spec, it may be unlikely to happen,
		 *      so that this process is really machine-dependant code.
		 *      and PostScript itself doesn't support to extend
		 *      the object size.
		 */
		hg_mem_set_dead(object);

		_hg_allocator_bfit_relocate(pool, &info);
		hobj = (HgObject *)p;
		hg_mem_get_object__inline(hobj, obj);
		if (obj != NULL && HG_MEMOBJ_IS_HGOBJECT (obj) &&
		    (vtable = hg_object_get_vtable(hobj)) != NULL &&
		    vtable->relocate) {
			vtable->relocate(hobj, &info);
		}

		return p;
	} else if (block_size < _hg_allocator_get_object_size__inline(object) &&
		   (_hg_allocator_get_object_size__inline(object) - block_size) > min_block_size) {
		HgHeap *heap = g_ptr_array_index(pool->heap_list, HG_MEMOBJ_GET_HEAP_ID (object));
		gsize fixed_size = _hg_allocator_get_object_size__inline(object) - block_size;
		HgMemBFitBlock *block = _hg_bfit_block_new((gpointer)((gsize)object + block_size),
							   fixed_size,
							   heap->serial);
		HgMemBFitBlock *blk = object->subid;
		HgAllocatorBFitPrivate *priv = pool->allocator->private;

		if (block == NULL) {
			hg_log_warning("Failed to allocate a block for resizing.");
			return NULL;
		}
		block->prev = blk;
		block->next = blk->next;
		if (blk->next)
			blk->next->prev = block;
		blk->length = block_size;
		blk->next = block;
		pool->used_heap_size -= block->length;
		_hg_allocator_bfit_add_free_block(priv, block);
	}

	return object->data;
}

static gsize
_hg_allocator_bfit_real_get_size(HgMemObject *object)
{
	HgMemBFitBlock *block = object->subid;

	return block->length;
}

static gboolean
_hg_allocator_bfit_real_garbage_collection(HgMemPool *pool)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	guint i;
	gboolean retval = FALSE;
	GList *reflist;
#ifdef DEBUG_GC
	guint total = 0, swept = 0;
#endif /* DEBUG_GC */

	if (pool->is_collecting) {
		/* just return without doing anything */
		return FALSE;
	}
	pool->is_collecting = TRUE;
	pool->age_of_gc_mark++;
	if (pool->age_of_gc_mark == 0)
		pool->age_of_gc_mark++;
	if (!pool->destroyed) {
		/* increase an age of mark in another pool too */
		for (reflist = pool->other_pool_ref_list;
		     reflist != NULL;
		     reflist = g_list_next(reflist)) {
			HgMemPool *p = reflist->data;

			p->age_of_gc_mark++;
			if (p->age_of_gc_mark == 0)
				p->age_of_gc_mark++;
		}
	}
#ifdef DEBUG_GC
	g_print("DEBUG_GC: starting GC for %s\n", pool->name);
#endif /* DEBUG_GC */
	if (!pool->destroyed) {
		if (!pool->use_gc) {
			pool->is_collecting = FALSE;

			return FALSE;
		}
#ifdef DEBUG_GC
		g_print("DEBUG_GC: marking start.\n");
#endif /* DEBUG_GC */
		/* mark phase */
		pool->allocator->vtable->gc_mark(pool);
	}
#ifdef DEBUG_GC
	g_print("DEBUG_GC: sweeping start.\n");
#endif /* DEBUG_GC */
	/* sweep phase */
	for (i = 0; i < priv->heap2block_array->len; i++) {
		HgMemBFitBlock *block = g_ptr_array_index(priv->heap2block_array, i), *tmp;
		HgMemObject *obj;

		while (block != NULL) {
			obj = block->heap_fragment;
			tmp = block->next;
			/* if block isn't in use, it will be destroyed during hg_mem_free.
			 * tmp must points out used block.
			 */
			while (tmp != NULL && tmp->in_use == 0)
				tmp = tmp->next;

			if (block->in_use > 0) {
				if (!hg_mem_is_gc_mark__inline(obj) &&
				    (pool->destroyed || !hg_mem_is_locked(obj))) {
#ifdef DEBUG_GC
					swept++;
					g_print("DEBUG_GC: sweeping %p (block: %p memobj: %p size: %" G_GSIZE_FORMAT " age: %d[current %d])\n", obj->data, obj->subid, obj, ((HgMemBFitBlock *)obj->subid)->length, HG_MEMOBJ_GET_MARK_AGE (obj), obj->pool->age_of_gc_mark);
#endif /* DEBUG_GC */
					hg_mem_free(obj->data);
					retval = TRUE;
				}
			}
#ifdef DEBUG_GC
			total++;
#endif /* DEBUG_GC */
			block = tmp;
		}
	}
#ifdef DEBUG_GC
	g_print("DEBUG_GC: GC finished (total: %d blocks swept: %d blocks)\n",
		total, swept);
#endif /* DEBUG_GC */
	pool->is_collecting = FALSE;

	return retval;
}

static void
_hg_allocator_bfit_real_gc_mark(HgMemPool *pool)
{
	HG_SET_STACK_END;

	if (pool->is_processing)
		return;
	pool->is_processing = TRUE;

#ifdef DEBUG_GC
	g_print("MARK AGE: %d (%s)\n", pool->age_of_gc_mark, pool->name);
#endif /* DEBUG_GC */
	G_STMT_START {
		GList *list, *reflist;
		HgMemObject *obj;
		jmp_buf env;

		/* trace the root node */
		for (list = pool->root_node; list != NULL; list = g_list_next(list)) {
			hg_mem_get_object__inline(list->data, obj);
			if (obj == NULL) {
				hg_log_warning("[BUG] Invalid object %p is in the root node.", list->data);
			} else {
				if (!hg_mem_is_gc_mark__inline(obj)) {
					hg_mem_gc_mark__inline(obj);
#ifdef DEBUG_GC
					g_print("MARK: %p (mem: %p age: %d) from root node.\n", obj->data, obj, HG_MEMOBJ_GET_MARK_AGE (obj));
#endif /* DEBUG_GC */
				} else {
#ifdef DEBUG_GC
					g_print("MARK[already]: %p (mem: %p age: %d) from root node.\n", obj->data, obj, HG_MEMOBJ_GET_MARK_AGE (obj));
#endif /* DEBUG_GC */
				}
			}
		}
		/* trace another pool */
		for (reflist = pool->other_pool_ref_list;
		     reflist != NULL;
		     reflist = g_list_next(reflist)) {
#ifdef DEBUG_GC
			g_print("DEBUG_GC: entering %s\n", ((HgMemPool *)reflist->data)->name);
#endif /* DEBUG_GC */
			((HgMemPool *)reflist->data)->allocator->vtable->gc_mark(reflist->data);
#ifdef DEBUG_GC
			g_print("DEBUG_GC: leaving %s\n", ((HgMemPool *)reflist->data)->name);
#endif /* DEBUG_GC */
		}
		/* trace in the registers */
		setjmp(env);
#ifdef DEBUG_GC
		g_print("DEBUG_GC: marking from registers.\n");
#endif /* DEBUG_GC */
		hg_mem_gc_mark_array_region(pool, (gpointer)env, (gpointer)env + sizeof (env));
		/* trace the stack */
#ifdef DEBUG_GC
		g_print("DEBUG_GC: marking from stacks.\n");
#endif /* DEBUG_GC */
		hg_mem_gc_mark_array_region(pool, _hg_stack_start, _hg_stack_end);
	} G_STMT_END;

	pool->is_processing = FALSE;
}

static gboolean
_hg_allocator_bfit_real_is_safe_object(HgMemPool   *pool,
				       HgMemObject *object)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;

	return hg_btree_find(priv->obj2block_tree, object) != NULL;
}

static HgMemSnapshot *
_hg_allocator_bfit_real_save_snapshot(HgMemPool *pool)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	HgMemSnapshot *retval;
	GPtrArray *heaps_list = g_ptr_array_new();
	guint i;

	hg_mem_garbage_collection(pool);
	for (i = 0; i < pool->n_heaps; i++) {
		gpointer start, end, top;
		HgMemBFitBlock *block, *prev;
		HgSnapshotChunk *chunk, *beginning_of_chunk = NULL, *prev_chunk = NULL;

#define _is_targeted_block(_heap)					\
		((HG_MEMOBJ_GET_FLAGS ((HgMemObject *)_heap) &		\
		  HG_FL_RESTORABLE) == HG_FL_RESTORABLE)

		block = g_ptr_array_index(priv->heap2block_array, i);
		while (block != NULL &&
		       (block->in_use == 0 ||
			!_is_targeted_block (block->heap_fragment)))
			block = block->next;
		if (block != NULL)
			top = block->heap_fragment;

		while (block != NULL) {
			start = block->heap_fragment;
			while (block != NULL &&
			       block->in_use == 1 &&
			       _is_targeted_block (block->heap_fragment)) {
				prev = block;
				block = block->next;
			}
			end = prev->heap_fragment + prev->length;

			chunk = _hg_bfit_snapshot_chunk_new(i, start - top, end - start, start);
			if (beginning_of_chunk == NULL)
				beginning_of_chunk = chunk;
			if (prev_chunk) {
				prev_chunk->next = chunk;
			}
			prev_chunk = chunk;

			while (block != NULL &&
			       (block->in_use == 0 ||
				!_is_targeted_block (block->heap_fragment)))
				block = block->next;
		}
		g_ptr_array_add(heaps_list, beginning_of_chunk);
	}

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgMemSnapshot),
					 HG_FL_HGOBJECT | HG_FL_COMPLEX);
	if (retval == NULL) {
		hg_log_warning("Failed to allocate a memory for snapshot.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_snapshot_vtable);
	retval->id = (gsize)pool;
	retval->heap_list = heaps_list;
	retval->n_heaps = pool->n_heaps;
	retval->age = priv->age_of_snapshot++;

#undef _is_targeted_block

	return retval;
}

static gboolean
_hg_allocator_bfit_real_restore_snapshot(HgMemPool     *pool,
					 HgMemSnapshot *snapshot)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	gboolean retval = TRUE;
	gint i;

	/* just ignore the older children snapshots */
	if (snapshot->age < priv->age_of_snapshot) {
		for (i = 0; i < snapshot->n_heaps; i++) {
			HgSnapshotChunk *chunk = g_ptr_array_index(snapshot->heap_list, i), *tmp;
			HgMemBFitBlock *block = g_ptr_array_index(priv->heap2block_array, i);

			while (chunk != NULL) {
				memcpy(block->heap_fragment + chunk->offset,
				       chunk->heap_chunks,
				       chunk->length);
				tmp = chunk;
				chunk = chunk->next;
				_hg_bfit_snapshot_chunk_free(tmp);
			}
		}
		priv->age_of_snapshot = snapshot->age;

		snapshot->n_heaps = 0;
		g_ptr_array_free(snapshot->heap_list, TRUE);
		snapshot->heap_list = NULL;
		snapshot->age = 0;
		retval = TRUE;
	}
	hg_mem_garbage_collection(pool);

	return retval;
}

/* snapshot */
static void
_hg_allocator_bfit_snapshot_real_free(gpointer data)
{
	HgMemSnapshot *snapshot = data;
	HgSnapshotChunk *chunk, *tmp;
	gint i;

	if (snapshot->heap_list) {
		for (i = 0; i < snapshot->n_heaps; i++) {
			chunk = g_ptr_array_index(snapshot->heap_list, i);
			while (chunk) {
				tmp = chunk;
				chunk = chunk->next;
				_hg_bfit_snapshot_chunk_free(tmp);
			}
		}
		g_ptr_array_free(snapshot->heap_list, TRUE);
	}
}

static void
_hg_allocator_bfit_snapshot_real_set_flags(gpointer data,
					   guint    flags)
{
	HgMemSnapshot *snapshot = data;
	HgSnapshotChunk *chunk;
	gint i;
	HgMemObject *obj;

	for (i = 0; i < snapshot->n_heaps; i++) {
		chunk = g_ptr_array_index(snapshot->heap_list, i);

		while (chunk) {
			obj = chunk->heap_chunks;

			if (!HG_CHECK_MAGIC_CODE (obj, HG_MEM_HEADER)) {
				hg_log_warning("[BUG] Invalid object %p to be marked in snapshot.", obj);
			} else {
				if (!hg_mem_is_flags__inline(obj, flags))
					hg_mem_add_flags__inline(obj, flags, TRUE);
			}

			chunk = chunk->next;
		}
	}
}

static void
_hg_allocator_bfit_snapshot_real_relocate(gpointer           data,
					  HgMemRelocateInfo *info)
{
	/* XXX: is this really necessary?
	 *      the snapshot objects and the chunks in it will be just
	 *      discarded when any complex objects that was created after made
	 *      this snapshot is there. and the relocation won't happens for
	 *      the simple objects.
	 */
}

static gpointer
_hg_allocator_bfit_snapshot_real_to_string(gpointer data)
{
	HgMemObject *obj;
	HgString *retval;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_string_new(obj->pool, 7);
	hg_string_append(retval, "-save-", -1);

	return retval;
}

/*
 * Public Functions
 */
HgAllocatorVTable *
hg_allocator_bfit_get_vtable(void)
{
	return &__hg_allocator_bfit_vtable;
}
