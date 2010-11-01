/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpath.h
 * Copyright (C) 2006-2010 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __HIEROGLYPH_HGPATH_H__
#define __HIEROGLYPH_HGPATH_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

#define HG_PATH_INIT						\
	(hg_object_register(HG_TYPE_PATH,			\
			    hg_object_path_get_vtable()))
#define HG_IS_QPATH(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_PATH)

typedef struct _hg_path_t	hg_path_t;
typedef enum _hg_path_type_t	hg_path_type_t;
typedef struct _hg_path_node_t	hg_path_node_t;


/* ref. PLRM 4.6.2. */
enum _hg_path_type_t {
	HG_PATH_SETBBOX   = 0,
	HG_PATH_MOVETO    = 1,
	HG_PATH_RMOVETO   = 2,
	HG_PATH_LINETO    = 3,
	HG_PATH_RLINETO   = 4,
	HG_PATH_CURVETO   = 5,
	HG_PATH_RCURVETO  = 6,
	HG_PATH_ARC       = 7,
	HG_PATH_ARCN      = 8,
	HG_PATH_ARCT      = 9,
	HG_PATH_CLOSEPATH = 10,
	HG_PATH_UCACHE    = 11,
	HG_PATH_END
};

struct _hg_path_t {
	hg_object_t o;
	hg_quark_t  qnode;
	gsize       length;
	gsize       estimated_point;
	gdouble     x;
	gdouble     y;
};
struct _hg_path_node_t {
	hg_path_type_t type;
	gdouble        x;
	gdouble        y;
};


hg_object_vtable_t *hg_object_path_get_vtable(void) G_GNUC_CONST;
hg_quark_t          hg_path_new              (hg_mem_t       *mem,
                                              gpointer       *ret);
gboolean            hg_path_add              (hg_path_t      *path,
                                              hg_path_type_t  type,
                                              gdouble         x,
                                              gdouble         y);
gboolean            hg_path_close            (hg_path_t      *path);
gboolean            hg_path_get_current_point(hg_path_t      *path,
                                              gdouble        *x,
                                              gdouble        *y);
gboolean            hg_path_moveto           (hg_path_t      *path,
                                              gdouble         x,
                                              gdouble         y);
gboolean            hg_path_rmoveto          (hg_path_t      *path,
                                              gdouble         x,
                                              gdouble         y);
gboolean            hg_path_lineto           (hg_path_t      *path,
                                              gdouble         x,
                                              gdouble         y);
gboolean            hg_path_rlineto          (hg_path_t      *path,
                                              gdouble         x,
                                              gdouble         y);
gboolean            hg_path_curveto          (hg_path_t      *path,
                                              gdouble         x1,
                                              gdouble         y1,
                                              gdouble         x2,
                                              gdouble         y2,
                                              gdouble         x3,
                                              gdouble         y3);
gboolean            hg_path_rcurveto         (hg_path_t      *path,
                                              gdouble         x1,
                                              gdouble         y1,
                                              gdouble         x2,
                                              gdouble         y2,
                                              gdouble         x3,
                                              gdouble         y3);
gboolean            hg_path_arc              (hg_path_t      *path,
                                              gdouble         x,
                                              gdouble         y,
                                              gdouble         r,
                                              gdouble         angle1,
                                              gdouble         angle2);
gboolean            hg_path_arcn             (hg_path_t      *path,
                                              gdouble         x,
                                              gdouble         y,
                                              gdouble         r,
                                              gdouble         angle1,
                                              gdouble         angle2);
gboolean            hg_path_arcto            (hg_path_t      *path,
					      gdouble         x1,
					      gdouble         y1,
					      gdouble         x2,
					      gdouble         y2,
					      gdouble         r,
					      gdouble        *tp[4]);

G_END_DECLS

#endif /* __HIEROGLYPH_HGPATH_H__ */
