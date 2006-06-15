/* 
 * visualizer.h
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
#ifndef __VISUALIZER_H__
#define __VISUALIZER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HG_TYPE_MEMORY_VISUALIZER	(hg_memory_visualizer_get_type())
#define HG_MEMORY_VISUALIZER(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), HG_TYPE_MEMORY_VISUALIZER, HgMemoryVisualizer))
#define HG_MEMORY_VISUALIZER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), HG_TYPE_MEMORY_VISUALIZER, HgMemoryVisualizerClass))
#define HG_IS_MEMORY_VISUALIZER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), HG_TYPE_MEMORY_VISUALIZER))
#define HG_IS_MEMORY_VISUALIZER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), HG_TYPE_MEMORY_VISUALIZER))
#define HG_MEMORY_VISUALIZER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HG_TYPE_MEMORY_VISUALIZER, HgMemoryVisualizerClass))


typedef struct _HgMemoryVisualizerClass	HgMemoryVisualizerClass;
typedef struct _HgMemoryVisualizer	HgMemoryVisualizer;
typedef enum _HgMemoryChunkState	HgMemoryChunkState;


enum _HgMemoryChunkState {
	HG_CHUNK_FREE,
	HG_CHUNK_USED,
};


GType        hg_memory_visualizer_get_type             (void) G_GNUC_CONST;
GtkWidget   *hg_memory_visualizer_new                  (void);
void         hg_memory_visualizer_set_max_size         (HgMemoryVisualizer *visual,
							const gchar        *name,
							gsize               size);
gsize        hg_memory_visualizer_get_max_size         (HgMemoryVisualizer *visual,
							const gchar        *name);
void         hg_memory_visualizer_set_heap_state       (HgMemoryVisualizer *visual,
							const gchar        *name,
							HgHeap             *heap);
void         hg_memory_visualizer_remove_pool          (HgMemoryVisualizer *visual,
							const gchar        *name);
void         hg_memory_visualizer_set_chunk_state      (HgMemoryVisualizer *visual,
							const gchar        *name,
							gint                heap_id,
							gsize               offset,
							gsize               size,
							HgMemoryChunkState  state);
void         hg_memory_visualizer_change_pool          (HgMemoryVisualizer *visual,
							const gchar        *name);
gsize        hg_memory_visualizer_get_used_size        (HgMemoryVisualizer *visual,
							const gchar        *name);
const gchar *hg_memory_visualizer_get_current_pool_name(HgMemoryVisualizer *visual);


G_END_DECLS

#endif /* __VISUALIZER_H__ */
