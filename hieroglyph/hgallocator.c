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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "hgerror.h"
#include "hgallocator.h"
#include "hgallocator-private.h"


static hg_allocator_bitmap_t *_hg_allocator_bitmap_new                (gsize                   size);
static void                   _hg_allocator_bitmap_destroy            (gpointer                data);
static gboolean               _hg_allocator_bitmap_resize             (hg_allocator_bitmap_t  *bitmap,
                                                                       gsize                   size);
static hg_quark_t             _hg_allocator_bitmap_alloc              (hg_allocator_bitmap_t  *bitmap,
                                                                       gsize                   size);
static hg_quark_t             _hg_allocator_bitmap_realloc            (hg_allocator_bitmap_t  *bitmap,
								       hg_quark_t              index,
								       gsize                   old_size,
								       gsize                   size);
static void                   _hg_allocator_bitmap_free               (hg_allocator_bitmap_t  *bitmap,
                                                                       hg_quark_t              index,
                                                                       gsize                   size);
static void                   _hg_allocator_bitmap_mark               (hg_allocator_bitmap_t  *bitmap,
                                                                       gint32                  index);
static void                   _hg_allocator_bitmap_clear              (hg_allocator_bitmap_t  *bitmap,
                                                                       gint32                  index);
static gboolean               _hg_allocator_bitmap_is_marked          (hg_allocator_bitmap_t  *bitmap,
                                                                       gint32                  index);
static gpointer               _hg_allocator_initialize                (void);
static void                   _hg_allocator_finalize                  (hg_allocator_data_t    *data);
static gboolean               _hg_allocator_resize_heap               (hg_allocator_data_t    *data,
                                                                       gsize                   size);
static hg_quark_t             _hg_allocator_alloc                     (hg_allocator_data_t    *data,
                                                                       gsize                   size,
								       gpointer               *ret);
static hg_quark_t             _hg_allocator_realloc                   (hg_allocator_data_t    *data,
								       hg_quark_t              quark,
								       gsize                   size,
								       gpointer               *ret);
static void                   _hg_allocator_free                      (hg_allocator_data_t    *data,
                                                                       hg_quark_t              index);
static gpointer               _hg_allocator_internal_lock_object      (hg_allocator_private_t *data,
                                                                       hg_quark_t              index,
								       gboolean                initialize);
static gpointer               _hg_allocator_real_lock_object          (hg_allocator_data_t    *data,
								       hg_quark_t              index);
static gpointer               _hg_allocator_lock_object               (hg_allocator_data_t    *data,
                                                                       hg_quark_t              index);
static void                   _hg_allocator_real_unlock_object        (hg_allocator_block_t   *block);
static void                   _hg_allocator_unlock_object             (hg_allocator_data_t    *data,
                                                                       hg_quark_t              index);

static hg_mem_vtable_t __hg_allocator_vtable = {
	.initialize    = _hg_allocator_initialize,
	.finalize      = _hg_allocator_finalize,
	.resize_heap   = _hg_allocator_resize_heap,
	.alloc         = _hg_allocator_alloc,
	.realloc       = _hg_allocator_realloc,
	.free          = _hg_allocator_free,
	.lock_object   = _hg_allocator_lock_object,
	.unlock_object = _hg_allocator_unlock_object,
};
G_LOCK_DEFINE_STATIC (bitmap);
G_LOCK_DEFINE_STATIC (allocator);

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
	hg_quark_t i, j;
	gsize aligned_size, required_size;

	hg_return_val_if_fail (bitmap != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	aligned_size = hg_mem_aligned_to(size, BLOCK_SIZE) / BLOCK_SIZE;
#define HG_MEM_DEBUG
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("ALLOC: %" G_GSIZE_FORMAT " blocks required\n", aligned_size);
	g_print("Bitmap: %" G_GSIZE_FORMAT " blocks allocated\n", bitmap->size);
	g_print("         1         2         3         4         5\n");
	g_print("12345678901234567890123456789012345678901234567890");
	for (i = 0; i < bitmap->size; i++) {
		if (i % 50 == 0)
			g_print("\n");
		g_print("%d", _hg_allocator_bitmap_is_marked(bitmap, i + 1) ? 1 : 0);
	}
	g_print("\n");
#endif
	required_size = aligned_size;
	for (i = 0; i < bitmap->size; i++) {
		if (!_hg_allocator_bitmap_is_marked(bitmap, i + 1)) {
			required_size--;
			for (j = i + 1; required_size > 0 && j < bitmap->size; j++) {
				if (!_hg_allocator_bitmap_is_marked(bitmap, j + 1)) {
					required_size--;
				} else {
					break;
				}
			}
			if (required_size == 0) {
				G_LOCK (bitmap);

				for (j = i; j < (i + aligned_size); j++)
					_hg_allocator_bitmap_mark(bitmap, j + 1);

				G_UNLOCK (bitmap);

				return i + 1;
			} else {
				i = j;
				required_size = aligned_size;
			}
		}
	}

	return Qnil;
}

static hg_quark_t
_hg_allocator_bitmap_realloc(hg_allocator_bitmap_t *bitmap,
			     hg_quark_t             index,
			     gsize                  old_size,
			     gsize                  size)
{
	hg_quark_t i, j;
	gsize aligned_size, old_aligned_size, required_size;

	hg_return_val_if_fail (bitmap != NULL, Qnil);
	hg_return_val_if_fail (index != Qnil, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	aligned_size = hg_mem_aligned_to(size, BLOCK_SIZE) / BLOCK_SIZE;
	old_aligned_size = hg_mem_aligned_to(old_size, BLOCK_SIZE) / BLOCK_SIZE;
#define HG_MEM_DEBUG
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("ALLOC: %" G_GSIZE_FORMAT " blocks to be grown from %" G_GSIZE_FORMAT " blocks at index %" G_GSIZE_FORMAT "\n", aligned_size, old_aligned_size, index);
	g_print("Bitmap: %" G_GSIZE_FORMAT " blocks allocated\n", bitmap->size);
	g_print("         1         2         3         4         5\n");
	g_print("12345678901234567890123456789012345678901234567890");
	for (i = 0; i < bitmap->size; i++) {
		if (i % 50 == 0)
			g_print("\n");
		g_print("%d", _hg_allocator_bitmap_is_marked(bitmap, i + 1) ? 1 : 0);
	}
	g_print("\n");
#endif
	required_size = aligned_size - old_aligned_size;
	if (!_hg_allocator_bitmap_is_marked(bitmap, index + old_aligned_size)) {
		required_size--;
		for (j = index + old_aligned_size + 1; required_size > 0 && j < bitmap->size; j++) {
			if (!_hg_allocator_bitmap_is_marked(bitmap, j)) {
				required_size--;
			} else {
				break;
			}
		}
	}
	if (required_size == 0) {
		G_LOCK (bitmap);

		for (j = index + old_aligned_size; j < (index + aligned_size); j++)
			_hg_allocator_bitmap_mark(bitmap, j);

		G_UNLOCK (bitmap);

		return index;
	} else {
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
		g_print("Falling back to allocate the memory.\n");
#endif
		return _hg_allocator_bitmap_alloc(bitmap, size);
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

	G_LOCK (bitmap);

	for (i = index; i < (index + aligned_size); i++)
		_hg_allocator_bitmap_clear(bitmap, i);

	G_UNLOCK (bitmap);

#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("After freed bitmap: %" G_GSIZE_FORMAT " blocks allocated\n", bitmap->size);
	g_print("         1         2         3         4         5\n");
	g_print("12345678901234567890123456789012345678901234567890");
	for (i = 0; i < bitmap->size; i++) {
		if (i % 50 == 0)
			g_print("\n");
		g_print("%d", _hg_allocator_bitmap_is_marked(bitmap, i + 1) ? 1 : 0);
	}
	g_print("\n");
#endif
}

static void
_hg_allocator_bitmap_mark(hg_allocator_bitmap_t *bitmap,
			  gint32                 index)
{
	hg_return_if_fail (bitmap != NULL);
	hg_return_if_fail (index > 0);

	bitmap->bitmaps[(index - 1) / sizeof (gint32)] |= 1 << ((index - 1) % sizeof (gint32));
}

static void
_hg_allocator_bitmap_clear(hg_allocator_bitmap_t *bitmap,
			   gint32                 index)
{
	hg_return_if_fail (bitmap != NULL);
	hg_return_if_fail (index > 0);

	bitmap->bitmaps[(index - 1) / sizeof (gint32)] &= ~(1 << ((index - 1) % sizeof (gint32)));
}

static gboolean
_hg_allocator_bitmap_is_marked(hg_allocator_bitmap_t *bitmap,
			       gint32                 index)
{
	hg_return_val_if_fail (bitmap != NULL, FALSE);
	hg_return_val_if_fail (index > 0, FALSE);

	return bitmap->bitmaps[(index - 1) / sizeof (gint32)] & 1 << ((index - 1) % sizeof (gint32));
}

/** allocator **/
static gpointer
_hg_allocator_initialize(void)
{
	hg_allocator_private_t *retval;

	retval = g_new0(hg_allocator_private_t, 1);
	if (retval) {
		retval->current_id = 2;
		retval->block_in_use = g_tree_new(&_btree_key_compare);
	}

	return retval;
}

static void
_hg_allocator_finalize(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv;

	if (!data)
		return;

	priv = (hg_allocator_private_t *)data;
	g_tree_destroy(priv->block_in_use);
	g_free(priv->heap);
	_hg_allocator_bitmap_destroy(priv->bitmap);

	g_free(priv);
}

static gboolean
_hg_allocator_resize_heap(hg_allocator_data_t *data,
			  gsize                size)
{
	hg_allocator_private_t *priv;

	priv = (hg_allocator_private_t *)data;
	if (priv->bitmap) {
		if (!_hg_allocator_bitmap_resize(priv->bitmap, size))
			return FALSE;
	} else {
		priv->bitmap = _hg_allocator_bitmap_new(size);
		if (!priv->bitmap)
			return FALSE;
	}
	priv->heap = g_realloc(priv->heap, priv->bitmap->size * BLOCK_SIZE);
	if (!priv->heap)
		return FALSE;
	data->total_size = priv->bitmap->size * BLOCK_SIZE;

	return TRUE;
}

static hg_quark_t
_hg_allocator_alloc(hg_allocator_data_t *data,
		    gsize                size,
		    gpointer            *ret)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block;
	gsize obj_size;
	hg_quark_t index, retval = Qnil;

	priv = (hg_allocator_private_t *)data;

	hg_return_val_if_fail (priv->current_id != 0, Qnil); /* FIXME: need to find out the free id */

	obj_size = hg_mem_aligned_size (sizeof (hg_allocator_block_t) + size);
	index = _hg_allocator_bitmap_alloc(priv->bitmap, obj_size);
	if (index != Qnil) {
		block = _hg_allocator_internal_lock_object(priv, index, TRUE);
		block->index = index;
		block->size = obj_size;
		retval = priv->current_id++;
		g_tree_insert(priv->block_in_use, HGQUARK_TO_POINTER (retval), block);
		/* NOTE: No unlock yet here.
		 *       any objects are supposed to be unlocked
		 *       when it's entered into the stack where PostScript
		 *       VM manages. otherwise it will be swept by GC.
		 */
		if (ret)
			*ret = hg_get_allocated_object (block);
	}

	return retval;
}

static hg_quark_t
_hg_allocator_realloc(hg_allocator_data_t *data,
		      hg_quark_t           quark,
		      gsize                size,
		      gpointer            *ret)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block, *new_block;
	gsize obj_size;
	hg_quark_t index = Qnil, retval = Qnil;

	G_LOCK (allocator);

	priv = (hg_allocator_private_t *)data;

	obj_size = hg_mem_aligned_size (sizeof (hg_allocator_block_t) + size);
	block = _hg_allocator_real_lock_object(data, quark);
	if (block) {
		volatile gint make_sure_if_no_referrer;

		make_sure_if_no_referrer = g_atomic_int_get(&block->lock_count);
		hg_return_val_if_fail (make_sure_if_no_referrer <= 2, Qnil);

		index = _hg_allocator_bitmap_realloc(priv->bitmap, block->index, block->size, obj_size);
		if (index != Qnil) {
			new_block = _hg_allocator_internal_lock_object(priv, index, FALSE);
			if (new_block != block) {
				memcpy(new_block, block, block->size);

				new_block->index = index;
				new_block->lock_count = make_sure_if_no_referrer;

				_hg_allocator_bitmap_free(priv->bitmap, block->index, block->size);
			}
			new_block->size = obj_size;
			retval = quark;
			g_tree_replace(priv->block_in_use, HGQUARK_TO_POINTER (retval), new_block);
			if (make_sure_if_no_referrer == 2) {
				/* ensure the requirement of the one time unlock_object */
				_hg_allocator_real_unlock_object(new_block);
			}

			if (ret)
				*ret = hg_get_allocated_object (new_block);
		}
	} else {
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
		g_warning("%lx isn't the allocated object.\n", index);
#endif
	}

	G_UNLOCK (allocator);

	return retval;
}

void
_hg_allocator_free(hg_allocator_data_t *data,
		   hg_quark_t           index)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block;

	priv = (hg_allocator_private_t *)data;
	block = _hg_allocator_real_lock_object(data, index);
	if (block) {
		g_tree_remove(priv->block_in_use, HGQUARK_TO_POINTER (index));
		_hg_allocator_bitmap_free(priv->bitmap, block->index, block->size);
	} else {
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
		g_warning("%lx isn't the allocated object.\n", index);
#endif
	}
}

static gpointer
_hg_allocator_internal_lock_object(hg_allocator_private_t *priv,
				   hg_quark_t              index,
				   gboolean                initialize)
{
	hg_allocator_block_t *retval;

	retval = (hg_allocator_block_t *)((gulong)priv->heap + ((index - 1) * BLOCK_SIZE));
	if (initialize) {
		memset(retval, 0, sizeof (hg_allocator_block_t));
		retval->lock_count = 1;
	}
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("%s: %p\n", __FUNCTION__, retval);
#endif

	return retval;
}

static gpointer
_hg_allocator_real_lock_object(hg_allocator_data_t *data,
			       hg_quark_t           index)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *retval = NULL;
	gint old_val;

	priv = (hg_allocator_private_t *)data;
	if ((retval = g_tree_lookup(priv->block_in_use, HGQUARK_TO_POINTER (index))) != NULL) {
		old_val = g_atomic_int_exchange_and_add((int *)&retval->lock_count, 1);
	}

	return retval;
}

static gpointer
_hg_allocator_lock_object(hg_allocator_data_t *data,
			  hg_quark_t           index)
{
	hg_allocator_block_t *retval = NULL;

	G_LOCK (allocator);

	retval = _hg_allocator_real_lock_object(data, index);

	G_UNLOCK (allocator);
	if (retval)
		return hg_get_allocated_object (retval);

	return NULL;
}

static void
_hg_allocator_real_unlock_object(hg_allocator_block_t *block)
{
	gint old_val;

	hg_return_if_fail (block != NULL);
	hg_return_if_fail (block->lock_count > 0);

  retry_atomic_decrement:
	old_val = g_atomic_int_get(&block->lock_count);
	if (!g_atomic_int_compare_and_exchange((int *)&block->lock_count, old_val, old_val - 1))
		goto retry_atomic_decrement;
}

static void
_hg_allocator_unlock_object(hg_allocator_data_t *data,
			    hg_quark_t           index)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *retval = NULL;

	priv = (hg_allocator_private_t *)data;
	if ((retval = g_tree_lookup(priv->block_in_use, HGQUARK_TO_POINTER (index))) != NULL)
		_hg_allocator_real_unlock_object(retval);
}

/*< public >*/
hg_mem_vtable_t *
hg_allocator_get_vtable(void)
{
	return &__hg_allocator_vtable;
}
