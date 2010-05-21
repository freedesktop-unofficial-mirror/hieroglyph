/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator.c
 * Copyright (C) 2006-2010 Akira TAGOH
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
/* Memory allocator on the first fit algorithm */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "hgerror.h"
#include "hgmem-private.h"
#include "hgallocator.h"


#define BLOCK_SIZE		32

#define hg_mem_aligned_to(x,y)			\
	(((x) + (y) - 1) & ~((y) - 1))
#define hg_mem_aligned_size(x)			\
	hg_mem_aligned_to(x, ALIGNOF_VOID_P)


struct _hg_allocator_bitmap_t {
	guint32 *bitmaps;
	gsize    size;
};
struct _hg_allocator_block_t {
	hg_quark_t     index;
	gsize          size;
	volatile guint lock_count;
};
struct _hg_allocator_private_t {
	hg_allocator_bitmap_t *bitmap;
	gpointer               heap;
	gsize                  size_in_use;
	hg_quark_t             current_id;
	GTree                 *block_in_use;
};


static hg_allocator_bitmap_t *_hg_allocator_bitmap_new                (gsize                  size);
static void                   _hg_allocator_bitmap_destroy            (gpointer               data);
static gboolean               _hg_allocator_bitmap_resize             (hg_allocator_bitmap_t *bitmap,
                                                                       gsize                  size);
static hg_quark_t             _hg_allocator_bitmap_alloc              (hg_allocator_bitmap_t *bitmap,
                                                                       gsize                  size);
static void                   _hg_allocator_bitmap_free               (hg_allocator_bitmap_t *bitmap,
                                                                       hg_quark_t             index,
                                                                       gsize                  size);
static void                   _hg_allocator_bitmap_mark               (hg_allocator_bitmap_t *bitmap,
                                                                       gint32                 index);
static void                   _hg_allocator_bitmap_clear              (hg_allocator_bitmap_t *bitmap,
                                                                       gint32                 index);
static gboolean               _hg_allocator_bitmap_is_marked          (hg_allocator_bitmap_t *bitmap,
                                                                       gint32                 index);
static gboolean               _hg_allocator_initialize                (hg_mem_t              *mem);
static void                   _hg_allocator_finalize                  (hg_mem_t              *mem);
static gboolean               _hg_allocator_resize_heap               (hg_mem_t              *mem);
static hg_quark_t             _hg_allocator_alloc                     (hg_mem_t              *mem,
                                                                       gsize                  size);
static void                   _hg_allocator_free                      (hg_mem_t              *mem,
                                                                       hg_quark_t             data);
static gpointer               _hg_allocator_initialize_and_lock_object(hg_mem_t              *mem,
                                                                       hg_quark_t             index);
static gpointer               _hg_allocator_lock_object               (hg_mem_t              *mem,
                                                                       hg_quark_t             data);
static void                   _hg_allocator_unlock_object             (hg_mem_t              *mem,
                                                                       hg_quark_t             data);

static hg_mem_vtable_t __hg_allocator_vtable = {
	.initialize    = _hg_allocator_initialize,
	.finalize      = _hg_allocator_finalize,
	.resize_heap   = _hg_allocator_resize_heap,
	.alloc         = _hg_allocator_alloc,
	.free          = _hg_allocator_free,
	.lock_object   = _hg_allocator_lock_object,
	.unlock_object = _hg_allocator_unlock_object,
};
G_LOCK_DEFINE_STATIC (bitmap);

/*< private >*/
static gint
_btree_key_compare(gconstpointer a,
		   gconstpointer b)
{
	return HGPOINTER_TO_QUARK (a) - HGPOINTER_TO_QUARK (b);
}

/** bitmap operation **/
static hg_allocator_bitmap_t *
_hg_allocator_bitmap_new(gsize size)
{
	hg_allocator_bitmap_t *retval;
	gsize aligned_size, bitmap_size;

	hg_return_val_if_fail (size > 0, NULL);

	aligned_size = hg_mem_aligned_to (size, BLOCK_SIZE);
	bitmap_size = hg_mem_aligned_to (aligned_size / BLOCK_SIZE, sizeof (guint32));
	retval = g_new(hg_allocator_bitmap_t, 1);
	if (retval) {
		retval->bitmaps = g_new0(guint32, bitmap_size / sizeof (guint32));
		retval->size = bitmap_size;
	}

	return retval;
}

static void
_hg_allocator_bitmap_destroy(gpointer data)
{
	hg_allocator_bitmap_t *bitmap = data;

	if (!data)
		return;
	g_free(bitmap->bitmaps);
	g_free(bitmap);
}

static gboolean
_hg_allocator_bitmap_resize(hg_allocator_bitmap_t *bitmap,
			    gsize                  size)
{
	gboolean retval = TRUE;
	gsize aligned_size, bitmap_size;

	hg_return_val_if_fail (bitmap != NULL, FALSE);

	G_LOCK (bitmap);

	aligned_size = hg_mem_aligned_to (size, BLOCK_SIZE);
	bitmap_size = hg_mem_aligned_to (aligned_size / BLOCK_SIZE, sizeof (guint32));
	bitmap->bitmaps = g_renew(guint32, bitmap->bitmaps, bitmap_size / sizeof (guint32));
	if (!bitmap->bitmaps) {
		retval = FALSE;
	} else {
		if (bitmap_size > bitmap->size) {
			memset(bitmap->bitmaps + (bitmap->size / sizeof (guint32)),
			       0, (bitmap_size - bitmap->size) / sizeof (guint32));
		}
		bitmap->size = bitmap_size;
	}

	G_UNLOCK (bitmap);

	return retval;
}

static hg_quark_t
_hg_allocator_bitmap_alloc(hg_allocator_bitmap_t *bitmap,
			   gsize                  size)
{
	gint32 i, j;
	gboolean found;
	gsize aligned_size;

	hg_return_val_if_fail (bitmap != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	aligned_size = hg_mem_aligned_to(size, BLOCK_SIZE) / BLOCK_SIZE;
	for (i = 0; i < bitmap->size; i++) {
		if (_hg_allocator_bitmap_is_marked(bitmap, i)) {
			found = TRUE;
			for (j = i + 1; j < (i + aligned_size); j++) {
				if (!_hg_allocator_bitmap_is_marked(bitmap, j)) {
					i = j;
					found = FALSE;
					break;
				}
			}
			if (found) {
				G_LOCK (bitmap);

				for (j = i; j < (i + aligned_size); j++)
					_hg_allocator_bitmap_mark(bitmap, j);

				G_UNLOCK (bitmap);

				return (hg_quark_t)i;
			}
		}
	}

	return Qnil;
}

static void
_hg_allocator_bitmap_free(hg_allocator_bitmap_t *bitmap,
			  hg_quark_t             index,
			  gsize                  size)
{
	hg_quark_t i;
	gsize aligned_size;

	hg_return_if_fail (bitmap != NULL);
	hg_return_if_fail (index > 0);
	hg_return_if_fail (size > 0);

	aligned_size = hg_mem_aligned_to(size, BLOCK_SIZE) / BLOCK_SIZE;
	for (i = index; i < (index + aligned_size); i++)
		_hg_allocator_bitmap_clear(bitmap, i);
}

static void
_hg_allocator_bitmap_mark(hg_allocator_bitmap_t *bitmap,
			  gint32                 index)
{
	hg_return_if_fail (bitmap != NULL);

	bitmap->bitmaps[index / sizeof (gint32)] |= 1 << (index % sizeof (gint32));
}

static void
_hg_allocator_bitmap_clear(hg_allocator_bitmap_t *bitmap,
			   gint32                 index)
{
	hg_return_if_fail (bitmap != NULL);

	bitmap->bitmaps[index / sizeof (gint32)] &= ~(1 << (index % sizeof (gint32)));
}

static gboolean
_hg_allocator_bitmap_is_marked(hg_allocator_bitmap_t *bitmap,
			       gint32                 index)
{
	hg_return_val_if_fail (bitmap != NULL, FALSE);

	return bitmap->bitmaps[index / sizeof (gint32)] & 1 << (index % sizeof (gint32));
}

/** allocator **/
static gboolean
_hg_allocator_initialize(hg_mem_t *mem)
{
	hg_allocator_private_t *data;

	hg_return_val_if_fail (mem != NULL, FALSE);

	if (mem->private)
		return TRUE;
	data = mem->private = g_new0(hg_allocator_private_t, 1);
	if (!data)
		return FALSE;
	data->current_id = 2;
	data->block_in_use = g_tree_new(&_btree_key_compare);

	return TRUE;
}

static void
_hg_allocator_finalize(hg_mem_t *mem)
{
	hg_allocator_private_t *data;

	hg_return_if_fail (mem != NULL);

	if (!mem->private)
		return;

	data = mem->private;
	g_tree_destroy(data->block_in_use);
	g_free(data->heap);
	_hg_allocator_bitmap_destroy(data->bitmap);

	g_free(mem->private);

	mem->private = NULL;
}

static gboolean
_hg_allocator_resize_heap(hg_mem_t *mem)
{
	hg_allocator_private_t *data;

	hg_return_val_if_fail (mem != NULL, FALSE);
	hg_return_val_if_fail (mem->private != NULL, FALSE);
	hg_return_val_if_fail (mem->total_size > 0, FALSE);

	data = mem->private;
	if (data->bitmap) {
		if (!_hg_allocator_bitmap_resize(data->bitmap, mem->total_size))
			return FALSE;
	} else {
		data->bitmap = _hg_allocator_bitmap_new(mem->total_size);
		if (!data->bitmap)
			return FALSE;
	}
	data->heap = g_realloc(data->heap, data->bitmap->size * BLOCK_SIZE);
	if (!data->heap)
		return FALSE;
	mem->total_size = data->bitmap->size * BLOCK_SIZE;

	return TRUE;
}

static hg_quark_t
_hg_allocator_alloc(hg_mem_t *mem,
		    gsize     size)
{
	hg_allocator_private_t *data;
	hg_allocator_block_t *block;
	gsize obj_size, header_size;
	hg_quark_t index, retval = Qnil;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (mem->private != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	data = mem->private;

	hg_return_val_if_fail (data->current_id != 0, Qnil); /* FIXME: need to find out the free id */

	header_size = hg_mem_aligned_size (sizeof (hg_allocator_block_t));
	obj_size = hg_mem_aligned_size (sizeof (hg_allocator_block_t) + size);
	index = _hg_allocator_bitmap_alloc(data->bitmap, obj_size);
	if (index >= 0) {
		block = _hg_allocator_initialize_and_lock_object(mem, index);
		memset(block, 0, sizeof (hg_allocator_block_t));
		block->index = index;
		block->size = obj_size;
		retval = data->current_id++;
		g_tree_insert(data->block_in_use, HGQUARK_TO_POINTER (retval), block);
		_hg_allocator_unlock_object(mem, retval);
	}

	return retval;
}

void
_hg_allocator_free(hg_mem_t   *mem,
		   hg_quark_t  index)
{
	hg_allocator_private_t *data;
	hg_allocator_block_t *block;

	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->private != NULL);
	hg_return_if_fail (index != Qnil);

	data = mem->private;
	block = _hg_allocator_lock_object(mem, index);
	if (block) {
		g_tree_remove(data->block_in_use, HGQUARK_TO_POINTER (index));
		_hg_allocator_bitmap_free(data->bitmap, block->index, block->size);
	}
}

static gpointer
_hg_allocator_initialize_and_lock_object(hg_mem_t   *mem,
					 hg_quark_t  index)
{
	hg_allocator_private_t *data;
	hg_allocator_block_t *retval;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (mem->private != NULL, NULL);

	data = mem->private;
	retval = (hg_allocator_block_t *)((gchar *)data->heap) + (index * BLOCK_SIZE);
	memset(retval, 0, sizeof (hg_allocator_block_t));
	retval->lock_count = 1;

	return retval;
}

static gpointer
_hg_allocator_lock_object(hg_mem_t   *mem,
			  hg_quark_t  index)
{
	hg_allocator_private_t *data;
	hg_allocator_block_t *retval = NULL;
	gint old_val;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (mem->private != NULL, NULL);

	data = mem->private;
	if ((retval = g_tree_lookup(data->block_in_use, HGQUARK_TO_POINTER (index))) != NULL) {
		old_val = g_atomic_int_exchange_and_add((int *)&retval->lock_count, 1);
	}

	return retval;
}

static void
_hg_allocator_unlock_object(hg_mem_t   *mem,
			    hg_quark_t  index)
{
	hg_allocator_private_t *data;
	hg_allocator_block_t *retval = NULL;
	gint old_val;

	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->private != NULL);

	data = mem->private;
	if ((retval = g_tree_lookup(data->block_in_use, HGQUARK_TO_POINTER (index))) != NULL) {
		hg_return_if_fail (retval->lock_count > 0);

	  retry_atomic_decrement:
		old_val = g_atomic_int_get(&retval->lock_count);
		if (!g_atomic_int_compare_and_exchange((int *)&retval->lock_count, old_val, old_val - 1))
			goto retry_atomic_decrement;
	}
}

/*< public >*/
hg_mem_vtable_t *
hg_allocator_get_vtable(void)
{
	return &__hg_allocator_vtable;
}
