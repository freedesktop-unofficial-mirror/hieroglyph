/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-libc.c
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

#include "hgallocator-libc.h"
#include "hgallocator-private.h"
#include "hgmem.h"


typedef struct _HieroGlyphAllocatorLibcPrivate		HgAllocatorLibcPrivate;


struct _HieroGlyphAllocatorLibcPrivate {
	GSList *chunk_list;
};


static gboolean       _hg_allocator_libc_real_initialize        (HgMemPool     *pool,
								 gsize          prealloc);
static gboolean       _hg_allocator_libc_real_destroy           (HgMemPool     *pool);
static gboolean       _hg_allocator_libc_real_resize_pool       (HgMemPool     *pool,
								 gsize          size);
static gpointer       _hg_allocator_libc_real_alloc             (HgMemPool     *pool,
								 gsize          size,
								 guint          flags);
static void           _hg_allocator_libc_real_free              (HgMemPool     *pool,
								 gpointer       data);
static gpointer       _hg_allocator_libc_real_resize            (HgMemObject   *object,
								 gsize          size);
static gsize          _hg_allocator_libc_real_get_size          (HgMemObject   *object);
static gboolean       _hg_allocator_libc_real_garbage_collection(HgMemPool     *pool);
static void           _hg_allocator_libc_real_gc_mark           (HgMemPool     *pool);
static gboolean       _hg_allocator_libc_real_is_safe_object    (HgMemPool     *pool,
								 HgMemObject   *object);
static HgMemSnapshot *_hg_allocator_libc_real_save_snapshot     (HgMemPool     *pool);
static gboolean       _hg_allocator_libc_real_restore_snapshot  (HgMemPool     *pool,
								 HgMemSnapshot *snapshot);


static HgAllocatorVTable __hg_allocator_libc_vtable = {
	.initialize         = _hg_allocator_libc_real_initialize,
	.destroy            = _hg_allocator_libc_real_destroy,
	.resize_pool        = _hg_allocator_libc_real_resize_pool,
	.alloc              = _hg_allocator_libc_real_alloc,
	.free               = _hg_allocator_libc_real_free,
	.resize             = _hg_allocator_libc_real_resize,
	.get_size           = _hg_allocator_libc_real_get_size,
	.garbage_collection = _hg_allocator_libc_real_garbage_collection,
	.gc_mark            = _hg_allocator_libc_real_gc_mark,
	.is_safe_object     = _hg_allocator_libc_real_is_safe_object,
	.save_snapshot      = _hg_allocator_libc_real_save_snapshot,
	.restore_snapshot   = _hg_allocator_libc_real_restore_snapshot,
};


/* dummy allocator with libc */
static gboolean
_hg_allocator_libc_real_initialize(HgMemPool *pool,
				   gsize      prealloc)
{
	HgAllocatorLibcPrivate *priv;

	priv = g_new0(HgAllocatorLibcPrivate, 1);
	if (priv == NULL)
		return FALSE;

	pool->total_heap_size = pool->initial_heap_size = prealloc;

	priv->chunk_list = NULL;

	pool->allocator->private = priv;

	return TRUE;
}

static gboolean
_hg_allocator_libc_real_destroy(HgMemPool *pool)
{
	HgAllocatorLibcPrivate *priv = pool->allocator->private;
	GSList *l = priv->chunk_list;

	while (l) {
		HgMemObject *obj = l->data;

		hg_mem_free(obj->data);
		l = g_slist_next(l);
	}

	g_slist_free(priv->chunk_list);
	g_free(priv);

	return TRUE;
}

static gboolean
_hg_allocator_libc_real_resize_pool(HgMemPool *pool,
				    gsize      size)
{
	pool->total_heap_size += size;

	return TRUE;
}

static gpointer
_hg_allocator_libc_real_alloc(HgMemPool *pool,
			      gsize      size,
			      guint      flags)
{
	HgAllocatorLibcPrivate *priv = pool->allocator->private;
	HgMemObject *retval;

	size += sizeof (HgMemObject);

	retval = g_malloc(size);
	if (!retval)
		return NULL;

	HG_SET_MAGIC_CODE (retval, HG_MEM_HEADER);
	retval->subid = 0;
	retval->pool = pool;
	HG_MEMOBJ_INIT_FLAGS (retval);
	HG_MEMOBJ_SET_HEAP_ID (retval, 0);
	if ((flags & HG_FL_HGOBJECT) != 0)
		HG_MEMOBJ_SET_HGOBJECT_ID (retval);
	HG_MEMOBJ_SET_FLAGS (retval, flags);

	priv->chunk_list = g_slist_append(priv->chunk_list, retval);

	return retval->data;
}

static void
_hg_allocator_libc_real_free(HgMemPool *pool,
			     gpointer   data)
{
	HgAllocatorLibcPrivate *priv = pool->allocator->private;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL) {
		g_warning("[BUG] Unknown object %p is going to be destroyed.", data);
		return;
	}
	priv->chunk_list = g_slist_remove(priv->chunk_list, obj);
	g_free(obj);
}

static gpointer
_hg_allocator_libc_real_resize(HgMemObject *object,
			       gsize        size)
{
	return NULL;
}

static gsize
_hg_allocator_libc_real_get_size(HgMemObject *object)
{
	return 0;
}

static gboolean
_hg_allocator_libc_real_garbage_collection(HgMemPool *pool)
{
	return FALSE;
}

static void
_hg_allocator_libc_real_gc_mark(HgMemPool *pool)
{
}

static gboolean
_hg_allocator_libc_real_is_safe_object(HgMemPool   *pool,
				       HgMemObject *object)
{
	HgAllocatorLibcPrivate *priv = pool->allocator->private;

	return g_slist_find(priv->chunk_list, object) != NULL;
}

static HgMemSnapshot *
_hg_allocator_libc_real_save_snapshot(HgMemPool *pool)
{
	return NULL;
}

static gboolean
_hg_allocator_libc_real_restore_snapshot(HgMemPool     *pool,
					 HgMemSnapshot *snapshot)
{
	return FALSE;
}

/*
 * Public Functions
 */
HgAllocatorVTable *
hg_allocator_libc_get_vtable(void)
{
	return &__hg_allocator_libc_vtable;
}
