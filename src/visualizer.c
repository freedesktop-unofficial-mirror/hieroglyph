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
	POOL_UPDATED,
	LAST_SIGNAL
};

struct _HgMemoryVisualizerClass {
	GtkLayoutClass parent_class;

	void (* pool_updated) (HgMemoryVisualizer *visual,
			       gpointer            list);
};

struct Heap2Offset {
	gsize *heap2offset;
	gchar *bitmaps;
	guint  alloc;
	gsize  total_size;
};

struct _HgMemoryVisualizer {
	GtkLayout parent_instance;

	/*< private >*/
	gdouble             block_size;
	GHashTable         *pool2size;
	GHashTable         *pool2array;
	GSList             *pool_name_list;
	const gchar        *current_pool_name;
	struct Heap2Offset *current_h2o;
	gboolean            need_update;
	guint               idle_id;
};


static struct Heap2Offset *_heap2offset_new                 (guint               prealloc);
static void                _heap2offset_free                (gpointer            data);
static void                _hg_memory_visualizer_add_idle   (HgMemoryVisualizer *visual);
static void                _hg_memory_visualizer_remove_idle(HgMemoryVisualizer *visual);


static GtkLayoutClass *parent_class = NULL;
static guint           signals[LAST_SIGNAL] = { 0 };

G_LOCK_DEFINE_STATIC (visualizer);


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
	if (visual->pool2size) {
		g_hash_table_destroy(visual->pool2size);
		visual->pool2size = NULL;
	}
	if (visual->pool2array) {
		g_hash_table_destroy(visual->pool2array);
		visual->pool2array = NULL;
	}
	_hg_memory_visualizer_remove_idle(visual);

	/* FIXME: not yet implemented */

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* GtkWidget */
#if 0
static void
hg_memory_visualizer_real_size_request(GtkWidget      *widget,
				       GtkRequisition *requisition)
{
}
#endif

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

	visual->parent_instance.hadjustment->page_size = allocation->width;
	visual->parent_instance.hadjustment->page_increment = allocation->width / 2;
	visual->parent_instance.vadjustment->page_size = allocation->height;
	visual->parent_instance.vadjustment->page_increment = allocation->height / 2;

	/* FIXME: scroll */

	g_signal_emit_by_name(visual->parent_instance.hadjustment, "changed");
	g_signal_emit_by_name(visual->parent_instance.vadjustment, "changed");
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

	g_print("realize\n");
	/* FIXME: not yet implemented */

}

static void
hg_memory_visualizer_real_unrealize(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	visual = HG_MEMORY_VISUALIZER (widget);

	_hg_memory_visualizer_remove_idle(visual);
	g_print("unrealize\n");
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

	if (visual->need_update)
		_hg_memory_visualizer_add_idle(visual);

	g_print("map\n");
	/* FIXME: not yet implemented */

}

static void
hg_memory_visualizer_real_unmap(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	visual = HG_MEMORY_VISUALIZER (widget);

	_hg_memory_visualizer_remove_idle(visual);
	g_print("unmap\n");
	/* FIXME: not yet implemented */

	if (GTK_WIDGET_CLASS (parent_class)->unmap)
		(* GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

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

	g_print("expose\n");
	/* FIXME: not yet implemented */

	return FALSE;
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
//	widget_class->size_request = hg_memory_visualizer_real_size_request;
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
	signals[POOL_UPDATED] = g_signal_new("pool-updated",
					     G_OBJECT_CLASS_TYPE (klass),
					     G_SIGNAL_RUN_FIRST,
					     G_STRUCT_OFFSET (HgMemoryVisualizerClass, pool_updated),
					     NULL, NULL,
					     gtk_marshal_VOID__POINTER,
					     G_TYPE_NONE, 1,
					     G_TYPE_POINTER);
}

static void
hg_memory_visualizer_instance_init(HgMemoryVisualizer *visual)
{
	visual->block_size = 0.0L;
	visual->pool2size = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	visual->pool2array = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _heap2offset_free);
	visual->pool_name_list = NULL;
	visual->current_pool_name = NULL;
	visual->current_h2o = NULL;
	visual->need_update = FALSE;
	visual->idle_id = 0;
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

static gboolean
_hg_memory_visualizer_idle_handler_cb(gpointer data)
{
	HgMemoryVisualizer *visual;

	G_LOCK (visualizer);

	visual = HG_MEMORY_VISUALIZER (data);
	gtk_widget_queue_draw(GTK_WIDGET (visual));
	visual->idle_id = 0;
	visual->need_update = FALSE;

	G_UNLOCK (visualizer);

	return FALSE;
}

static void
_hg_memory_visualizer_add_idle(HgMemoryVisualizer *visual)
{
	g_return_if_fail (visual->need_update);

	if (!visual->idle_id) {
		visual->idle_id = gtk_idle_add(_hg_memory_visualizer_idle_handler_cb, visual);
	}
}

static void
_hg_memory_visualizer_remove_idle(HgMemoryVisualizer *visual)
{
	if (visual->idle_id != 0) {
		gtk_idle_remove(visual->idle_id);
		visual->idle_id = 0;
	}
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
	visual->pool_name_list = g_slist_append(visual->pool_name_list,
						g_strdup(name));
	if (visual->current_pool_name == NULL) {
		visual->current_pool_name = name;
		visual->current_h2o = h2o;
	}
	g_signal_emit(visual, signals[POOL_UPDATED], 0, visual->pool_name_list);
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
	G_LOCK (visualizer);
	visual->need_update = TRUE;
	_hg_memory_visualizer_add_idle(visual);
	G_UNLOCK (visualizer);
}

void
hg_memory_visualizer_change_pool(HgMemoryVisualizer *visual,
				 const gchar        *name)
{
	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);

	if (g_slist_find(visual->pool_name_list, name) != NULL) {
		visual->current_pool_name = name;
		visual->current_h2o = g_hash_table_lookup(visual->pool2array, name);
		G_LOCK (visualizer);
		visual->need_update = TRUE;
		_hg_memory_visualizer_add_idle(visual);
		G_UNLOCK (visualizer);
	} else {
		/* send a signal to update the out of date list */
		g_signal_emit(visual, signals[POOL_UPDATED], 0, visual->pool_name_list);
	}
}
