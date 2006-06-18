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
	DRAW_UPDATED,
	GC_STARTED,
	GC_FINISHED,
	LAST_SIGNAL
};

struct _HgMemoryVisualizerClass {
	GtkMiscClass parent_class;

	void (* pool_updated) (HgMemoryVisualizer *visual,
			       gpointer            list);
	void (* draw_updated) (HgMemoryVisualizer *visual);
	void (* gc_started)   (HgMemoryVisualizer *visual);
	void (* gc_finished)  (HgMemoryVisualizer *visual);
};

struct Heap2Offset {
	gsize *heap2offset;
	gchar *bitmaps;
	guint  alloc;
	gsize  total_size;
};

struct _HgMemoryVisualizer {
	GtkMisc parent_instance;

	/*< private >*/
	gdouble             block_size;
	GHashTable         *pool2total_size;
	GHashTable         *pool2used_size;
	GHashTable         *pool2array;
	GSList             *pool_name_list;
	gchar              *current_pool_name;
	struct Heap2Offset *current_h2o;
	gboolean            need_update;
	guint               idle_id;
	GdkGC              *bg_gc;
	GdkGC              *used_gc;
	GdkGC              *free_gc;
	GdkGC              *used_and_free_gc;
	GdkPixmap          *pixmap;
};


static struct Heap2Offset *_heap2offset_new                      (guint               prealloc);
static void                _heap2offset_free                     (gpointer            data);
static void                _hg_memory_visualizer_add_idle        (HgMemoryVisualizer *visual);
static void                _hg_memory_visualizer_remove_idle     (HgMemoryVisualizer *visual);
static void                _hg_memory_visualizer_create_gc       (HgMemoryVisualizer *visual);
static void                _hg_memory_visualizer_redraw_in_pixmap(HgMemoryVisualizer *visual);


static GtkMiscClass *parent_class = NULL;
static guint         signals[LAST_SIGNAL] = { 0 };

G_LOCK_DEFINE_STATIC (visualizer);


/*
 * Signal handlers
 */
/* GObject */

/* GtkObject */
static void
hg_memory_visualizer_real_destroy(GtkObject *object)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (object));

	visual = HG_MEMORY_VISUALIZER (object);
	if (visual->pool2total_size) {
		g_hash_table_destroy(visual->pool2total_size);
		visual->pool2total_size = NULL;
	}
	if (visual->pool2used_size) {
		g_hash_table_destroy(visual->pool2used_size);
		visual->pool2used_size = NULL;
	}
	if (visual->pool2array) {
		g_hash_table_destroy(visual->pool2array);
		visual->pool2array = NULL;
	}
	if (visual->pool_name_list) {
		GSList *l;

		for (l = visual->pool_name_list; l != NULL; l = g_slist_next(l))
			g_free(l->data);
		g_slist_free(visual->pool_name_list);
		visual->pool_name_list = NULL;
	}
	visual->current_pool_name = NULL;
	visual->current_h2o = NULL;
	_hg_memory_visualizer_remove_idle(visual);

	if (visual->bg_gc) {
		g_object_unref(visual->bg_gc);
		visual->bg_gc = NULL;
	}
	if (visual->used_gc) {
		g_object_unref(visual->used_gc);
		visual->used_gc = NULL;
	}
	if (visual->free_gc) {
		g_object_unref(visual->free_gc);
		visual->free_gc = NULL;
	}
	if (visual->used_and_free_gc) {
		g_object_unref(visual->used_and_free_gc);
		visual->used_and_free_gc = NULL;
	}

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* GtkWidget */
static void
hg_memory_visualizer_real_size_request(GtkWidget      *widget,
				       GtkRequisition *requisition)
{
	GtkWidget *parent = gtk_widget_get_parent(widget);

	requisition->width = widget->requisition.width = parent->requisition.width;
	requisition->height = widget->requisition.height = parent->requisition.height;
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

	G_LOCK (visualizer);

	_hg_memory_visualizer_redraw_in_pixmap(visual);

	G_UNLOCK (visualizer);
}

static void
hg_memory_visualizer_real_realize(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	visual = HG_MEMORY_VISUALIZER (widget);

	widget->style = gtk_style_attach(widget->style, widget->window);
	gdk_window_set_background(widget->window, &widget->style->base[GTK_WIDGET_STATE (widget)]);

	if (visual->bg_gc == NULL ||
	    visual->used_gc == NULL ||
	    visual->free_gc == NULL ||
	    visual->used_and_free_gc == NULL)
		_hg_memory_visualizer_create_gc(visual);

	G_LOCK (visualizer);

	_hg_memory_visualizer_redraw_in_pixmap(visual);

	G_UNLOCK (visualizer);
}

static void
hg_memory_visualizer_real_unrealize(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	visual = HG_MEMORY_VISUALIZER (widget);

	_hg_memory_visualizer_remove_idle(visual);

	if (visual->bg_gc) {
		g_object_unref(visual->bg_gc);
		visual->bg_gc = NULL;
	}
	if (visual->used_gc) {
		g_object_unref(visual->used_gc);
		visual->used_gc = NULL;
	}
	if (visual->free_gc) {
		g_object_unref(visual->free_gc);
		visual->free_gc = NULL;
	}
	if (visual->used_and_free_gc) {
		g_object_unref(visual->used_and_free_gc);
		visual->used_and_free_gc = NULL;
	}

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
}

static void
hg_memory_visualizer_real_unmap(GtkWidget *widget)
{
	HgMemoryVisualizer *visual;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (widget));

	visual = HG_MEMORY_VISUALIZER (widget);

	_hg_memory_visualizer_remove_idle(visual);

	if (GTK_WIDGET_CLASS (parent_class)->unmap)
		(* GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

static gboolean
hg_memory_visualizer_real_expose(GtkWidget      *widget,
				 GdkEventExpose *event)
{
	HgMemoryVisualizer *visual;
	GtkMisc *misc;

	g_return_val_if_fail (HG_IS_MEMORY_VISUALIZER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	misc = GTK_MISC (widget);
	visual = HG_MEMORY_VISUALIZER (widget);

	if (!GTK_WIDGET_MAPPED (widget))
		return FALSE;

	gdk_draw_drawable(widget->window,
			  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			  visual->pixmap,
			  event->area.x, event->area.y,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);

	return FALSE;
}

/*
 * Private Functions
 */
static void
hg_memory_visualizer_class_init(HgMemoryVisualizerClass *klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);

	/* initialize GObject */

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

	/* signals */
	signals[POOL_UPDATED] = g_signal_new("pool-updated",
					     G_OBJECT_CLASS_TYPE (klass),
					     G_SIGNAL_RUN_FIRST,
					     G_STRUCT_OFFSET (HgMemoryVisualizerClass, pool_updated),
					     NULL, NULL,
					     gtk_marshal_VOID__POINTER,
					     G_TYPE_NONE, 1,
					     G_TYPE_POINTER);
	signals[DRAW_UPDATED] = g_signal_new("draw-updated",
					     G_OBJECT_CLASS_TYPE (klass),
					     G_SIGNAL_RUN_FIRST,
					     G_STRUCT_OFFSET (HgMemoryVisualizerClass, draw_updated),
					     NULL, NULL,
					     gtk_marshal_VOID__VOID,
					     G_TYPE_NONE, 0);
	signals[GC_STARTED] = g_signal_new("gc-started",
					   G_OBJECT_CLASS_TYPE (klass),
					   G_SIGNAL_RUN_FIRST,
					   G_STRUCT_OFFSET (HgMemoryVisualizerClass, gc_started),
					   NULL, NULL,
					   gtk_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);
	signals[GC_FINISHED] = g_signal_new("gc-finished",
					    G_OBJECT_CLASS_TYPE (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (HgMemoryVisualizerClass, gc_finished),
					    NULL, NULL,
					    gtk_marshal_VOID__VOID,
					    G_TYPE_NONE, 0);
}

static void
hg_memory_visualizer_instance_init(HgMemoryVisualizer *visual)
{
	visual->pool2total_size = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	visual->pool2used_size = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	visual->pool2array = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _heap2offset_free);
	visual->pool_name_list = NULL;
	visual->current_pool_name = NULL;
	visual->current_h2o = NULL;
	visual->need_update = FALSE;
	visual->idle_id = 0;
	visual->bg_gc = NULL;
	visual->used_gc = NULL;
	visual->free_gc = NULL;
	visual->used_and_free_gc = NULL;
	visual->pixmap = NULL;
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

	visual = HG_MEMORY_VISUALIZER (data);

	G_LOCK (visualizer);

	_hg_memory_visualizer_redraw_in_pixmap(visual);

	gtk_widget_queue_draw(GTK_WIDGET (visual));

	visual->idle_id = 0;
	visual->need_update = FALSE;

	G_UNLOCK (visualizer);

	g_signal_emit(visual, signals[DRAW_UPDATED], 0);

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

static void
_hg_memory_visualizer_create_gc(HgMemoryVisualizer *visual)
{
	GdkGCValues values;

	gdk_color_black(gdk_colormap_get_system(), &values.background);
	visual->bg_gc = gdk_gc_new_with_values(GTK_WIDGET (visual)->window, &values,
					       GDK_GC_BACKGROUND);
	values.foreground.pixel = 0;
	values.foreground.red = 0;
	values.foreground.green = 0;
	values.foreground.blue = 32768;
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &values.foreground, FALSE, FALSE);
	visual->free_gc = gdk_gc_new_with_values(GTK_WIDGET (visual)->window, &values,
						 GDK_GC_FOREGROUND);
	values.foreground.pixel = 0;
	values.foreground.red = 32768;
	values.foreground.green = 0;
	values.foreground.blue = 0;
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &values.foreground, FALSE, FALSE);
	visual->used_gc = gdk_gc_new_with_values(GTK_WIDGET (visual)->window, &values,
						 GDK_GC_FOREGROUND);
	values.foreground.pixel = 0;
	values.foreground.red = 32768;
	values.foreground.green = 0;
	values.foreground.blue = 32768;
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &values.foreground, FALSE, FALSE);
	visual->used_and_free_gc = gdk_gc_new_with_values(GTK_WIDGET (visual)->window, &values,
						 GDK_GC_FOREGROUND);
}

static void
_hg_memory_visualizer_redraw_in_pixmap(HgMemoryVisualizer *visual)
{
	GtkWidget *widget = GTK_WIDGET (visual);
	GtkMisc *misc = GTK_MISC (visual);
	gint i, j, base_x, base_y, x, y, width, height, base_scale, block_size, area;

	if (GTK_WIDGET_REALIZED (widget)) {
		if (visual->pixmap)
			g_object_unref(G_OBJECT (visual->pixmap));
		visual->pixmap = gdk_pixmap_new(widget->window,
						widget->allocation.width,
						widget->allocation.height,
						-1);
		base_x = x = misc->xpad;
		base_y = y = misc->ypad;
		width = widget->allocation.width - misc->xpad;
		height = widget->allocation.height - misc->ypad;
		area = (gdouble)(width - base_x) * (gdouble)(height - base_y);
		/* draw a background */
		gdk_draw_rectangle(visual->pixmap,
				   visual->bg_gc,
				   TRUE,
				   x, y, width, height);
		/* draw a memory images */
		if (visual->current_h2o) {
			struct Heap2Offset *h2o = visual->current_h2o;
			gint _used = 0, _free = 0, _scale;
			GdkGC *gc;

			if (area >= h2o->total_size) {
				block_size = (gint)sqrt(area / h2o->total_size);
				base_scale = 1;
			} else {
				base_scale = (gint)(1 / sqrt((gdouble)area / (gdouble)h2o->total_size));
				base_scale *= base_scale;
				block_size = 1;
			}
			_scale = base_scale;
			for (i = 0; i < h2o->total_size / 8; i++) {
				for (j = 7; j >= 0; j--) {
					if ((x + block_size) > width) {
						x = base_x;
						y += block_size;
					}
					if ((h2o->bitmaps[i] & (1 << j)) != 0) {
						_used++;
					} else {
						_free++;
					}
					_scale--;
					if (_scale == 0) {
						if (_used > 0 && _free == 0)
							gc = visual->used_gc;
						else if (_used == 0 && _free > 0)
							gc = visual->free_gc;
						else
							gc = visual->used_and_free_gc;
						gdk_draw_rectangle(visual->pixmap,
								   gc,
								   TRUE,
								   x, y,
								   block_size, block_size);
						x += block_size;
						_scale = base_scale;
						_used = 0;
						_free = 0;
					}
				}
			}
		}
		visual->need_update = TRUE;
		_hg_memory_visualizer_add_idle(visual);
	}
}

static gint
_hg_memory_visualizer_compare_pool_name_list(gconstpointer a,
					     gconstpointer b)
{
	return strcmp(a, b);
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

		mv_type = g_type_register_static(GTK_TYPE_MISC, "HgMemoryVisualizer",
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
	struct Heap2Offset *h2o;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);

	g_hash_table_insert(visual->pool2total_size, g_strdup(name), GSIZE_TO_POINTER (size));
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
			memset((void *)((gsize)h2o->bitmaps + (h2o->total_size / 8)),
			       0,
			       (size - h2o->total_size) / 8);
		}
	} else {
		h2o->bitmaps = g_new0(gchar, (size + 8) / 8);
	}
	h2o->total_size = size;
}

gsize
hg_memory_visualizer_get_max_size(HgMemoryVisualizer *visual,
				  const gchar        *name)
{
	g_return_val_if_fail (HG_IS_MEMORY_VISUALIZER (visual), 0);
	g_return_val_if_fail (name != NULL, 0);

	return GPOINTER_TO_SIZE (g_hash_table_lookup(visual->pool2total_size, name));
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
	if (g_slist_find_custom(visual->pool_name_list,
				name,
				_hg_memory_visualizer_compare_pool_name_list) == NULL) {
		visual->pool_name_list = g_slist_append(visual->pool_name_list,
							g_strdup(name));
	}
	if (visual->current_pool_name == NULL) {
		visual->current_pool_name = g_strdup(name);
		visual->current_h2o = h2o;
	}

	gdk_threads_enter();
	g_signal_emit(visual, signals[POOL_UPDATED], 0, visual->pool_name_list);
	gdk_threads_leave();
}

void
hg_memory_visualizer_remove_pool(HgMemoryVisualizer *visual,
				 const gchar        *name)
{
	GSList *l;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);

	if ((l = g_slist_find_custom(visual->pool_name_list,
				     name,
				     _hg_memory_visualizer_compare_pool_name_list)) == NULL) {
		g_warning("pool %s isn't registered.", name);
		return;
	}

	G_LOCK (visualizer);

	g_hash_table_remove(visual->pool2total_size, name);
	g_hash_table_remove(visual->pool2used_size, name);
	g_hash_table_remove(visual->pool2array, name);
	g_free(l->data);
	visual->pool_name_list = g_slist_delete_link(visual->pool_name_list, l);

	G_UNLOCK (visualizer);

	if (strcmp(name, visual->current_pool_name) == 0) {
		const gchar *pool_name;

		if (visual->pool_name_list)
			pool_name = visual->pool_name_list->data;
		else
			pool_name = "";
		hg_memory_visualizer_change_pool(visual, pool_name);
	}
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
	gsize base, location, start, end, i, used_size;
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

	G_LOCK (visualizer);

	used_size = hg_memory_visualizer_get_used_size(visual, name);
	for (i = start; i < end; i++) {
		location = i / 8;
		bit = 7 - (i % 8);
		switch (state) {
		    case HG_CHUNK_FREE:
			    h2o->bitmaps[location] &= ~(1 << bit);
			    if (i == start)
				    used_size -= size;
			    break;
		    case HG_CHUNK_USED:
			    h2o->bitmaps[location] |= (1 << bit);
			    if (i == start)
				    used_size += size;
			    break;
		    default:
			    break;
		}
	}
	g_hash_table_insert(visual->pool2used_size, g_strdup(name), GSIZE_TO_POINTER (used_size));

	if (strcmp(name, visual->current_pool_name) == 0) {
		visual->need_update = TRUE;
		_hg_memory_visualizer_add_idle(visual);
	}
	G_UNLOCK (visualizer);
}

void
hg_memory_visualizer_change_pool(HgMemoryVisualizer *visual,
				 const gchar        *name)
{
	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (name != NULL);

	G_LOCK (visualizer);

	if (g_slist_find(visual->pool_name_list, name) != NULL) {
		if (visual->current_pool_name)
			g_free(visual->current_pool_name);
		visual->current_pool_name = g_strdup(name);
		visual->current_h2o = g_hash_table_lookup(visual->pool2array, name);
	} else {
		/* send a signal to update the out of date list */
		if (visual->current_pool_name)
			g_free(visual->current_pool_name);
		visual->current_pool_name = NULL;
		visual->current_h2o = NULL;

		g_signal_emit(visual, signals[POOL_UPDATED], 0, visual->pool_name_list);
	}
	visual->need_update = TRUE;
	_hg_memory_visualizer_add_idle(visual);

	G_UNLOCK (visualizer);
}

gsize
hg_memory_visualizer_get_used_size(HgMemoryVisualizer *visual,
				   const gchar        *name)
{
	g_return_val_if_fail (HG_IS_MEMORY_VISUALIZER (visual), 0);
	g_return_val_if_fail (name != NULL, 0);

	return GPOINTER_TO_SIZE (g_hash_table_lookup(visual->pool2used_size, name));
}

const gchar *
hg_memory_visualizer_get_current_pool_name(HgMemoryVisualizer *visual)
{
	return visual->current_pool_name;
}

void
hg_memory_visualizer_notify_gc_state(HgMemoryVisualizer *visual,
				     gboolean            is_finished)
{
	gdk_threads_enter();
	if (is_finished)
		g_signal_emit(visual, signals[GC_FINISHED], 0);
	else
		g_signal_emit(visual, signals[GC_STARTED], 0);
	gdk_threads_leave();

	G_LOCK (visualizer);

	visual->need_update = TRUE;
	_hg_memory_visualizer_add_idle(visual);

	G_UNLOCK (visualizer);
}
