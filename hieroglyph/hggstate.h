/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggsate.h
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

#ifndef __HIEROGLYPH_HGGSTATE_H__
#define __HIEROGLYPH_HGGSTATE_H__

#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgmatrix.h>
#include <hieroglyph/hgpath.h>

HG_BEGIN_DECLS

#define HG_GSTATE_INIT						\
	(hg_object_register(HG_TYPE_GSTATE,			\
			    hg_object_gstate_get_vtable()))
#define HG_IS_QGSTATE(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_GSTATE)

typedef struct _hg_gstate_t	hg_gstate_t;
typedef struct _hg_color_t	hg_color_t;
typedef enum _hg_color_mode_t	hg_color_mode_t;
typedef enum _hg_linecap_t	hg_linecap_t;
typedef enum _hg_linejoin_t	hg_linejoin_t;

enum _hg_color_mode_t {
	HG_COLOR_RGB,
	HG_COLOR_HSB,
	HG_COLOR_GRAY,
};
enum _hg_linecap_t {
	HG_LINECAP_BUTT = 0,
	HG_LINECAP_ROUND,
	HG_LINECAP_SQUARE,
	HG_LINECAP_END
};
enum _hg_linejoin_t {
	HG_LINEJOIN_MITER = 0,
	HG_LINEJOIN_ROUND,
	HG_LINEJOIN_BEVEL,
	HG_LINEJOIN_END
};
struct _hg_color_t {
	hg_color_mode_t type;
	union {
		struct {
			hg_real_t red;
			hg_real_t green;
			hg_real_t blue;
		} rgb;
		struct {
			hg_real_t hue;
			hg_real_t saturation;
			hg_real_t brightness;
		} hsb;
	} is;
};
struct _hg_gstate_t {
	hg_object_t   o;
	hg_quark_t    qpath;
	hg_quark_t    qclippath;
	hg_quark_t    qdashpattern;
	hg_matrix_t   ctm;
	hg_color_t    color;
	hg_linecap_t  linecap;
	hg_linejoin_t linejoin;
	hg_real_t     linewidth;
	hg_real_t     miterlen;
	hg_real_t     dash_offset;
	hg_bool_t     is_snapshot:1;
};

hg_object_vtable_t *hg_object_gstate_get_vtable(void) HG_GNUC_CONST;
hg_quark_t          hg_gstate_new              (hg_mem_t      *mem,
                                                hg_pointer_t  *ret);
void                hg_gstate_set_ctm          (hg_gstate_t   *gstate,
						hg_matrix_t   *matrix);
void                hg_gstate_get_ctm          (hg_gstate_t   *gstate,
						hg_matrix_t   *matrix);
void                hg_gstate_set_path         (hg_gstate_t   *gstate,
                                                hg_quark_t     qpath);
hg_quark_t          hg_gstate_get_path         (hg_gstate_t   *gstate);
void                hg_gstate_set_clippath     (hg_gstate_t   *gstate,
                                                hg_quark_t     qpath);
hg_quark_t          hg_gstate_get_clippath     (hg_gstate_t   *gstate);
void                hg_gstate_set_rgbcolor     (hg_gstate_t   *gstate,
						hg_real_t      red,
						hg_real_t      green,
						hg_real_t      blue);
void                hg_gstate_set_hsbcolor     (hg_gstate_t   *gstate,
						hg_real_t      hue,
						hg_real_t      saturation,
						hg_real_t      brightness);
void                hg_gstate_set_graycolor    (hg_gstate_t   *gstate,
						hg_real_t      gray);
void                hg_gstate_set_linewidth    (hg_gstate_t   *gstate,
						hg_real_t      width);
void                hg_gstate_set_linecap      (hg_gstate_t   *gstate,
						hg_linecap_t   linecap);
void                hg_gstate_set_linejoin     (hg_gstate_t   *gstate,
						hg_linejoin_t  linejoin);
hg_bool_t           hg_gstate_set_miterlimit   (hg_gstate_t   *gstate,
						hg_real_t      miterlen);
hg_bool_t           hg_gstate_set_dash         (hg_gstate_t   *gstate,
						hg_quark_t     qpattern,
						hg_real_t      offset);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGGSTATE_H__ */
