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
#include <glib/gi18n.h>
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
	glong   max_size;
	gdouble block_size;
};


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
	HgMemoryVisualizer *visual = HG_MEMORY_VISUALIZER (object);

	switch (prop_id) {
	    case PROP_MAX_SIZE:
		    hg_memory_visualizer_set_max_size(visual, g_value_get_long(value));
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
	HgMemoryVisualizer *visual = HG_MEMORY_VISUALIZER (object);

	switch (prop_id) {
	    case PROP_MAX_SIZE:
		    g_value_set_long(value, hg_memory_visualizer_get_max_size(visual));
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
	visual->max_size = 0;
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
				  glong               size)
{
	glong area;
	gdouble scale;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));

	area = visual->parent_instance.width * visual->parent_instance.height;
	visual->max_size = size;
	if (area > size) {
		scale = area / size;
		visual->block_size = sqrt(scale);
	} else {
		/* FIXME: need to scale up to fit in */
	}
}

glong
hg_memory_visualizer_get_max_size(HgMemoryVisualizer *visual)
{
	g_return_val_if_fail (HG_IS_MEMORY_VISUALIZER (visual), 0);

	return visual->max_size;
}

void
hg_memory_visualizer_set_chunk_state(HgMemoryVisualizer *visual,
				     gint                heap_id,
				     gpointer            addr,
				     gsize               size,
				     HgMemoryChunkState  state)
{
}
