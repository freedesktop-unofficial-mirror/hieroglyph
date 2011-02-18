/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpath.h
 * Copyright (C) 2006-2011 Akira TAGOH
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGPATH_H__
#define __HIEROGLYPH_HGPATH_H__

#include <hieroglyph/hgobject.h>

HG_BEGIN_DECLS

#define HG_PATH_INIT						\
	(hg_object_register(HG_TYPE_PATH,			\
			    hg_object_path_get_vtable()))
#define HG_IS_QPATH(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_PATH)

typedef struct _hg_path_t			hg_path_t;
typedef struct _hg_path_bbox_t			hg_path_bbox_t;
typedef enum _hg_path_type_t			hg_path_type_t;
typedef struct _hg_path_node_t			hg_path_node_t;
typedef struct _hg_path_operate_vtable_t	hg_path_operate_vtable_t;


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
	hg_usize_t  length;
};
struct _hg_path_node_t {
	hg_path_type_t type;
	hg_real_t      dx;
	hg_real_t      dy;
	hg_real_t      cx;
	hg_real_t      cy;
};
struct _hg_path_bbox_t {
	hg_real_t llx;
	hg_real_t lly;
	hg_real_t urx;
	hg_real_t ury;
};
struct _hg_path_operate_vtable_t {
	void (* new_path)   (hg_pointer_t user_data);
	void (* close_path) (hg_pointer_t user_data);
	void (* moveto)     (hg_pointer_t user_data,
			     hg_real_t    x,
			     hg_real_t    y);
	void (* rmoveto)    (hg_pointer_t user_data,
			     hg_real_t    rx,
			     hg_real_t    ry);
	void (* lineto)     (hg_pointer_t user_data,
			     hg_real_t    x,
			     hg_real_t    y);
	void (* rlineto)    (hg_pointer_t user_data,
			     hg_real_t    rx,
			     hg_real_t    ry);
	void (* curveto)    (hg_pointer_t user_data,
			     hg_real_t    x1,
			     hg_real_t    y1,
			     hg_real_t    x2,
			     hg_real_t    y2,
			     hg_real_t    x3,
			     hg_real_t    y3);
	void (* rcurveto)   (hg_pointer_t user_data,
			     hg_real_t    rx1,
			     hg_real_t    ry1,
			     hg_real_t    rx2,
			     hg_real_t    ry2,
			     hg_real_t    rx3,
			     hg_real_t    ry3);
};


hg_object_vtable_t *hg_object_path_get_vtable(void) HG_GNUC_CONST;
hg_quark_t          hg_path_new              (hg_mem_t                 *mem,
                                              hg_pointer_t             *ret);
hg_bool_t           hg_path_get_current_point(hg_path_t                *path,
                                              hg_real_t                *x,
                                              hg_real_t                *y);
hg_bool_t           hg_path_close            (hg_path_t                *path);
hg_bool_t           hg_path_moveto           (hg_path_t                *path,
                                              hg_real_t                 x,
                                              hg_real_t                 y);
hg_bool_t           hg_path_rmoveto          (hg_path_t                *path,
                                              hg_real_t                 x,
                                              hg_real_t                 y);
hg_bool_t           hg_path_lineto           (hg_path_t                *path,
                                              hg_real_t                 x,
                                              hg_real_t                 y);
hg_bool_t           hg_path_rlineto          (hg_path_t                *path,
                                              hg_real_t                 x,
                                              hg_real_t                 y);
hg_bool_t           hg_path_curveto          (hg_path_t                *path,
                                              hg_real_t                 x1,
                                              hg_real_t                 y1,
                                              hg_real_t                 x2,
                                              hg_real_t                 y2,
                                              hg_real_t                 x3,
                                              hg_real_t                 y3);
hg_bool_t           hg_path_rcurveto         (hg_path_t                *path,
                                              hg_real_t                 x1,
                                              hg_real_t                 y1,
                                              hg_real_t                 x2,
                                              hg_real_t                 y2,
                                              hg_real_t                 x3,
                                              hg_real_t                 y3);
hg_bool_t           hg_path_arc              (hg_path_t                *path,
                                              hg_real_t                 x,
                                              hg_real_t                 y,
                                              hg_real_t                 r,
                                              hg_real_t                 angle1,
                                              hg_real_t                 angle2);
hg_bool_t           hg_path_arcn             (hg_path_t                *path,
                                              hg_real_t                 x,
                                              hg_real_t                 y,
                                              hg_real_t                 r,
                                              hg_real_t                 angle1,
                                              hg_real_t                 angle2);
hg_bool_t           hg_path_arcto            (hg_path_t                *path,
                                              hg_real_t                 x1,
                                              hg_real_t                 y1,
                                              hg_real_t                 x2,
                                              hg_real_t                 y2,
                                              hg_real_t                 r,
                                              hg_real_t                 tp[4]);
hg_bool_t           hg_path_operate          (hg_path_t                *path,
                                              hg_path_operate_vtable_t *vtable,
                                              hg_pointer_t              user_data);
hg_bool_t           hg_path_get_bbox         (hg_path_t                *path,
                                              hg_bool_t                 ignore_last_moveto,
                                              hg_path_bbox_t           *ret);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGPATH_H__ */
