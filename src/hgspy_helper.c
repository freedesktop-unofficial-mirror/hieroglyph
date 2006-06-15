/* 
 * hgspy_helper.c
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
#include "config.h"
#endif
#include <stdlib.h>
#include <gmodule.h>
#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgmem.h>
#include "../hieroglyph/hgallocator-private.h"
#include "hgspy_helper.h"
#include "visualizer.h"


static gpointer __hg_mem_pool_destroy = NULL;
static gpointer __hg_mem_pool_add_heap = NULL;
static gpointer __hg_mem_alloc_with_flags = NULL;
static gpointer __hg_mem_free = NULL;
static gpointer __hg_mem_resize = NULL;
static GModule *__handle = NULL;
static GtkWidget *visual = NULL;

/*
 * Private Functions
 */
static void  __attribute__((constructor))
helper_init(void)
{
	unsetenv("LD_PRELOAD");
	__handle = g_module_open("libhieroglyph.so", 0);
	if (__handle == NULL) {
		g_warning("Failed g_module_open: %s", g_module_error());
		exit(1);
	}
	if (!g_module_symbol(__handle, "hg_mem_pool_destroy", &__hg_mem_pool_destroy)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		exit(1);
	}
	if (!g_module_symbol(__handle, "hg_mem_pool_add_heap", &__hg_mem_pool_add_heap)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		exit(1);
	}
	if (!g_module_symbol(__handle, "hg_mem_alloc_with_flags", &__hg_mem_alloc_with_flags)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		exit(1);
	}
	if (!g_module_symbol(__handle, "hg_mem_free", &__hg_mem_free)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		exit(1);
	}
	if (!g_module_symbol(__handle, "hg_mem_resize", &__hg_mem_resize)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		exit(1);
	}
	g_type_init();
	visual = hg_memory_visualizer_new();
}

void  __attribute__((destructor))
helper_finalize(void)
{
	if (__handle)
		g_module_close(__handle);
}

/*
 * preload functions
 */
void
hg_mem_pool_destroy(HgMemPool *pool)
{
	gchar *pool_name = g_strdup(hg_mem_pool_get_name(pool));

	((void (*) (HgMemPool *))__hg_mem_pool_destroy) (pool);

	hg_memory_visualizer_remove_pool(HG_MEMORY_VISUALIZER (visual),
					 pool_name);
	g_free(pool_name);
}

void
hg_mem_pool_add_heap(HgMemPool *pool,
		     HgHeap    *heap)
{
	((void (*) (HgMemPool *, HgHeap *))__hg_mem_pool_add_heap) (pool, heap);
	hg_memory_visualizer_set_heap_state(HG_MEMORY_VISUALIZER (visual),
					    hg_mem_pool_get_name(pool),
					    heap);
}

gpointer
hg_mem_alloc_with_flags(HgMemPool *pool,
			gsize      size,
			guint      flags)
{
	gpointer retval = ((gpointer (*) (HgMemPool *, gsize, guint))
			   __hg_mem_alloc_with_flags) (pool, size, flags);
	HgMemObject *obj;

	if (retval) {
		hg_mem_get_object__inline(retval, obj);
		if (obj) {
			gint heap_id = HG_MEMOBJ_GET_HEAP_ID (obj);
			HgHeap *h = g_ptr_array_index(obj->pool->heap_list, heap_id);

			hg_memory_visualizer_set_chunk_state(HG_MEMORY_VISUALIZER (visual),
							     hg_mem_pool_get_name(obj->pool),
							     heap_id,
							     (gsize)obj - (gsize)h->heaps,
							     hg_mem_get_object_size(retval),
							     HG_CHUNK_USED);
		}
	}

	return retval;
}

gboolean
hg_mem_free(gpointer data)
{
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj) {
		gint heap_id = HG_MEMOBJ_GET_HEAP_ID (obj);
		HgHeap *h = g_ptr_array_index(obj->pool->heap_list, heap_id);

		hg_memory_visualizer_set_chunk_state(HG_MEMORY_VISUALIZER (visual),
						     hg_mem_pool_get_name(obj->pool),
						     heap_id,
						     (gsize)obj - (gsize)h->heaps,
						     hg_mem_get_object_size(data),
						     HG_CHUNK_FREE);
	}

	return ((gboolean (*) (gpointer))__hg_mem_free) (data);
}

gpointer
hg_mem_resize(gpointer data,
	      gsize    size)
{
	gpointer retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj) {
		gint heap_id = HG_MEMOBJ_GET_HEAP_ID (obj);
		HgHeap *h = g_ptr_array_index(obj->pool->heap_list, heap_id);

		hg_memory_visualizer_set_chunk_state(HG_MEMORY_VISUALIZER (visual),
						     hg_mem_pool_get_name(obj->pool),
						     heap_id,
						     (gsize)obj - (gsize)h->heaps,
						     hg_mem_get_object_size(data),
						     HG_CHUNK_FREE);
	}

	retval = ((gpointer (*) (gpointer, gsize))__hg_mem_resize) (data, size);

	if (retval) {
		hg_mem_get_object__inline(retval, obj);
		if (obj) {
			gint heap_id = HG_MEMOBJ_GET_HEAP_ID (obj);
			HgHeap *h = g_ptr_array_index(obj->pool->heap_list, heap_id);

			hg_memory_visualizer_set_chunk_state(HG_MEMORY_VISUALIZER (visual),
							     hg_mem_pool_get_name(obj->pool),
							     heap_id,
							     (gsize)obj - (gsize)h->heaps,
							     hg_mem_get_object_size(retval),
							     HG_CHUNK_USED);
		}
	}

	return retval;
}

/*
 * Public Functions
 */
GtkWidget *
hg_spy_helper_get_widget(void)
{
	return visual;
}
