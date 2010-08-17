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

#include <stdlib.h>
#include <string.h>
#include "hgerror.h"
#include "hgquark.h"
#include "hgallocator.h"
#include "hgallocator-private.h"

#define DEFAULT_RESIZE_SIZE	65535


G_INLINE_FUNC hg_allocator_bitmap_t *_hg_allocator_bitmap_new        (gsize                    size);
G_INLINE_FUNC void                   _hg_allocator_bitmap_destroy    (gpointer                 data);
G_INLINE_FUNC gboolean               _hg_allocator_bitmap_resize     (hg_allocator_bitmap_t   *bitmap,
                                                                      gsize                    size);
G_INLINE_FUNC hg_quark_t             _hg_allocator_bitmap_alloc      (hg_allocator_bitmap_t   *bitmap,
                                                                      gsize                    size);
G_INLINE_FUNC hg_quark_t             _hg_allocator_bitmap_realloc    (hg_allocator_bitmap_t   *bitmap,
                                                                      hg_quark_t               index,
                                                                      gsize                    old_size,
                                                                      gsize                    size);
G_INLINE_FUNC void                   _hg_allocator_bitmap_free       (hg_allocator_bitmap_t   *bitmap,
                                                                      hg_quark_t               index,
                                                                      gsize                    size);
G_INLINE_FUNC void                   _hg_allocator_bitmap_mark       (hg_allocator_bitmap_t   *bitmap,
                                                                      gint32                   index);
G_INLINE_FUNC void                   _hg_allocator_bitmap_clear      (hg_allocator_bitmap_t   *bitmap,
                                                                      gint32                   index);
G_INLINE_FUNC gboolean               _hg_allocator_bitmap_is_marked  (hg_allocator_bitmap_t   *bitmap,
                                                                      gint32                   index);
G_INLINE_FUNC gboolean               _hg_allocator_bitmap_range_mark (hg_allocator_bitmap_t   *bitmap,
                                                                      gint32                  *index,
                                                                      gsize                    size);
G_INLINE_FUNC void                   _hg_allocator_bitmap_dump       (hg_allocator_bitmap_t   *bitmap);
static gpointer                      _hg_allocator_initialize        (void);
static void                          _hg_allocator_finalize          (hg_allocator_data_t     *data);
static gboolean                      _hg_allocator_resize_heap       (hg_allocator_data_t     *data,
                                                                      gsize                    size);
static hg_quark_t                    _hg_allocator_alloc             (hg_allocator_data_t     *data,
                                                                      gsize                    size,
                                                                      gpointer                *ret);
static hg_quark_t                    _hg_allocator_realloc           (hg_allocator_data_t     *data,
                                                                      hg_quark_t               quark,
                                                                      gsize                    size,
                                                                      gpointer                *ret);
static void                          _hg_allocator_free              (hg_allocator_data_t     *data,
                                                                      hg_quark_t               index);
G_INLINE_FUNC gpointer               _hg_allocator_get_internal_block(hg_allocator_private_t  *data,
                                                                      hg_quark_t               index,
                                                                      gboolean                 initialize);
G_INLINE_FUNC gpointer               _hg_allocator_real_lock_object  (hg_allocator_data_t     *data,
                                                                      hg_quark_t               index);
static gpointer                      _hg_allocator_lock_object       (hg_allocator_data_t     *data,
                                                                      hg_quark_t               index);
G_INLINE_FUNC void                   _hg_allocator_real_unlock_object(hg_allocator_block_t    *block);
static void                          _hg_allocator_unlock_object     (hg_allocator_data_t     *data,
                                                                      hg_quark_t               index);
static gboolean                      _hg_allocator_gc_init           (hg_allocator_data_t     *data);
static gboolean                      _hg_allocator_gc_mark           (hg_allocator_data_t     *data,
                                                                      hg_quark_t               index,
                                                                      GError                 **error);
static gboolean                      _hg_allocator_gc_finish         (hg_allocator_data_t     *data,
                                                                      gboolean                 was_error);


static hg_mem_vtable_t __hg_allocator_vtable = {
	.initialize    = _hg_allocator_initialize,
	.finalize      = _hg_allocator_finalize,
	.resize_heap   = _hg_allocator_resize_heap,
	.alloc         = _hg_allocator_alloc,
	.realloc       = _hg_allocator_realloc,
	.free          = _hg_allocator_free,
	.lock_object   = _hg_allocator_lock_object,
	.unlock_object = _hg_allocator_unlock_object,
	.gc_init       = _hg_allocator_gc_init,
	.gc_mark       = _hg_allocator_gc_mark,
	.gc_finish     = _hg_allocator_gc_finish,
};
G_LOCK_DEFINE_STATIC (bitmap);
G_LOCK_DEFINE_STATIC (allocator);

/*< private >*/

/** bitmap operation **/
G_INLINE_FUNC hg_allocator_bitmap_t *
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

G_INLINE_FUNC void
_hg_allocator_bitmap_destroy(gpointer data)
{
	hg_allocator_bitmap_t *bitmap = data;

	if (!data)
		return;
	g_free(bitmap->bitmaps);
	g_free(bitmap);
}

G_INLINE_FUNC gboolean
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

G_INLINE_FUNC hg_quark_t
_hg_allocator_bitmap_alloc(hg_allocator_bitmap_t *bitmap,
			   gsize                  size)
{
	hg_quark_t i, idx = 0;
	gsize aligned_size;
	gboolean retry = FALSE;
	gint32 j;

	hg_return_val_if_fail (bitmap != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	aligned_size = hg_mem_aligned_to(size, BLOCK_SIZE) / BLOCK_SIZE;
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("ALLOC: %" G_GSIZE_FORMAT " blocks required\n", aligned_size);
	_hg_allocator_bitmap_dump(bitmap);
#endif
#if 0
	if (aligned_size >= bitmap->last_allocated_size) {
		/* that may be less likely to get a space
		 * prior to the place where allocated last time
		 */
		idx = bitmap->last_index;
	} else {
		bitmap->last_allocated_size = 0;
		bitmap->last_index = 0;
	}
#else
	/* XXX: it seems working fine more than above */
	idx = bitmap->last_index;
#endif
  find_free_bitmap:
	for (i = idx; i < bitmap->size; i++) {
		j = i + 1;
		if (_hg_allocator_bitmap_range_mark(bitmap, &j, aligned_size)) {
			bitmap->last_allocated_size = aligned_size;
			bitmap->last_index = ((i + 1) >= bitmap->size ? 0 : i);

#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
			g_print("Allocated at %" G_GSIZE_FORMAT "\n", i);
#endif

			return i + 1;
		} else {
			i = j;
		}
	}
	if (idx != 0 && !retry) {
		/* retry to find out a free space at the beginning again */
		retry = TRUE;
		idx = 0;
		goto find_free_bitmap;
	}

	return Qnil;
}

G_INLINE_FUNC hg_quark_t
_hg_allocator_bitmap_realloc(hg_allocator_bitmap_t *bitmap,
			     hg_quark_t             index,
			     gsize                  old_size,
			     gsize                  size)
{
	hg_quark_t i;
	gssize aligned_size, old_aligned_size, required_size;

	hg_return_val_if_fail (bitmap != NULL, Qnil);
	hg_return_val_if_fail (index != Qnil, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	aligned_size = hg_mem_aligned_to(size, BLOCK_SIZE) / BLOCK_SIZE;
	old_aligned_size = hg_mem_aligned_to(old_size, BLOCK_SIZE) / BLOCK_SIZE;
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("ALLOC: %" G_GSIZE_FORMAT " blocks to be grown from %" G_GSIZE_FORMAT " blocks at index %" G_GSIZE_FORMAT "\n", aligned_size, old_aligned_size, index);
	_hg_allocator_bitmap_dump(bitmap);
#endif
	required_size = aligned_size - old_aligned_size;
	if (required_size < 0) {
		G_LOCK (bitmap);

		for (i = index + aligned_size; i < (index + old_aligned_size); i++)
			_hg_allocator_bitmap_clear(bitmap, i);

		G_UNLOCK (bitmap);

		return index;
	}
	if (!_hg_allocator_bitmap_is_marked(bitmap, index + old_aligned_size)) {
		required_size--;
		for (i = index + old_aligned_size + 1; required_size > 0 && i < bitmap->size; i++) {
			if (!_hg_allocator_bitmap_is_marked(bitmap, i)) {
				required_size--;
			} else {
				break;
			}
		}
	}
	if (required_size == 0) {
		G_LOCK (bitmap);

		for (i = index + old_aligned_size; i < (index + aligned_size); i++)
			_hg_allocator_bitmap_mark(bitmap, i);

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

G_INLINE_FUNC void
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
	if (aligned_size >= bitmap->last_allocated_size) {
		bitmap->last_index = i - 1;
		bitmap->last_allocated_size = aligned_size;
	}

	G_UNLOCK (bitmap);

#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("After freed");
	_hg_allocator_bitmap_dump(bitmap);
#endif
}

G_INLINE_FUNC void
_hg_allocator_bitmap_mark(hg_allocator_bitmap_t *bitmap,
			  gint32                 index)
{
	hg_return_if_fail (bitmap != NULL);
	hg_return_if_fail (index > 0);
	hg_return_if_fail (!_hg_allocator_bitmap_is_marked(bitmap, index));

	bitmap->bitmaps[(index - 1) / sizeof (gint32)] |= 1 << ((index - 1) % sizeof (gint32));
}

G_INLINE_FUNC void
_hg_allocator_bitmap_clear(hg_allocator_bitmap_t *bitmap,
			   gint32                 index)
{
	hg_return_if_fail (bitmap != NULL);
	hg_return_if_fail (index > 0);

	bitmap->bitmaps[(index - 1) / sizeof (gint32)] &= ~(1 << ((index - 1) % sizeof (gint32)));
}

G_INLINE_FUNC gboolean
_hg_allocator_bitmap_is_marked(hg_allocator_bitmap_t *bitmap,
			       gint32                 index)
{
	hg_return_val_if_fail (bitmap != NULL, FALSE);
	hg_return_val_if_fail (index > 0, FALSE);

	return bitmap->bitmaps[(index - 1) / sizeof (gint32)] & 1 << ((index - 1) % sizeof (gint32));
}

G_INLINE_FUNC gboolean
_hg_allocator_bitmap_range_mark(hg_allocator_bitmap_t *bitmap,
				gint32                *index,
				gsize                  size)
{
	gsize required_size = size, j;

	if (!_hg_allocator_bitmap_is_marked(bitmap, *index)) {
		required_size--;
		for (j = *index; required_size > 0 && j < bitmap->size; j++) {
			if (!_hg_allocator_bitmap_is_marked(bitmap, j + 1)) {
				required_size--;
			} else {
				break;
			}
		}
		if (required_size == 0) {
			G_LOCK (bitmap);

			for (j = *index; j < (*index + size); j++)
				_hg_allocator_bitmap_mark(bitmap, j);

			G_UNLOCK (bitmap);

			return TRUE;
		} else {
			*index = j;
		}
	} else {
		(*index)--;
	}

	return FALSE;
}

G_INLINE_FUNC void
_hg_allocator_bitmap_dump(hg_allocator_bitmap_t *bitmap)
{
	gsize i;

	g_print("bitmap: %" G_GSIZE_FORMAT " blocks allocated\n", bitmap->size);
	g_print("        :         1         2         3         4         5         6         7\n");
	g_print("        :12345678901234567890123456789012345678901234567890123456789012345678901234567890");
	for (i = 0; i < bitmap->size; i++) {
		if (i % 70 == 0)
			g_print("\n%08lx:", i);
		g_print("%d", _hg_allocator_bitmap_is_marked(bitmap, i + 1) ? 1 : 0);
	}
	g_print("\n");
}

/** allocator **/
static gpointer
_hg_allocator_initialize(void)
{
	hg_allocator_private_t *retval;

	retval = g_new0(hg_allocator_private_t, 1);

	return retval;
}

static void
_hg_allocator_finalize(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv;

	if (!data)
		return;

	priv = (hg_allocator_private_t *)data;
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

	obj_size = hg_mem_aligned_size (sizeof (hg_allocator_block_t) + size);
  retry:
	index = _hg_allocator_bitmap_alloc(priv->bitmap, obj_size);
	if (index != Qnil) {
		/* Update the used size */
		data->used_size += obj_size;

		block = _hg_allocator_get_internal_block(priv, index, TRUE);
		block->size = obj_size;
		retval = index;
		/* NOTE: No unlock yet here.
		 *       any objects are supposed to be unlocked
		 *       when it's entered into the stack where PostScript
		 *       VM manages. otherwise it will be swept by GC.
		 */
		if (ret)
			*ret = hg_get_allocated_object (block);
		else
			_hg_allocator_real_unlock_object(block);
	} else {
		if (data->resizable) {
			gsize resize_size = MAX (size, DEFAULT_RESIZE_SIZE);

			if (resize_size == size) {
				/* We'll be here soon */
				resize_size = MAX (resize_size, data->total_size);
			}
			if (_hg_allocator_resize_heap(data, data->total_size + resize_size))
				goto retry;
		}
	}

	return retval;
}

static hg_quark_t
_hg_allocator_realloc(hg_allocator_data_t *data,
		      hg_quark_t           qdata,
		      gsize                size,
		      gpointer            *ret)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block, *new_block;
	gsize obj_size;
	hg_quark_t index = Qnil, retval = Qnil;
	gboolean retried = FALSE;

	G_LOCK (allocator);

	priv = (hg_allocator_private_t *)data;

	obj_size = hg_mem_aligned_size (sizeof (hg_allocator_block_t) + size);
	block = _hg_allocator_real_lock_object(data, qdata);
	if (block) {
		volatile gint make_sure_if_no_referrer;

		make_sure_if_no_referrer = g_atomic_int_get(&block->lock_count);
		hg_return_val_after_eval_if_fail (make_sure_if_no_referrer == 1, Qnil, _hg_allocator_real_unlock_object(block));

	  retry:
		index = _hg_allocator_bitmap_realloc(priv->bitmap, qdata, block->size, obj_size);
		if (index != Qnil) {
			/* Update the used size */
			data->used_size += (obj_size - block->size);

			new_block = _hg_allocator_get_internal_block(priv, index, FALSE);
			if (new_block != block) {
				memcpy(new_block, block, block->size);

				new_block->lock_count = make_sure_if_no_referrer;

				_hg_allocator_bitmap_free(priv->bitmap, qdata, block->size);
			}
			new_block->size = obj_size;
			retval = index;

			if (ret)
				*ret = hg_get_allocated_object (new_block);
			else
				_hg_allocator_real_unlock_object(new_block);
		} else {
			if (data->resizable && !retried) {
				gsize resize_size = MAX (size, DEFAULT_RESIZE_SIZE);

				if (resize_size == size) {
					/* We'll be here soon */
					resize_size = MAX (resize_size, data->total_size);
				}
				if (_hg_allocator_resize_heap(data, data->total_size + resize_size)) {
					retried = TRUE;
					goto retry;
				}
			}
			_hg_allocator_real_unlock_object(block);
		}
	} else {
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
		g_warning("%lx isn't the allocated object.\n", qdata);
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
		/* Update the used size */
		data->used_size -= block->size;

		_hg_allocator_bitmap_free(priv->bitmap, index, block->size);
	} else {
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
		g_warning("%lx isn't the allocated object.\n", index);
#endif
	}
}

G_INLINE_FUNC gpointer
_hg_allocator_get_internal_block(hg_allocator_private_t *priv,
				 hg_quark_t              index,
				 gboolean                initialize)
{
	hg_allocator_block_t *retval;
	gsize idx = (index - 1) * BLOCK_SIZE;

	if (idx > ((hg_allocator_data_t *)priv)->total_size) {
		gchar *s = hg_get_stacktrace();

		g_warning("Invalid quark to access: 0x%lx\nStack trace:\n%s", index, s);
		return NULL;
	}
	retval = (hg_allocator_block_t *)((gulong)priv->heap + idx);
	if (initialize) {
		memset(retval, 0, sizeof (hg_allocator_block_t));
		retval->lock_count = 1;
	}
#if defined(HG_DEBUG) && defined(HG_MEM_DEBUG)
	g_print("%s: %p\n", __FUNCTION__, retval);
#endif

	return retval;
}

G_INLINE_FUNC gpointer
_hg_allocator_real_lock_object(hg_allocator_data_t *data,
			       hg_quark_t           index)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *retval = NULL;
	gint old_val;

	priv = (hg_allocator_private_t *)data;
	if (_hg_allocator_bitmap_is_marked(priv->bitmap, index)) {
		/* XXX: maybe better validate the block size too? */
		retval = _hg_allocator_get_internal_block(priv, index, FALSE);
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

G_INLINE_FUNC void
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

	if (_hg_allocator_bitmap_is_marked(priv->bitmap, index)) {
		/* XXX: maybe better validate the block size too? */
		retval = _hg_allocator_get_internal_block(priv, index, FALSE);
		_hg_allocator_real_unlock_object(retval);
	}
}

static gboolean
_hg_allocator_gc_init(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;

	if (priv->slave_bitmap) {
		g_warning("GC is already ongoing.");
		return FALSE;
	}
	priv->slave_bitmap = _hg_allocator_bitmap_new(priv->bitmap->size * BLOCK_SIZE);
	priv->slave.total_size = data->total_size;
	priv->slave.used_size = 0;

	return TRUE;
}

static gboolean
_hg_allocator_gc_mark(hg_allocator_data_t  *data,
		      hg_quark_t            index,
		      GError              **error)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_allocator_block_t *block;
	gint32 q;
	gboolean retval = TRUE;
	GError *err = NULL;
	gsize aligned_size;

	/* this isn't the object in the targeted spool */
	if (!priv->slave_bitmap)
		return TRUE;

	block = _hg_allocator_real_lock_object(data, index);
	if (block) {
		q = index;
		aligned_size = hg_mem_aligned_to(block->size, BLOCK_SIZE) / BLOCK_SIZE;
		if (_hg_allocator_bitmap_range_mark(priv->slave_bitmap, &q, aligned_size)) {
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
			g_print("GC: Marked index %ld, size: %ld\n", index, aligned_size);
			_hg_allocator_bitmap_dump(priv->slave_bitmap);
#endif
			priv->slave.used_size += block->size;
		} else {
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
			g_print("GC: already marked index %ld\n", index);
#endif
		}
		_hg_allocator_real_unlock_object(block);
	} else {
		g_set_error(&err, HG_ERROR, EINVAL,
			    "%lx isn't an allocated object from this spool", index);
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

static gboolean
_hg_allocator_gc_finish(hg_allocator_data_t *data,
			gboolean             was_error)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;

	if (!priv->slave_bitmap) {
		g_warning("GC isn't yet started.");
		return FALSE;
	}

	G_LOCK (allocator);

	/* XXX: this has to be dropped in the future */
	if (!was_error) {
		gsize i, used_size;

		used_size = data->used_size - priv->slave.used_size;

#define HG_GC_DEBUG
#if defined (HG_DEBUG) && defined (HG_GC_DEBUG)
		g_print("GC: marking the locked objects\n");
#endif
		/* give aid to the locked blocks */
		for (i = 0; used_size > 0 && i < priv->bitmap->size; i++) {
			if (_hg_allocator_bitmap_is_marked(priv->bitmap, i + 1)) {
				hg_allocator_block_t *block;
				gsize aligned_size;

				block = _hg_allocator_get_internal_block((hg_allocator_private_t *)data,
									 i + 1, FALSE);

				if (block == NULL) {
					was_error = TRUE;
					break;
				}
				if (!_hg_allocator_bitmap_is_marked(priv->slave_bitmap, i + 1)) {
					used_size -= block->size;
					if (block->lock_count > 0) {
						g_warning("[BUG] locked block without references 0x%lx [size: %ld, count: %d]\n",
							  i + 1, block->size, block->lock_count);
#if defined (HG_DEBUG) && defined (HG_GC_DEBUG)
						abort();
#endif
						was_error = !_hg_allocator_gc_mark(data, i + 1, NULL);
						if (was_error)
							break;
					}
				}
				aligned_size = hg_mem_aligned_to(block->size, BLOCK_SIZE) / BLOCK_SIZE;
				i += aligned_size - 1;
			}
		}
	}

#if defined (HG_DEBUG) && defined (HG_GC_DEBUG)
	if (!was_error)
		g_print("GC: %ld -> %ld (%ld bytes freed) / %ld\n",
			data->used_size, priv->slave.used_size,
			data->used_size - priv->slave.used_size,
			data->total_size);
	else
		g_print("GC: failed.\n");
#endif

	if (was_error) {
		_hg_allocator_bitmap_destroy(priv->slave_bitmap);
		priv->slave_bitmap = NULL;
	} else {
		_hg_allocator_bitmap_destroy(priv->bitmap);
		priv->bitmap = priv->slave_bitmap;
		priv->slave_bitmap = NULL;
		data->used_size = priv->slave.used_size;
	}

	G_UNLOCK (allocator);

	return !was_error;
}

/*< public >*/
hg_mem_vtable_t *
hg_allocator_get_vtable(void)
{
	return &__hg_allocator_vtable;
}
