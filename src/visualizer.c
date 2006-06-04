/* 
 * visualizer.c
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
#include <math.h>
#include <string.h>
#include <glib/gi18n.h>
#include <hieroglyph/hgtypes.h>
#include "../hieroglyph/hgallocator-private.h"
#include "visualizer.h"


enum {
	PROP_0,
	PROP_MAX_SIZE,
};

enum {
	LAST_SIGNAL
};

struct _HgMemoryVisualizerClass {
	GtkLayoutClass parent_class;
};

struct _HgMemoryVisualizer {
	GtkLayout parent_instance;

	/*< private >*/
	gdouble     block_size;
	GHashTable *pool2size;
	GHashTable *pool2array;
};

struct Heap2Offset {
	gsize *heap2offset;
	gchar *bitmaps;
	guint  alloc;
	gsize  total_size;
};


static struct Heap2Offset *_heap2offset_new (guint    prealloc);
static void                _heap2offset_free(gpointer data);


static GtkLayoutClass *parent_class = NULL;
//static guint           signals[LAST_SIGNAL] = { 0 };


/*
 * Signal handlers
 */
/* GObject */
static void
hg_memory_visualizer_real_set_property(GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
//	HgMemoryVisualizer *visual = HG_MEMORY_VISUALIZER (object);

	switch (prop_id) {
	    case PROP_MAX_SIZE:
//		    hg_memory_visualizer_set_max_size(visual, g_value_get_long(value));
		    break;
	    default:
		    break;
	}
}

static void
hg_memory_visualizer_real_get_property(GObject    *object,
				       guint       prop_id,
				       GValue     *value,
				       GParamSpec *pspec)
{
//	HgMemoryVisualizer *visual = HG_MEMORY_VISUALIZER (object);

	switch (prop_id) {
	    case PROP_MAX_SIZE:
//		    g_value_set_long(value, hg_memory_visualizer_get_max_size(visual));
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

/* GtkObject */
static void
hg_memory_visualizer_real_destroy(GtkObject *object)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (object));

	visual = HG_MEMORY_VISUALIZER (object);
	if (visual->pool2size)
		g_hash_table_destroy(visual->pool2size);
	if (visual->pool2array)
		g_hash_table_destroy(visual->pool2array);

	/* FIXME: not yet implemented */

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* GtkWidget */
static gboolean
hg_memory_visualizer_real_expose(GtkWidget      *widget,
				 GdkEventExpose *event)
{
	HgMemoryVisualizer *visual;

	g_return_val_if_fail (HG_IS_MEMORY_VISUALIZER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	visual = HG_MEMORY_VISUALIZER (widget);

	if (!GTK_WIDGET_DRAWABLE (widget) || (event->window != visual->parent_instance.bin_window))
		return FALSE;

	/* FIXME: not yet implemented */

	return FALSE;
}

static void
hg_memory_visualizer_real_realize(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	visual = HG_MEMORY_VISUALIZER (widget);

	gdk_window_set_events(visual->parent_instance.bin_window,
			      (gdk_window_get_events(visual->parent_instance.bin_window)
			       | GDK_EXPOSURE_MASK));

	/* FIXME: not yet implemented */

}

static void
hg_memory_visualizer_real_unrealize(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	visual = HG_MEMORY_VISUALIZER (widget);

	/* FIXME: not yet implemented */

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
hg_memory_visualizer_real_map(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	if (GTK_WIDGET_CLASS (parent_class)->map)
		(* GTK_WIDGET_CLASS (parent_class)->map) (widget);

	visual = HG_MEMORY_VISUALIZER (widget);

	/* FIXME: not yet implemented */

}

static void
hg_memory_visualizer_real_unmap(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	visual = HG_MEMORY_VISUALIZER (widget);

	/* FIXME: not yet implemented */

	if (GTK_WIDGET_CLASS (parent_class)->unmap)
		(* GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

static void
hg_memory_visualizer_real_size_request(GtkWidget      *widget,
				       GtkRequisition *requisition)
{
}

static void
hg_memory_visualizer_real_size_allocate(GtkWidget     *widget,
					GtkAllocation *allocation)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));
	g_return_if_fail (allocation != NULL);

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		(* GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	visual = HG_MEMORY_VISUALIZER (widget);

	/* FIXME: not yet implemented */

}

/*
 * Private Functions
 */
static void
hg_memory_visualizer_class_init(HgMemoryVisualizerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);

	/* initialize GObject */
	gobject_class->set_property = hg_memory_visualizer_real_set_property;
	gobject_class->get_property = hg_memory_visualizer_real_get_property;

	/* initialize GtkObject */
	object_class->destroy = hg_memory_visualizer_real_destroy;

	/* initialize GtkWidget */
	widget_class->expose_event = hg_memory_visualizer_real_expose;
	widget_class->realize = hg_memory_visualizer_real_realize;
	widget_class->unrealize = hg_memory_visualizer_real_unrealize;
	widget_class->map = hg_memory_visualizer_real_map;
	widget_class->unmap = hg_memory_visualizer_real_unmap;
	widget_class->size_request = hg_memory_visualizer_real_size_request;
	widget_class->size_allocate = hg_memory_visualizer_real_size_allocate;

	/* initialize HgMemoryVisualizer */

	/* GObject properties */
	g_object_class_install_property(gobject_class,
					PROP_MAX_SIZE,
					g_param_spec_long("max_size",
							  _("Max size of memory pool"),
							  _("Max size of memory pool that is now being visualized."),
							  0,
							  G_MAXLONG,
							  0,
							  G_PARAM_READWRITE));

	/* signals */
}

static void
hg_memory_visualizer_instance_init(HgMemoryVisualizer *visual)
{
	visual->block_size = 0.0L;
	visual->pool2size = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	visual->pool2array = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _heap2offset_free);
}

static struct Heap2Offset *
_heap2offset_new(guint prealloc)
{
	struct Heap2Offset *retval;

	g_return_val_if_fail (prealloc > 0, NULL);

	retval = g_new(struct Heap2Offset, 1);
	if (retval != NULL) {
		retval->heap2offset = g_new(gsize, prealloc);
		retval->bitmaps = NULL;
		retval->alloc = prealloc;
		retval->total_size = 0;
	}

	return retval;
}

static void
_heap2offset_free(gpointer data)
{
	struct Heap2Offset *p = data;

	if (p->heap2offset)
		g_free(p->heap2offset);
	if (p->bitmaps)
		g_free(p->bitmaps);
	g_free(data);
}

/*
 * Public Functions
 */
GType
hg_memory_visualizer_get_type(void)
{
	static GType mv_type = 0;

	if (!mv_type) {
		static const GTypeInfo mv_info = {
			.class_size     = sizeof (HgMemoryVisualizerClass),
			.base_init      = NULL,
			.base_finalize  = NULL,
			.class_init     = (GClassInitFunc)hg_memory_visualizer_class_init,
			.class_finalize = NULL,
			.class_data     = NULL,
			.instance_size  = sizeof (HgMemoryVisualizer),
			.n_preallocs    = 0,
			.instance_init  = (GInstanceInitFunc)hg_memory_visualizer_instance_init,
			.value_table    = NULL,
		};

		mv_type = g_type_register_static(GTK_TYPE_LAYOUT, "HgMemoryVisualizer",
						 &mv_info, 0);
	}

	return mv_type;
}

GtkWidget *
hg_memory_visualizer_new(void)
{
	return GTK_WIDGET (g_object_new(HG_TYPE_MEMORY_VISUALIZER, NULL));
}

void
hg_memory_visualizer_set_max_size(HgMemoryVisualizer *visual,
				  const gchar        *name,
				  gsize               size)
{
	gsize area;
	gdouble scale;
	struct Heap2Offset *h2o;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);

	area = visual->parent_instance.width * visual->parent_instance.height;
	g_hash_table_insert(visual->pool2size, g_strdup(name), GSIZE_TO_POINTER (size));
	if ((h2o = g_hash_table_lookup(visual->pool2array, name)) == NULL) {
		h2o = _heap2offset_new(256);
		g_hash_table_insert(visual->pool2array, g_strdup(name), h2o);
	}
	if (h2o->bitmaps) {
		if (size > h2o->total_size) {
			gpointer p = g_realloc(h2o->bitmaps, sizeof (gchar) * ((size + 8) / 8));
			if (p == NULL) {
				g_warning("Failed to allocate a memory for bitmaps.");
				return;
			}
			h2o->bitmaps = p;
			memset((void *)((gsize)h2o->bitmaps + h2o->total_size), 0, size - h2o->total_size);
		}
	} else {
		h2o->bitmaps = g_new0(gchar, (size + 8) / 8);
	}
	h2o->total_size = size;
	if (area > size) {
		scale = area / size;
		visual->block_size = sqrt(scale);
	} else {
		/* FIXME: need to scale up to fit in */
	}
}

gsize
hg_memory_visualizer_get_max_size(HgMemoryVisualizer *visual,
				  const gchar        *name)
{
	g_return_val_if_fail (HG_IS_MEMORY_VISUALIZER (visual), 0);
	g_return_val_if_fail (name != NULL, 0);

	return GPOINTER_TO_SIZE (g_hash_table_lookup(visual->pool2size, name));
}

void
hg_memory_visualizer_set_heap_state(HgMemoryVisualizer *visual,
				    const gchar        *name,
				    HgHeap             *heap)
{
	gsize current_size;
	struct Heap2Offset *h2o;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);
	g_return_if_fail (heap != NULL);

	if ((h2o = g_hash_table_lookup(visual->pool2array, name)) == NULL) {
		h2o = _heap2offset_new(256);
		g_hash_table_insert(visual->pool2array, g_strdup(name), h2o);
	}
	current_size = hg_memory_visualizer_get_max_size(visual, name);
	if (heap->serial >= h2o->alloc) {
		gpointer p = g_realloc(h2o->heap2offset,
				       sizeof (gsize) * (heap->serial + 256));

		if (p == NULL) {
			g_warning("Failed to allocate a memory for heap.");
			return;
		}
		h2o->heap2offset = p;
		h2o->alloc = heap->serial + 256;
	}
	h2o->heap2offset[heap->serial] = current_size;
	hg_memory_visualizer_set_max_size(visual, name, current_size + heap->total_heap_size);
}

void
hg_memory_visualizer_set_chunk_state(HgMemoryVisualizer *visual,
				     const gchar        *name,
				     gint                heap_id,
				     gsize               offset,
				     gsize               size,
				     HgMemoryChunkState  state)
{
	struct Heap2Offset *h2o;
	gsize base, location, start, end, i;
	gint bit;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);

	h2o = g_hash_table_lookup(visual->pool2array, name);
	if (h2o == NULL) {
		g_warning("Unknown pool found (pool: %s)\n", name);
		return;
	}
	base = h2o->heap2offset[heap_id];
	start = base + offset;
	end = start + size;
	for (i = start; i < end; i++) {
		location = i / 8;
		bit = 7 - (i % 8);
		switch (state) {
		    case HG_CHUNK_FREE:
			    h2o->bitmaps[location] &= ~(1 << bit);
			    break;
		    case HG_CHUNK_USED:
			    h2o->bitmaps[location] |= (1 << bit);
			    break;
		    default:
			    break;
		}
	}
	/* FIXME: update window */
}
