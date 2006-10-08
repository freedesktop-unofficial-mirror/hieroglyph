/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * iarray.h
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
#ifndef __HG_INT_ARRAY_H__
#define __HG_INT_ARRAY_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

typedef struct _HieroGlyphIntArray	HgIntArray;

#define hg_int_array_free(obj)	hg_mem_free(obj)

HgIntArray *hg_int_array_new    (HgMemPool        *pool,
				 gint32            num);
gboolean    hg_int_array_append (HgIntArray       *array,
				 gint32            i);
gboolean    hg_int_array_replace(HgIntArray       *array,
				 gint32            i,
				 guint             index);
gboolean    hg_int_array_remove (HgIntArray       *array,
				 guint             index);
gint32      hg_int_array_index  (const HgIntArray *array,
				 guint             index);
guint       hg_int_array_length (HgIntArray       *array);


G_END_DECLS

#endif /* __HG_INT_ARRAY_H__ */
