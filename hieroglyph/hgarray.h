/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGARRAY_H__
#define __HIEROGLYPH_HGARRAY_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

typedef struct _hg_bs_array_t	hg_bs_array_t;
typedef struct _hg_array_t	hg_array_t;
typedef gboolean (* hg_array_traverse_func_t) (hg_mem_t    *mem,
					       hg_quark_t   q,
					       gpointer     data,
					       GError     **error);

struct _hg_bs_array_t {
	hg_bs_template_t t;
	guint16          length;
	guint32          offset;
};
struct _hg_array_t {
	hg_object_t o;
	hg_quark_t  qcontainer;
	guint16     offset;
	guint16     length;
	gsize       allocated_size;
	gboolean    is_fixed_size;
};


hg_object_vtable_t *hg_object_array_get_vtable(void);
hg_quark_t          hg_array_new              (hg_mem_t                  *mem,
                                               gsize                      size,
                                               gpointer                  *ret);
gboolean            hg_array_set              (hg_array_t                *array,
                                               hg_quark_t                 quark,
                                               gsize                      index);
hg_quark_t          hg_array_get              (hg_array_t                *array,
                                               gsize                      index,
                                               GError                   **error);
gboolean            hg_array_insert           (hg_array_t                *array,
                                               hg_quark_t                 quark,
                                               gsize                      pos);
gboolean            hg_array_remove           (hg_array_t                *array,
                                               gsize                      pos);
gsize               hg_array_length           (hg_array_t                *array);
gsize               hg_array_maxlength        (hg_array_t                *array);
void                hg_array_foreach          (hg_array_t                *array,
                                               hg_array_traverse_func_t   func,
                                               gpointer                   data,
                                               GError                   **error);


G_END_DECLS

#endif /* __HIEROGLYPH_HGARRAY_H__ */