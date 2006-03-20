/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * lbgraphics.h
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
#ifndef __LIBRETTO_GRAPHICS_H__
#define __LIBRETTO_GRAPHICS_H__

#include <hieroglyph/hgtypes.h>
#include <libretto/lbtypes.h>

G_BEGIN_DECLS


#define libretto_graphics_get_state(graphics_)		((graphics_)->current_gstate)

LibrettoGraphics *libretto_graphics_new             (HgMemPool        *pool);
gboolean          libretto_graphics_init            (LibrettoGraphics *graphics);
gboolean          libretto_graphics_initclip        (LibrettoGraphics *graphics);
gboolean          libretto_graphics_set_page_size   (LibrettoGraphics *graphics,
						     HgPageSize        size);
gboolean          libretto_graphics_set_graphic_size(LibrettoGraphics *graphics,
						     gdouble           width,
						     gdouble           height);
gboolean          libretto_graphics_show_page       (LibrettoGraphics *graphics);
gboolean          libretto_graphics_save            (LibrettoGraphics *graphics);
gboolean          libretto_graphics_restore         (LibrettoGraphics *graphics);

/* matrix */
gboolean libretto_graphics_matrix_rotate   (LibrettoGraphics *graphics,
					    gdouble           angle);
gboolean libretto_graphics_matrix_scale    (LibrettoGraphics *graphics,
					    gdouble           x,
					    gdouble           y);
gboolean libretto_graphics_matrix_translate(LibrettoGraphics *graphics,
					    gdouble           x,
					    gdouble           y);

/* path */
gboolean libretto_graphic_state_path_new          (LibrettoGraphicState *gstate);
gboolean libretto_graphic_state_path_close        (LibrettoGraphicState *gstate);
gboolean libretto_graphic_state_path_from_clip    (LibrettoGraphicState *gstate);
gboolean libretto_graphic_state_get_bbox_from_path(LibrettoGraphicState *gstate,
						   gboolean              ignore_moveto,
						   HgPathBBox           *bbox);
gboolean libretto_graphic_state_path_moveto       (LibrettoGraphicState *gstate,
						   gdouble               x,
						   gdouble               y);
gboolean libretto_graphic_state_path_lineto       (LibrettoGraphicState *gstate,
						   gdouble               x,
						   gdouble               y);
gboolean libretto_graphic_state_path_rlineto      (LibrettoGraphicState *gstate,
						   gdouble               x,
						   gdouble               y);
gboolean libretto_graphic_state_path_curveto      (LibrettoGraphicState *gstate,
						   gdouble               x1,
						   gdouble               y1,
						   gdouble               x2,
						   gdouble               y2,
						   gdouble               x3,
						   gdouble               y3);
gboolean libretto_graphic_state_path_arc          (LibrettoGraphicState *gstate,
						   gdouble               x,
						   gdouble               y,
						   gdouble               r,
						   gdouble               angle1,
						   gdouble               angle2);

/* color */
gboolean libretto_graphic_state_color_set_rgb(LibrettoGraphicState *graphics,
					      gdouble               red,
					      gdouble               green,
					      gdouble               blue);
gboolean libretto_graphic_state_color_set_hsv(LibrettoGraphicState *graphics,
					      gdouble               hue,
					      gdouble               saturation,
					      gdouble               value);

/* rendering */
gboolean libretto_graphics_render_eofill(LibrettoGraphics *graphics);
gboolean libretto_graphics_render_fill  (LibrettoGraphics *graphics);
gboolean libretto_graphics_render_stroke(LibrettoGraphics *graphics);

/* state */
LibrettoGraphicState *libretto_graphic_state_new          (HgMemPool            *pool);
gboolean              libretto_graphic_state_set_linewidth(LibrettoGraphicState *graphics,
							   gdouble               width);

/* debugging support */
gboolean libretto_graphics_debug(LibrettoGraphics *graphics,
				 HgDebugFunc       func,
				 gpointer          data);


G_END_DECLS

#endif /* __LIBRETTO_GRAPHICS_H__ */
