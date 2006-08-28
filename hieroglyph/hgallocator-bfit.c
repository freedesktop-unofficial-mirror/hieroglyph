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
#include "hgallocator-libc.h"
#include "hgallocator-private.h"
#include "hgmem.h"
#include "hgbtree.h"
#include "hgstring.h"


#define BTREE_N_NODE	6


typedef struct _HieroGlyphAllocatorBFitPrivate		HgAllocatorBFitPrivate;
typedef struct _HieroGlyphMemBFitBlock			HgMemBFitBlock;


struct _HieroGlyphAllocatorBFitPrivate {
	HgBTree     *free_block_tree;
	GPtrArray   *heap2block_array;
	HgBTree     *obj2block_tree;
	gint         age_of_snapshot;
	HgAllocator *libc_allocator;
	HgMemPool   *libc_pool;
};

struct _HieroGlyphMemBFitBlock {
	gpointer        heap_fragment;
	gint            heap_id;
	HgMemBFitBlock *prev;
	HgMemBFitBlock *next;
	gsize           length;
	gint            in_use;
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

static void
_hg_allocator_bfit_remove_block(HgAllocatorBFitPrivate *priv,
				HgMemBFitBlock         *block)
{
	gsize aligned;
	GSList *l, *tmp = NULL;

	_hg_allocator_get_minimum_aligned_size__inline(block->length,
						       HG_MEM_ALIGNMENT,
						       aligned);
	if ((l = hg_btree_find(priv->free_block_tree, GSIZE_TO_POINTER (aligned))) == NULL) {
		g_warning("[BUG] there are no memory chunks sized %" G_GSIZE_FORMAT " (aligned size: %" G_GSIZE_FORMAT ".\n",
			  block->length, aligned);
	} else {
		if ((tmp = g_slist_find(l, block)) == NULL) {
			g_warning("[BUG] can't find a memory block %p (size: %" G_GSIZE_FORMAT ", aligned size: %" G_GSIZE_FORMAT ".\n",
				  block, block->length, aligned);
		} else {
			l = g_slist_delete_link(l, tmp);
			if (l == NULL) {
				/* remove node from tree */
				hg_btree_remove(priv->free_block_tree,
						GSIZE_TO_POINTER (aligned));
			} else {
				hg_btree_replace(priv->free_block_tree,
						 GSIZE_TO_POINTER (aligned), l);
			}
		}
	}
}

static void
_hg_allocator_bfit_add_free_block(HgAllocatorBFitPrivate *priv,
				  HgMemBFitBlock         *block)
{
	gsize aligned;
	GSList *l;

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
			g_warning("[BUG] wrong block chain detected. (block: %p heap: %p length: %" G_GSIZE_FORMAT ") is chained from (block: %p heap: %p length: %" G_GSIZE_FORMAT ")", block, block->heap_fragment, block->length, b, b->heap_fragment, b->length);
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
			g_warning("[BUG] wrong block chain detected. (block: %p heap: %p length: %" G_GSIZE_FORMAT ") is chained to (block: %p heap: %p length: %" G_GSIZE_FORMAT ")", block, block->heap_fragment, block->length, b, b->heap_fragment, b->length);
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
		l = g_slist_alloc();
		l->data = block;
		l->next = NULL;
		hg_btree_add(priv->free_block_tree, GSIZE_TO_POINTER (aligned), l);
	} else {
		l = g_slist_prepend(l, block);
		hg_btree_replace(priv->free_block_tree, GSIZE_TO_POINTER (aligned), l);
	}
}

static HgMemBFitBlock *
_hg_allocator_bfit_get_free_block(HgMemPool              *pool,
				  HgAllocatorBFitPrivate *priv,
				  gsize                   size)
{
	gsize aligned, real_aligned;
	GSList *l;
	HgMemBFitBlock *retval = NULL;

	g_return_val_if_fail (size % HG_MEM_ALIGNMENT == 0, NULL);

	_hg_allocator_get_minimum_aligned_size__inline(size, HG_MEM_ALIGNMENT, aligned);
	if ((l = hg_btree_find_near(priv->free_block_tree, GSIZE_TO_POINTER (aligned))) == NULL)
		return NULL;
	retval = l->data;
	_hg_allocator_get_minimum_aligned_size__inline(retval->length,
						       HG_MEM_ALIGNMENT,
						       real_aligned);
	l = g_slist_delete_link(l, l);
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

static gboolean
_hg_allocator_bfit_btree_traverse_in_destroy(gpointer key,
					     gpointer val,
					     gpointer data)
{
	g_slist_free(val);

	return TRUE;
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

	priv->libc_allocator = hg_allocator_new(hg_allocator_libc_get_vtable());
	priv->libc_pool = hg_mem_pool_new(priv->libc_allocator, "libc", 256, FALSE);
	if (priv->libc_pool == NULL) {
		hg_heap_free(heap);
		hg_allocator_destroy(priv->libc_allocator);
		g_free(priv);

		return FALSE;
	}
	priv->free_block_tree = hg_btree_new(priv->libc_pool, BTREE_N_NODE);
	priv->heap2block_array = g_ptr_array_new();
	priv->obj2block_tree = hg_btree_new(priv->libc_pool, BTREE_N_NODE);
	priv->age_of_snapshot = 0;

	g_ptr_array_add(priv->heap2block_array, block);
	_hg_allocator_bfit_add_free_block(priv, block);
	hg_mem_pool_add_heap(pool, heap);

	pool->allocator->private = priv;

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
	hg_btree_foreach(priv->free_block_tree, _hg_allocator_bfit_btree_traverse_in_destroy, NULL);
	hg_mem_free(priv->free_block_tree);
	hg_mem_free(priv->obj2block_tree);
	hg_mem_pool_destroy(priv->libc_pool);
	hg_allocator_destroy(priv->libc_allocator);
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
		g_warning("[BUG] Unknown object %p is going to be destroyed.", data);
		return;
	}
	block = obj->subid;
	if (block == NULL) {
		g_warning("[BUG] Broken object %p is going to be destroyed.", data);
		return;
	}
	if (hg_btree_find(priv->obj2block_tree, block->heap_fragment) == NULL) {
		g_warning("[BUG] Failed to remove an object %p (block: %p) from list.",
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
		hg_mem_free(object->data);
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
			g_warning("Failed to allocate a block for resizing.");
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
					g_print("DEBUG_GC: sweeping %p (block: %p memobj: %p size: %" G_GSIZE_FORMAT ")\n", obj->data, obj->subid, obj, ((HgMemBFitBlock *)obj->subid)->length);
#endif /* DEBUG_GC */
					hg_mem_free(obj->data);
					retval = TRUE;
				}
			}
			block = tmp;
		}
	}
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
				g_warning("[BUG] Invalid object %p is in the root node.", list->data);
			} else {
				if (!hg_mem_is_gc_mark__inline(obj)) {
#ifdef DEBUG_GC
					g_print("MARK: %p (mem: %p age: %d) from root node.\n", obj->data, obj, HG_MEMOBJ_GET_MARK_AGE (obj));
#endif /* DEBUG_GC */
					hg_mem_gc_mark__inline(obj);
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
	HgMemSnapshot *retval;
	guint i;

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgMemSnapshot),
					 HG_FL_HGOBJECT | HG_FL_COMPLEX);
	if (retval == NULL) {
		g_warning("Failed to allocate a memory for snapshot.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	/* set NULL to avoid the call before finishing an initialization. */
	HG_OBJECT_SET_VTABLE_ID (&retval->object, 0);

	retval->id = (gsize)pool;
	retval->heap_list = g_ptr_array_new();
	if (retval->heap_list == NULL) {
		g_warning("Failed to allocate a memory for snapshot.");
		hg_mem_free(retval);

		return NULL;
	}
	retval->n_heaps = 0;
	for (i = 0; i < pool->n_heaps; i++) {
		HgHeap *heap;
		HgHeap *origheap = g_ptr_array_index(pool->heap_list, i);

		heap = hg_heap_new(pool, origheap->total_heap_size);
		if (heap == NULL) {
			g_warning("Failed to allocate a heap for snapshot.");
			hg_mem_free(retval);

			return NULL;
		}
		heap->serial = origheap->serial;
		/* need to decrease the number of heap in pool */
		pool->n_heaps--;
		retval->n_heaps++;
		g_ptr_array_add(retval->heap_list, heap);
	}

	/* FIXME */

	hg_object_set_vtable(&retval->object, &__hg_snapshot_vtable);

	return retval;
}

static gboolean
_hg_allocator_bfit_real_restore_snapshot(HgMemPool     *pool,
					 HgMemSnapshot *snapshot)
{
	return FALSE;
}

/* snapshot */
static void
_hg_allocator_bfit_snapshot_real_free(gpointer data)
{
}

static void
_hg_allocator_bfit_snapshot_real_set_flags(gpointer data,
					   guint    flags)
{
}

static void
_hg_allocator_bfit_snapshot_real_relocate(gpointer           data,
					  HgMemRelocateInfo *info)
{
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
