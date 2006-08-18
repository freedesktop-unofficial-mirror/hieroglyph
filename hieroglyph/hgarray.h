/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.h
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
#ifndef __HG_ARRAY_H__
#define __HG_ARRAY_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

#define hg_array_free(obj)	hg_mem_free(obj)

HgArray     *hg_array_new             (HgMemPool     *pool,
				       gint32         num);
gboolean     hg_array_append          (HgArray       *array,
				       HgValueNode   *node);
gboolean     hg_array_append_forcibly (HgArray       *array,
				       HgValueNode   *node,
				       gboolean       force);
gboolean     hg_array_replace         (HgArray       *array,
				       HgValueNode   *node,
				       guint          index);
gboolean     hg_array_replace_forcibly(HgArray       *array,
				       HgValueNode   *node,
				       guint          index,
				       gboolean       force);
gboolean     hg_array_remove          (HgArray       *array,
				       guint          index);
HgValueNode *hg_array_index           (const HgArray *array,
				       guint          index);
guint        hg_array_length          (HgArray       *array);
gboolean     hg_array_fix_array_size  (HgArray       *array);
HgArray     *hg_array_make_subarray   (HgMemPool     *pool,
				       HgArray       *array,
				       guint          start_index,
				       guint          end_index);
gboolean     hg_array_copy_as_subarray(HgArray       *src,
				       HgArray       *dest,
				       guint          start_index,
				       guint          end_index);
gboolean     hg_array_compare         (const HgArray *a,
				       const HgArray *b);


G_END_DECLS

#endif /* __HG_ARRAY_H__ */
