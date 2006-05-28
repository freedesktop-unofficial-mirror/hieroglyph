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
#include "hgspy_helper.h"
#include "visualizer.h"


static gpointer __hg_mem_alloc_with_flags = NULL;
static gpointer __hg_mem_free = NULL;
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
	if (!g_module_symbol(__handle, "hg_mem_alloc_with_flags", &__hg_mem_alloc_with_flags)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		exit(1);
	}
	if (!g_module_symbol(__handle, "hg_mem_free", &__hg_mem_free)) {
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
gpointer
hg_mem_alloc_with_flags(HgMemPool *pool,
			gsize      size,
			guint      flags)
{
	gpointer retval = ((gpointer (*) (HgMemPool *, gsize, guint))
			   __hg_mem_alloc_with_flags) (pool, size, flags);
	HgMemObject *obj;

	hg_mem_get_object__inline(retval, obj);
	if (obj) {
		hg_memory_visualizer_set_chunk_state(HG_MEMORY_VISUALIZER (visual),
						     obj->heap_id,
						     obj,
						     obj->block_size,
						     HG_CHUNK_USED);
	}

	return retval;
}

gboolean
hg_mem_free(gpointer data)
{
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj) {
		hg_memory_visualizer_set_chunk_state(HG_MEMORY_VISUALIZER (visual),
						     obj->heap_id,
						     obj,
						     obj->block_size,
						     HG_CHUNK_FREE);
	}

	return ((gboolean (*) (gpointer))__hg_mem_free) (data);
}

/*
 * Public Functions
 */
GtkWidget *
hg_spy_helper_get_widget(void)
{
	return visual;
}
