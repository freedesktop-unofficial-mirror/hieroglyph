/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpath.h
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
#ifndef __HG_PATH_H__
#define __HG_PATH_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

HgPathNode *hg_path_node_new             (HgMemPool  *pool,
					  HgPathType  type,
					  gdouble     x,
					  gdouble     y);
void        hg_path_node_free            (HgPathNode *node);
HgPath     *hg_path_new                  (HgMemPool  *pool);
gboolean    hg_path_copy                 (HgPath     *src,
					  HgPath     *dest);
gboolean    hg_path_find                 (HgPath     *path,
					  HgPathType  type);
gboolean    hg_path_compute_current_point(HgPath     *path,
					  gdouble    *x,
					  gdouble    *y);
gboolean    hg_path_get_bbox             (HgPath     *path,
					  gboolean    ignore_moveto,
					  HgPathBBox *bbox);
gboolean    hg_path_clear                (HgPath     *path,
					  gboolean    free_segment);
gboolean    hg_path_close                (HgPath     *path);
gboolean    hg_path_moveto               (HgPath     *path,
					  gdouble     x,
					  gdouble     y);
gboolean    hg_path_lineto               (HgPath     *path,
					  gdouble     x,
					  gdouble     y);
gboolean    hg_path_rlineto              (HgPath     *path,
					  gdouble     x,
					  gdouble     y);
gboolean    hg_path_curveto              (HgPath     *path,
					  gdouble     x1,
					  gdouble     y1,
					  gdouble     x2,
					  gdouble     y2,
					  gdouble     x3,
					  gdouble     y3);
gboolean    hg_path_arc                  (HgPath     *path,
					  gdouble     x,
					  gdouble     y,
					  gdouble     r,
					  gdouble     angle1,
					  gdouble     angle2);
gboolean    hg_path_matrix               (HgPath     *path,
					  gdouble     xx,
					  gdouble     yx,
					  gdouble     xy,
					  gdouble     yy,
					  gdouble     x0,
					  gdouble     y0);


G_END_DECLS

#endif /* __HG_PATH_H__ */
