/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggraphics.h
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
#ifndef __HG_GRAPHICS_H__
#define __HG_GRAPHICS_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


#define hg_graphics_get_state(graphics_)		((graphics_)->current_gstate)

HgGraphics *hg_graphics_new             (HgMemPool  *pool);
gboolean    hg_graphics_init            (HgGraphics *graphics);
gboolean    hg_graphics_initclip        (HgGraphics *graphics);
gboolean    hg_graphics_set_page_size   (HgGraphics *graphics,
					 HgPageSize  size);
gboolean    hg_graphics_set_graphic_size(HgGraphics *graphics,
					 gdouble     width,
					 gdouble     height);
gboolean    hg_graphics_show_page       (HgGraphics *graphics);
gboolean    hg_graphics_save            (HgGraphics *graphics);
gboolean    hg_graphics_restore         (HgGraphics *graphics);

/* matrix */
gboolean hg_graphics_matrix_rotate   (HgGraphics *graphics,
				      gdouble     angle);
gboolean hg_graphics_matrix_scale    (HgGraphics *graphics,
				      gdouble     x,
				      gdouble     y);
gboolean hg_graphics_matrix_translate(HgGraphics *graphics,
				      gdouble     x,
				      gdouble     y);

/* path */
gboolean hg_graphic_state_path_new          (HgGraphicState *gstate);
gboolean hg_graphic_state_path_close        (HgGraphicState *gstate);
gboolean hg_graphic_state_path_from_clip    (HgGraphicState *gstate);
gboolean hg_graphic_state_get_bbox_from_path(HgGraphicState *gstate,
					     gboolean        ignore_moveto,
					     HgPathBBox     *bbox);
gboolean hg_graphic_state_path_moveto       (HgGraphicState *gstate,
					     gdouble         x,
					     gdouble         y);
gboolean hg_graphic_state_path_lineto       (HgGraphicState *gstate,
					     gdouble         x,
					     gdouble         y);
gboolean hg_graphic_state_path_rlineto      (HgGraphicState *gstate,
					     gdouble         x,
					     gdouble         y);
gboolean hg_graphic_state_path_curveto      (HgGraphicState *gstate,
					     gdouble         x1,
					     gdouble         y1,
					     gdouble         x2,
					     gdouble         y2,
					     gdouble         x3,
					     gdouble         y3);
gboolean hg_graphic_state_path_arc          (HgGraphicState *gstate,
					     gdouble         x,
					     gdouble         y,
					     gdouble         r,
					     gdouble         angle1,
					     gdouble         angle2);

/* color */
gboolean hg_graphic_state_color_set_rgb(HgGraphicState *graphics,
					gdouble         red,
					gdouble         green,
					gdouble         blue);
gboolean hg_graphic_state_color_set_hsv(HgGraphicState *graphics,
					gdouble         hue,
					gdouble         saturation,
					gdouble         value);

/* rendering */
gboolean hg_graphics_render_eofill(HgGraphics *graphics);
gboolean hg_graphics_render_fill  (HgGraphics *graphics);
gboolean hg_graphics_render_stroke(HgGraphics *graphics);

/* state */
HgGraphicState *hg_graphic_state_new          (HgMemPool      *pool);
gboolean        hg_graphic_state_set_linewidth(HgGraphicState *graphics,
					       gdouble         width);

/* debugging support */
gboolean hg_graphics_debug(HgGraphics *graphics,
			   HgDebugFunc func,
			   gpointer    data);


G_END_DECLS

#endif /* __HG_GRAPHICS_H__ */
