/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include "hgmem.h"
#include "hgallocator-private.h"
#include "hgbtree.h"

#define VTABLE_TREE_N_NODE	3

gpointer _hg_stack_start = NULL;
gpointer _hg_stack_end = NULL;
static gboolean hg_mem_is_initialized = FALSE;
static GAllocator *hg_mem_g_list_allocator = NULL;
static HgBTree *_hg_object_vtable_tree = NULL;
static GPtrArray *_hg_object_vtable_array = NULL;

G_LOCK_DEFINE_STATIC (hgobject);

/*
 * Private Functions
 */

/*
 * initializer
 */
#ifdef USE_SYSDEP_CODE
static void
_hg_mem_init_stack_start(void)
{
#if 0
#define STAT_BUFSIZE	4096
#define STAT_SKIP	27
	int fd, i;
	char stat_buffer[STAT_BUFSIZE];
	char c;
	guint offset = 0;
	gsize result = 0;

	if ((fd = open("/proc/self/stat", O_RDONLY)) == -1 ||
	    read(fd, stat_buffer, STAT_BUFSIZE) < 2 * STAT_SKIP) {
		g_error("Failed to read /proc/self/stat");
	} else {
		c = stat_buffer[offset++];
		for (i = 0; i < STAT_SKIP; i++) {
			while (isspace(c)) c = stat_buffer[offset++];
			while (!isspace(c)) c = stat_buffer[offset++];
		}
		while (isspace(c)) c = stat_buffer[offset++];
		while (isdigit(c)) {
			result *= 10;
			result += c - '0';
			c = stat_buffer[offset++];
		}
		close(fd);
		if (result < 0x10000000)
			g_error("the stack bottom may be invalid: %x.", result);
		_hg_stack_start = (gpointer)result;
	}
#else
	extern int *__libc_stack_end;
	/* FIXME: the above code somehow doesn't work on valgrind */
	_hg_stack_start = __libc_stack_end;
#endif
}
#endif

/* memory pool */
static void
_hg_mem_pool_free(HgMemPool *pool)
{
	gint i;

	for (i = 0; i < pool->n_heaps; i++) {
		HgHeap *heap = g_ptr_array_index(pool->heap_list, i);

		hg_heap_free(heap);
	}
	g_free(pool->name);
	g_ptr_array_free(pool->heap_list, TRUE);
	g_free(pool);
}

/*
 * Public Functions
 */

/* allocator */
HgAllocator *
hg_allocator_new(const HgAllocatorVTable *vtable)
{
	HgAllocator *retval;

	retval = g_new(HgAllocator, 1);
	retval->private = NULL;
	retval->used    = FALSE;
	retval->vtable  = vtable;

	return retval;
}

void
hg_allocator_destroy(HgAllocator *allocator)
{
	g_return_if_fail (allocator != NULL);
	g_return_if_fail (!allocator->used);

	g_free(allocator);
}

HgHeap *
hg_heap_new(HgMemPool *pool,
	    gsize      size)
{
	HgHeap *retval = g_new(HgHeap, 1);

	if (retval != NULL) {
		retval->heaps = g_malloc(size);
		if (retval->heaps == NULL) {
			g_free(retval);
			return NULL;
		}
		retval->total_heap_size = size;
		retval->used_heap_size = 0;
		retval->serial = pool->n_heaps++;
	}

	return retval;
}

void
hg_heap_free(HgHeap *heap)
{
	g_return_if_fail (heap != NULL);

	if (heap->heaps)
		g_free(heap->heaps);
	g_free(heap);
}

/* initializer */
void
hg_mem_init_stack_start(gpointer mark)
{
	_hg_stack_start = mark;
}

void
hg_mem_init(void)
{
	g_return_if_fail (_hg_stack_start != NULL);

	if (!hg_mem_is_initialized) {
		if (!hg_mem_g_list_allocator) {
			hg_mem_g_list_allocator = g_allocator_new("Default GAllocator for GList", 128);
			g_list_push_allocator(hg_mem_g_list_allocator);
		}
		if (!_hg_object_vtable_tree) {
			_hg_object_vtable_tree = hg_btree_new(VTABLE_TREE_N_NODE);
			if (_hg_object_vtable_tree == NULL) {
				g_warning("Failed to initialize VTable tree.");
				return;
			}
			_hg_object_vtable_array = g_ptr_array_new();
		}
		hg_mem_is_initialized = TRUE;
	}
}

void
hg_mem_finalize(void)
{
	if (hg_mem_is_initialized) {
		g_list_pop_allocator();
		g_allocator_free(hg_mem_g_list_allocator);
		hg_btree_destroy(_hg_object_vtable_tree);
		g_ptr_array_free(_hg_object_vtable_array, TRUE);
		hg_mem_g_list_allocator = NULL;
		_hg_object_vtable_tree = NULL;
		_hg_object_vtable_array = NULL;
		hg_mem_is_initialized = FALSE;
	}
}

void
hg_mem_set_stack_end(gpointer mark)
{
	_hg_stack_end = mark;
}

/* memory pool */
HgMemPool *
hg_mem_pool_new(HgAllocator *allocator,
		const gchar *identity,
		gsize        prealloc,
		gboolean     allow_resize)
{
	HgMemPool *pool;

	g_return_val_if_fail (_hg_stack_start != NULL, NULL);
	g_return_val_if_fail (allocator != NULL, NULL);
	g_return_val_if_fail (allocator->vtable->initialize != NULL &&
			      allocator->vtable->alloc != NULL &&
			      allocator->vtable->free != NULL, NULL);
	g_return_val_if_fail (identity != NULL, NULL);
	g_return_val_if_fail (prealloc > 0, NULL);
	g_return_val_if_fail (!allow_resize ||
			      (allow_resize && allocator->vtable->resize_pool != NULL), NULL);
	g_return_val_if_fail (!allocator->used, NULL);

	pool = (HgMemPool *)g_new(HgMemPool, 1);
	if (pool == NULL) {
		g_error("Failed to allocate a memory pool for %s", identity);
		return NULL;
	}
	pool->name = g_strdup(identity);
	pool->heap_list = g_ptr_array_new();
	pool->n_heaps = 0;
	pool->initial_heap_size = 0;
	pool->total_heap_size = 0;
	pool->used_heap_size = 0;
	pool->allow_resize = allow_resize;
	pool->access_mode = HG_ST_READABLE | HG_ST_WRITABLE;
	pool->destroyed = FALSE;
	pool->allocator = allocator;
	pool->root_node = NULL;
	pool->other_pool_ref_list = NULL;
	pool->periodical_gc = FALSE;
	pool->gc_checked = FALSE;
	pool->use_gc = TRUE;
	pool->is_global_mode = FALSE;
	pool->is_processing = FALSE;
	pool->is_collecting = FALSE;
	pool->gc_threshold = 50;
	pool->age_of_gc_mark = 0;
	allocator->used = TRUE;
	if (!allocator->vtable->initialize(pool, prealloc)) {
		_hg_mem_pool_free(pool);
		return NULL;
	}

	return pool;
}

void
hg_mem_pool_destroy(HgMemPool *pool)
{
	g_return_if_fail (pool != NULL);

	pool->destroyed = TRUE;
	if (pool->allocator->vtable->destroy) {
		pool->allocator->vtable->destroy(pool);
	}
	if (pool->root_node) {
		g_list_free(pool->root_node);
	}
	if (pool->other_pool_ref_list) {
		g_list_free(pool->other_pool_ref_list);
	}
	pool->allocator->used = FALSE;
	_hg_mem_pool_free(pool);
}

const gchar *
hg_mem_pool_get_name(HgMemPool *pool)
{
	return pool->name;
}

gboolean
hg_mem_pool_allow_resize(HgMemPool *pool,
			 gboolean   flag)
{
	g_return_val_if_fail (pool != NULL, FALSE);
	g_return_val_if_fail (!flag ||
			      (flag && pool->allocator->vtable->resize_pool != NULL), FALSE);

	pool->allow_resize = flag;

	return TRUE;
}

gsize
hg_mem_pool_get_used_heap_size(HgMemPool *pool)
{
	g_return_val_if_fail (pool != NULL, 0);

	return pool->used_heap_size;
}

gsize
hg_mem_pool_get_free_heap_size(HgMemPool *pool)
{
	g_return_val_if_fail (pool != NULL, 0);

	return pool->total_heap_size - pool->used_heap_size;
}

void
hg_mem_pool_add_heap(HgMemPool *pool,
		     HgHeap    *heap)
{
	guint i;

	g_return_if_fail (pool != NULL);
	g_return_if_fail (heap != NULL);

	for (i = 0; i < pool->heap_list->len; i++) {
		HgHeap *h = g_ptr_array_index(pool->heap_list, i);

		g_return_if_fail (h != heap);
	}

	g_ptr_array_add(pool->heap_list, heap);
}

void
hg_mem_pool_use_periodical_gc(HgMemPool *pool,
			      gboolean   flag)
{
	g_return_if_fail (pool != NULL);

	pool->periodical_gc = flag;
}

void
hg_mem_pool_use_garbage_collection(HgMemPool *pool,
				   gboolean   flag)
{
	g_return_if_fail (pool != NULL);

	pool->use_gc = flag;
}

guint
hg_mem_pool_get_default_access_mode(HgMemPool *pool)
{
	g_return_val_if_fail (pool != NULL, 0);

	return pool->access_mode;
}

void
hg_mem_pool_set_default_access_mode(HgMemPool *pool,
				    guint      state)
{
	g_return_if_fail (pool != NULL);

	pool->access_mode = state;
}

gboolean
hg_mem_pool_is_global_mode(HgMemPool *pool)
{
	g_return_val_if_fail (pool != NULL, FALSE);

	return pool->is_global_mode;
}

void
hg_mem_pool_use_global_mode(HgMemPool *pool,
			    gboolean   flag)
{
	g_return_if_fail (pool != NULL);

	pool->is_global_mode = flag;
}

gboolean
_hg_mem_pool_is_own_memobject(HgMemPool   *pool,
			      HgMemObject *obj)
{
	gint i;
	gboolean retval = FALSE;

	for (i = 0; i < pool->n_heaps; i++) {
		HgHeap *heap = g_ptr_array_index(pool->heap_list, i);

		if ((gsize)obj >= (gsize)heap->heaps &&
		    (gsize)obj <= ((gsize)heap->heaps + heap->total_heap_size)) {
			retval = TRUE;
			break;
		}
	}

	return retval;
}

gboolean
hg_mem_pool_is_own_object(HgMemPool *pool,
			  gpointer   data)
{
	HgMemObject *obj;

	g_return_val_if_fail (pool != NULL, FALSE);

	if (!hg_mem_pool_is_global_mode(pool)) {
		/* always return true */
		return TRUE;
	}
	hg_mem_get_object__inline(data, obj);
	if (g_list_find(obj->pool->root_node, data) != NULL) {
		/* We privilege the object that is already in the root node */
		return TRUE;
	}

	return _hg_mem_pool_is_own_memobject(pool, obj);
}

HgMemSnapshot *
hg_mem_pool_save_snapshot(HgMemPool *pool)
{
	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (pool->allocator->vtable->save_snapshot != NULL, NULL);

	return pool->allocator->vtable->save_snapshot(pool);
}

gboolean
hg_mem_pool_restore_snapshot(HgMemPool     *pool,
			     HgMemSnapshot *snapshot)
{
	g_return_val_if_fail (pool != NULL, FALSE);
	g_return_val_if_fail (snapshot != NULL, FALSE);
	g_return_val_if_fail (pool->allocator->vtable->restore_snapshot != NULL, FALSE);

	return pool->allocator->vtable->restore_snapshot(pool, snapshot);
}

gboolean
hg_mem_garbage_collection(HgMemPool *pool)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (pool != NULL, FALSE);

	if (pool->allocator->vtable->garbage_collection)
		retval = pool->allocator->vtable->garbage_collection(pool);

	return retval;
}

gpointer
hg_mem_alloc(HgMemPool *pool,
	     gsize      size)
{
	return hg_mem_alloc_with_flags(pool, size, 0);
}

gpointer
hg_mem_alloc_with_flags(HgMemPool *pool,
			gsize      size,
			guint      flags)
{
	gpointer retval;

	g_return_val_if_fail (pool != NULL, NULL);

	if (pool->periodical_gc) {
		if (!pool->gc_checked &&
		    (pool->used_heap_size * 100 / pool->total_heap_size) > pool->gc_threshold) {
			if (!hg_mem_garbage_collection(pool)) {
				pool->gc_threshold += 5;
			} else {
				pool->gc_checked = TRUE;
				if ((pool->used_heap_size * 100 / pool->total_heap_size) > pool->gc_threshold) {
					pool->gc_threshold += 5;
				} else {
					pool->gc_threshold -= 5;
				}
			}
		} else {
			pool->gc_threshold -= 5;
			pool->gc_checked = FALSE;
		}
		if (pool->gc_threshold < 50)
			pool->gc_threshold = 50;
		if (pool->gc_threshold > 90)
			pool->gc_threshold = 90;
	}
	retval = pool->allocator->vtable->alloc(pool, size, flags);
	if (!retval) {
		if (hg_mem_garbage_collection(pool)) {
			/* retry */
			retval = pool->allocator->vtable->alloc(pool, size, flags);
		}
	}
	if (!retval) {
		/* try growing the heap up when still failed */
		if (pool->allow_resize &&
		    pool->allocator->vtable->resize_pool(pool, size)) {
			hg_mem_garbage_collection(pool);
			retval = pool->allocator->vtable->alloc(pool, size, flags);
		}
	}

	return retval;
}

gboolean
hg_mem_free(gpointer data)
{
	HgMemObject *obj;
	HgObject *hobj;

	g_return_val_if_fail (data != NULL, FALSE);

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL) {
		g_warning("[BUG] Invalid object %p is given to be freed.", data);
		return FALSE;
	} else {
		const HgObjectVTable const *vtable;

		hobj = data;
		if (HG_MEMOBJ_IS_HGOBJECT (obj) &&
		    (vtable = hg_object_get_vtable(hobj)) != NULL &&
		    vtable->free) {
			vtable->free(data);
			/* prevents to invoke 'free' twice
			 * when the pool destroy process is being run.
			 */
			HG_OBJECT_SET_VTABLE_ID (hobj, 0);
		}
		if (!obj->pool->destroyed)
			obj->pool->allocator->vtable->free(obj->pool, data);
	}

	return TRUE;
}

gpointer
hg_mem_resize(gpointer data,
	      gsize    size)
{
	HgMemObject *obj;

	g_return_val_if_fail (data != NULL, NULL);

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL) {
		g_warning("Invalid object %p was about to be resized.", data);
		return NULL;
	}

	return obj->pool->allocator->vtable->resize(obj, size);
}

gsize
hg_mem_get_object_size(gpointer data)
{
	HgMemObject *obj;

	g_return_val_if_fail (data != NULL, 0);

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL) {
		g_warning("Invalid object %p was about to get an object size.", data);
		return 0;
	}

	return obj->pool->allocator->vtable->get_size(obj);
}

/* GC */
void
hg_mem_gc_mark_array_region(HgMemPool *pool,
			    gpointer   start,
			    gpointer   end)
{
	gpointer p;
	HgMemObject *obj;

	g_return_if_fail (pool->allocator->vtable->is_safe_object != NULL);

	if (start > end) {
		p = start;
		start = end - 1;
		end = p + 1;
	}
	for (p = start; p < end; p++) {
		obj = hg_mem_get_object__inline_nocheck(*(gsize *)p);
		if (pool->allocator->vtable->is_safe_object(pool, obj)) {
			if (!hg_mem_is_gc_mark__inline(obj)) {
#ifdef DEBUG_GC
				g_print("MARK: %p (mem: %p age: %d) from array region.\n", obj->data, obj, HG_MEMOBJ_GET_MARK_AGE (obj));
#endif /* DEBUG_GC */
				hg_mem_gc_mark__inline(obj);
			} else {
#ifdef DEBUG_GC
				g_print("MARK[already]: %p (mem: %p) from array region.\n", obj->data, obj);
#endif /* DEBUG_GC */
			}
		}
		obj = p;
		if (pool->allocator->vtable->is_safe_object(pool, obj)) {
			if (!hg_mem_is_gc_mark__inline(obj)) {
#ifdef DEBUG_GC
				g_print("MARK: %p (mem: %p) from array region.\n", obj->data, obj);
#endif /* DEBUG_GC */
				hg_mem_gc_mark__inline(obj);
			} else {
#ifdef DEBUG_GC
				g_print("MARK[already]: %p (mem: %p) from array region.\n", obj->data, obj);
#endif /* DEBUG_GC */
			}
		}
	}
}

void
hg_mem_add_root_node(HgMemPool *pool,
		     gpointer   data)
{
	pool->root_node = g_list_append(pool->root_node, data);
}

void
hg_mem_remove_root_node(HgMemPool *pool,
			gpointer   data)
{
	HgMemObject *obj;

	g_return_if_fail (pool != NULL);

	hg_mem_get_object__inline(data, obj);
	g_return_if_fail (obj != NULL);
	g_return_if_fail (_hg_mem_pool_is_own_memobject(pool, obj));

	pool->root_node = g_list_remove(pool->root_node, data);
}

void
hg_mem_add_pool_reference(HgMemPool *pool,
			  HgMemPool *other_pool)
{
	g_return_if_fail (pool != NULL);
	g_return_if_fail (other_pool != NULL);
	g_return_if_fail (pool != other_pool); /* to avoid the loop */

	if (g_list_find(pool->other_pool_ref_list, other_pool) == NULL)
		pool->other_pool_ref_list = g_list_append(pool->other_pool_ref_list,
							  other_pool);
}

void
hg_mem_remove_pool_reference(HgMemPool *pool,
			     HgMemPool *other_pool)
{
	g_return_if_fail (pool != NULL);
	g_return_if_fail (other_pool != NULL);

	pool->other_pool_ref_list = g_list_remove(pool->other_pool_ref_list, other_pool);
}

/* HgObject */
guint
hg_object_get_state(HgObject *object)
{
	g_return_val_if_fail (object != NULL, 0);

	return HG_OBJECT_GET_STATE (object);
}

void
hg_object_set_state(HgObject *object,
		    guint     state)
{
	g_return_if_fail (object != NULL);

	HG_OBJECT_SET_STATE (object, state);
}

void
hg_object_add_state(HgObject *object,
		    guint     state)
{
	state |= hg_object_get_state(object);
	hg_object_set_state(object, state);
}

gboolean
hg_object_is_state(HgObject *object,
		   guint     state)
{
	g_return_val_if_fail (object != NULL, FALSE);

	return (HG_OBJECT_GET_STATE (object) & state) == state;
}

gpointer
hg_object_dup(HgObject *object)
{
	const HgObjectVTable const *vtable;
	HgMemObject *obj;

	g_return_val_if_fail (object != NULL, NULL);

	hg_mem_get_object__inline(object, obj);
	if (obj != NULL && HG_MEMOBJ_IS_HGOBJECT (obj) &&
	    (vtable = hg_object_get_vtable(object)) != NULL &&
	    vtable->dup)
		return vtable->dup(object);

	return object;
}

gpointer
hg_object_copy(HgObject *object)
{
	const HgObjectVTable const *vtable;
	HgMemObject *obj;

	g_return_val_if_fail (object != NULL, NULL);

	hg_mem_get_object__inline(object, obj);
	if (obj != NULL && HG_MEMOBJ_IS_HGOBJECT (obj) &&
	    (vtable = hg_object_get_vtable(object)) != NULL &&
	    vtable->copy)
		return vtable->copy(object);

	return object;
}

const HgObjectVTable const *
hg_object_get_vtable(HgObject *object)
{
	guint id;
	HgMemObject *obj;

	g_return_val_if_fail (object != NULL, NULL);
	hg_mem_get_object__inline(object, obj);
	g_return_val_if_fail (HG_MEMOBJ_IS_HGOBJECT (obj), NULL);

	id = HG_OBJECT_GET_VTABLE_ID (object);

	if (id == 0) {
		/* 0 is still valid and intentional that means no vtable. */
		return NULL;
	}
	if (id > _hg_object_vtable_array->len) {
		g_warning("[BUG] Invalid vtable ID found: %p id: %d latest id: %u\n",
			  object, id, _hg_object_vtable_array->len);

		return NULL;
	}

	return g_ptr_array_index(_hg_object_vtable_array, id - 1);
}

void
hg_object_set_vtable(HgObject                   *object,
		     const HgObjectVTable const *vtable)
{
	guint id = 0;

	g_return_if_fail (object != NULL);
	g_return_if_fail (vtable != NULL);
	g_return_if_fail (hg_mem_is_initialized);
	g_return_if_fail (_hg_object_vtable_array->len < 255);

	G_LOCK (hgobject);

	if ((id = GPOINTER_TO_UINT(hg_btree_find(_hg_object_vtable_tree, (gpointer)vtable))) == 0) {
		g_ptr_array_add(_hg_object_vtable_array, (gpointer)vtable);
		id = _hg_object_vtable_array->len;
		hg_btree_add(_hg_object_vtable_tree, (gpointer)vtable, GUINT_TO_POINTER (id));
	}
	if (id > 255) {
		g_warning("[BUG] Invalid vtable ID found in tree: %p id %u\n", object, id);
		id = 0;
	}
	HG_OBJECT_SET_VTABLE_ID (object, id);

	G_UNLOCK (hgobject);
}
