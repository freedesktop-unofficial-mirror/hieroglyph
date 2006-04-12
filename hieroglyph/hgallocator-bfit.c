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

#include "hgallocator-bfit.h"
#include "hgallocator-private.h"
#include "hgmem.h"
#include "hgbtree.h"


#define BTREE_N_NODE	6


typedef struct _HieroGlyphAllocatorBFitPrivate		HgAllocatorBFitPrivate;
typedef struct _HieroGlyphMemBFitBlock			HgMemBFitBlock;


struct _HieroGlyphAllocatorBFitPrivate {
	GPtrArray *free_block_array;
	GPtrArray *used_block_array;
};

struct _HieroGlyphMemBFitBlock {
	gpointer        heap_fragment;
	gint            heap_id;
	HgMemBFitBlock *prev;
	HgMemBFitBlock *next;
	gsize           length;
	gboolean        in_use;
};

static gboolean       _hg_allocator_bfit_real_initialize        (HgMemPool     *pool,
								 gsize          prealloc);
static gboolean       _hg_allocator_bfit_real_destroy           (HgMemPool     *pool);
static gboolean       _hg_allocator_bfit_real_resize_pool       (HgMemPool     *pool,
								 gsize          size);
static gpointer       _hg_allocator_bfit_real_alloc             (HgMemPool     *pool,
								 gsize          size,
								 guint          flags);
static void           _hg_allocator_bfit_real_free              (HgMemPool     *pool,
								 gpointer       data);
static gpointer       _hg_allocator_bfit_real_resize            (HgMemObject   *object,
								 gsize          size);
static gboolean       _hg_allocator_bfit_real_garbage_collection(HgMemPool     *pool);
static void           _hg_allocator_bfit_real_gc_mark           (HgMemPool     *pool);
static HgMemSnapshot *_hg_allocator_bfit_real_save_snapshot     (HgMemPool     *pool);
static gboolean       _hg_allocator_bfit_real_restore_snapshot  (HgMemPool     *pool,
								 HgMemSnapshot *snapshot);


static HgAllocatorVTable __hg_allocator_bfit_vtable = {
	.initialize         = _hg_allocator_bfit_real_initialize,
	.destroy            = _hg_allocator_bfit_real_destroy,
	.resize_pool        = _hg_allocator_bfit_real_resize_pool,
	.alloc              = _hg_allocator_bfit_real_alloc,
	.free               = _hg_allocator_bfit_real_free,
	.resize             = _hg_allocator_bfit_real_resize,
	.garbage_collection = _hg_allocator_bfit_real_garbage_collection,
	.gc_mark            = _hg_allocator_bfit_real_gc_mark,
	.save_snapshot      = _hg_allocator_bfit_real_save_snapshot,
	.restore_snapshot   = _hg_allocator_bfit_real_restore_snapshot,
};

/*
 * Private Functions
 */
#define _hg_allocator_compute_minimum_block_size_index__inline(__hg_acb_size, __hg_acb_ret) \
	G_STMT_START {							\
		gsize __hg_acbt = (__hg_acb_size);			\
		(__hg_acb_ret) = 0;					\
		while (__hg_acbt >= 2) {				\
			__hg_acbt /= 2;					\
			(__hg_acb_ret)++;				\
		}							\
	} G_STMT_END
#define _hg_allocator_grow_index_size__inline(__hg_acb_size, __hg_acb_ret) \
	G_STMT_START {							\
		if ((1 << (__hg_acb_ret)) < (__hg_acb_size))		\
			(__hg_acb_ret)++;				\
	} G_STMT_END
#define _hg_allocator_compute_block_size_index__inline(__hg_acb_size, __hg_acb_ret) \
	G_STMT_START {							\
		_hg_allocator_compute_minimum_block_size_index__inline((__hg_acb_size), (__hg_acb_ret)); \
		_hg_allocator_grow_index_size__inline((__hg_acb_size), (__hg_acb_ret));	\
	} G_STMT_END
#define _hg_allocator_compute_block_size__inline(__hg_acb_size, __hg_acb_ret) \
	G_STMT_START {							\
		_hg_allocator_compute_block_size_index__inline((__hg_acb_size), (__hg_acb_ret)); \
		(__hg_acb_ret) = 1 << (__hg_acb_ret);			\
	} G_STMT_END

/* utility functions */
static HgMemBFitBlock *
_hg_bfit_block_new(gpointer heap,
		   gsize    length)
{
	HgMemBFitBlock *retval = g_new(HgMemBFitBlock, 1);

	if (retval != NULL) {
		retval->heap_fragment = heap;
		retval->length = length;
		retval->in_use = FALSE;
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
	GPtrArray *farray = priv->free_block_array;
	guint index;
	GSList *l, *tmp = NULL;

	_hg_allocator_compute_minimum_block_size_index__inline(block->length, index);
	l = g_ptr_array_index(farray, index);
	if ((tmp = g_slist_find(l, block)) == NULL) {
		/* maybe bug? */
		g_warning("[BUG] can't find a memory block %p (size: %" G_GSIZE_FORMAT ", index: %u).\n", block, block->length, index);
		/* try another way */
		_hg_allocator_grow_index_size__inline(block->length, index);
		l = g_ptr_array_index(farray, index);
		if ((tmp = g_slist_find(l, block)) == NULL) {
			g_warning("[BUG] can't find a memory block %p with even index: %u either.\n", block, index);
		} else {
			if ((1 << index) > block->length)
				g_warning("[BUG] a memory block %p is indexed over its size (size: %" G_GSIZE_FORMAT ", index: %u).\n", block, block->length, index);
		}
	}
	if (tmp != NULL) {
		g_ptr_array_index(farray, index) = g_slist_delete_link(l, tmp);
	}
}

static void
_hg_allocator_bfit_add_used_block(HgAllocatorBFitPrivate *priv,
				  HgMemBFitBlock         *block)
{
	GPtrArray *farray = priv->used_block_array;
	guint index;

	block->in_use = TRUE;
	_hg_allocator_compute_minimum_block_size_index__inline(block->length, index);
	if (farray->len < index) {
		guint i, j = index - farray->len;
		GSList *l;

		for (i = 0; i < j; i++)
			g_ptr_array_add(farray, NULL);
		l = g_slist_alloc();
		l->data = block;
		l->next = NULL;
		g_ptr_array_add(farray, l);
	} else {
		GSList *l = g_ptr_array_index(farray, index);

		g_ptr_array_index(farray, index) = g_slist_append(l, block);
	}
}

static GSList *
_hg_allocator_bfit_get_used_block_list(HgAllocatorBFitPrivate *priv,
				       HgMemBFitBlock         *block,
				       guint                  *index)
{
	guint idx;
	GPtrArray *farray = priv->used_block_array;

	_hg_allocator_compute_minimum_block_size_index__inline(block->length, idx);
	if (index != NULL)
		*index = idx;
	if (farray->len < idx)
		return NULL;
	return g_ptr_array_index(farray, idx);
}

static gboolean
_hg_allocator_bfit_has_used_block_with_list(HgAllocatorBFitPrivate *priv,
					    HgMemBFitBlock         *block,
					    GSList                 *list)
{
	return g_slist_find(list, block) != NULL;
}

#if 0
static gboolean
_hg_allocator_bfit_has_used_block(HgAllocatorBFitPrivate *priv,
				  HgMemBFitBlock         *block)
{
	GSList *l = _hg_allocator_bfit_get_used_block_list(priv, block, NULL);

	return _hg_allocator_bfit_has_used_block_with_list(priv, block, l);
}
#endif

static gboolean
_hg_allocator_bfit_remove_used_block(HgAllocatorBFitPrivate *priv,
				     HgMemBFitBlock         *block)
{
	guint index;
	gboolean retval = FALSE;
	GSList *l = _hg_allocator_bfit_get_used_block_list(priv, block, &index);
	GPtrArray *farray = priv->used_block_array;

	if (_hg_allocator_bfit_has_used_block_with_list(priv, block, l)) {
		g_ptr_array_index(farray, index) = g_slist_remove(l, block);
		retval = TRUE;
	}

	return retval;
}

static void
_hg_allocator_bfit_add_free_block(HgAllocatorBFitPrivate *priv,
				  HgMemBFitBlock         *block)
{
	GPtrArray *farray = priv->free_block_array;
	guint index;

	block->in_use = FALSE;
	/* trying to resolve the fragmentation */
	while (block->prev != NULL && block->prev->in_use == FALSE) {
		HgMemBFitBlock *b = block->prev;

		/* block must be removed from array because available size is increased. */
		_hg_allocator_bfit_remove_block(priv, b);
		/* it could be merged now */
		b->length += block->length;
		b->next = block->next;
		g_free(block);
		block = b;
	}
	while (block->next != NULL && block->next->in_use == FALSE) {
		HgMemBFitBlock *b = block->next;

		/* it could be merged now */
		block->length += b->length;
		block->next = b->next;
		/* block must be removed because it's no longer available */
		_hg_allocator_bfit_remove_block(priv, b);
		g_free(b);
	}
	_hg_allocator_compute_minimum_block_size_index__inline(block->length, index);
	if (farray->len < index) {
		guint i, j = index - farray->len;
		GSList *l;

		for (i = 0; i < j; i++)
			g_ptr_array_add(farray, NULL);
		l = g_slist_alloc();
		l->data = block;
		l->next = NULL;
		g_ptr_array_add(farray, l);
	} else {
		GSList *l = g_ptr_array_index(farray, index);

		g_ptr_array_index(farray, index) = g_slist_append(l, block);
	}
}

static HgMemBFitBlock *
_hg_allocator_bfit_get_free_block(HgAllocatorBFitPrivate *priv,
				  gsize                   size)
{
	guint index;
	GPtrArray *farray = priv->free_block_array;
	GSList *l;
	guint i;
	HgMemBFitBlock *retval = NULL;

	_hg_allocator_compute_minimum_block_size_index__inline(size, index);
	if (farray->len < index)
		return NULL;
	for (i = index; i < farray->len; i++) {
		l = g_ptr_array_index(farray, i);
		if (l != NULL) {
			retval = l->data;
			g_ptr_array_index(farray, i) = g_slist_delete_link(l, l);
		}
	}
	if (retval != NULL) {
		gsize size = 1 << index, unused = retval->length - size, min_size;
		HgMemBFitBlock *block;

		_hg_allocator_compute_block_size__inline(sizeof (HgMemObject), min_size);
		if (unused > 0 && unused >= min_size) {
			block = _hg_bfit_block_new((gpointer)((gsize)retval->heap_fragment + size),
						   unused);
			if (block != NULL) {
				block->prev = retval;
				block->next = retval->next;
				retval->length = size;
				retval->next = block;
				/* in_use flag must be set to true here.
				 * otherwise warning will appears in _hg_allocator_bfit_add_free_block.
				 */
				retval->in_use = TRUE;
				_hg_allocator_bfit_add_free_block(priv, block);
			}
		}
		retval->in_use = TRUE;
	}

	return retval;
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

	_hg_allocator_compute_block_size__inline(prealloc, total_heap_size);
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

	block = _hg_bfit_block_new(heap->heaps, total_heap_size);
	if (block == NULL) {
		hg_heap_free(heap);
		g_free(priv);

		return FALSE;
	}

	priv->free_block_array = g_ptr_array_new();
	priv->used_block_array = g_ptr_array_new();

	_hg_allocator_bfit_add_free_block(priv, block);
	g_ptr_array_add(pool->heap_list, heap);

	pool->allocator->private = priv;

	return TRUE;
}

static gboolean
_hg_allocator_bfit_real_destroy(HgMemPool *pool)
{
	HgAllocatorBFitPrivate *priv = pool->allocator->private;
	guint i;

	if (priv->free_block_array) {
		for (i = 0; i < priv->free_block_array->len; i++) {
			GSList *l = g_ptr_array_index(priv->free_block_array, i), *ll;

			for (ll = l; ll != NULL; ll = g_slist_next(ll)) {
				_hg_bfit_block_free(ll->data);
			}
			g_slist_free(l);
		}
		g_ptr_array_free(priv->free_block_array, TRUE);
	}
	if (priv->used_block_array) {
		for (i = 0; i < priv->used_block_array->len; i++) {
			GSList *l = g_ptr_array_index(priv->used_block_array, i), *ll;

			for (ll = l; ll != NULL; ll = g_slist_next(ll)) {
				g_print(":%p\n", ll->data);
				_hg_bfit_block_free(ll->data);
			}
			g_slist_free(l);
		}
		g_ptr_array_free(priv->used_block_array, TRUE);
	}
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

	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject) + size, block_size);
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
	block = _hg_bfit_block_new(heap->heaps, block_size);
	if (block == NULL) {
		hg_heap_free(heap);

		return FALSE;
	}
	pool->total_heap_size += block_size;
	_hg_allocator_bfit_add_free_block(priv, block);
	g_ptr_array_add(pool->heap_list, heap);

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

	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject), min_size);
	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject) + size, block_size);
	block = _hg_allocator_bfit_get_free_block(priv, block_size);
	if (block != NULL) {
		obj = block->heap_fragment;
		obj->id = HG_MEM_HEADER;
		obj->subid = block;
		obj->heap_id = block->heap_id;
		obj->pool = pool;
		obj->block_size = block_size;
		obj->flags = flags;
		_hg_allocator_bfit_add_used_block(priv, block);

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
	if (!_hg_allocator_bfit_remove_used_block(priv, block)) {
		g_warning("[BUG] Failed to remove an object %p (block: %p) from list.",
			  data, block);
		return;
	}
	_hg_allocator_bfit_add_free_block(priv, block);
}

static gpointer
_hg_allocator_bfit_real_resize(HgMemObject *object,
			       gsize        size)
{
	return NULL;
}

static gboolean
_hg_allocator_bfit_real_garbage_collection(HgMemPool *pool)
{
	return FALSE;
}

static void
_hg_allocator_bfit_real_gc_mark(HgMemPool *pool)
{
}

static HgMemSnapshot *
_hg_allocator_bfit_real_save_snapshot(HgMemPool *pool)
{
	return NULL;
}

static gboolean
_hg_allocator_bfit_real_restore_snapshot(HgMemPool     *pool,
					 HgMemSnapshot *snapshot)
{
	return FALSE;
}

/*
 * Public Functions
 */
HgAllocatorVTable *
hg_allocator_bfit_get_vtable(void)
{
	return &__hg_allocator_bfit_vtable;
}
