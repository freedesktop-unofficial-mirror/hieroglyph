/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * lbgraphics.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgmatrix.h>
#include <hieroglyph/hgpage.h>
#include <hieroglyph/hgpath.h>
#include <hieroglyph/hgrender.h>
#include "lbgraphics.h"


static void     _libretto_graphic_state_real_set_flags(gpointer           data,
						       guint              flags);
static void     _libretto_graphic_state_real_relocate (gpointer           data,
						       HgMemRelocateInfo *info);
static gpointer _libretto_graphic_state_real_copy     (gpointer           data);
static void     _libretto_graphics_real_free          (gpointer           data);
static void     _libretto_graphics_real_set_flags     (gpointer           data,
						       guint              flags);
static void     _libretto_graphics_real_relocate      (gpointer           data,
						       HgMemRelocateInfo *info);


static HgObjectVTable __lb_gstate_vtable = {
	.free      = NULL,
	.set_flags = _libretto_graphic_state_real_set_flags,
	.relocate  = _libretto_graphic_state_real_relocate,
	.dup       = NULL,
	.copy      = _libretto_graphic_state_real_copy,
	.to_string = NULL,
};

static HgObjectVTable __lb_graphics_vtable = {
	.free      = _libretto_graphics_real_free,
	.set_flags = _libretto_graphics_real_set_flags,
	.relocate  = _libretto_graphics_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_libretto_graphic_state_real_set_flags(gpointer data,
				       guint    flags)
{
	LibrettoGraphicState *gstate = data;
	HgMemObject *obj;

#define _libretto_graphic_state_set_mark(member)			\
	if (gstate->member) {						\
		hg_mem_get_object__inline(gstate->member, obj);		\
		if (obj == NULL) {					\
			g_warning("Invalid object %p to be marked: Graphics", gstate->member); \
		} else {						\
			hg_mem_add_flags__inline(obj, flags, TRUE);	\
		}							\
	}

	_libretto_graphic_state_set_mark(path);
	_libretto_graphic_state_set_mark(clip_path);
	_libretto_graphic_state_set_mark(color_space);
	_libretto_graphic_state_set_mark(font);
	_libretto_graphic_state_set_mark(dashline_pattern);

#undef _libretto_graphic_state_set_mark
}

static void
_libretto_graphic_state_real_relocate(gpointer           data,
				      HgMemRelocateInfo *info)
{
	LibrettoGraphicState *gstate = data;

#define _libretto_graphic_state_relocate(member)			\
	if ((gsize)gstate->member >= info->start &&			\
	    (gsize)gstate->member <= info->end) {			\
		gstate->member = (gpointer)((gsize)gstate->member + info->diff); \
	}

	_libretto_graphic_state_relocate(path);
	_libretto_graphic_state_relocate(clip_path);
	_libretto_graphic_state_relocate(color_space);
	_libretto_graphic_state_relocate(font);
	_libretto_graphic_state_relocate(dashline_pattern);

#undef _libretto_graphic_state_relocate
}

static gpointer
_libretto_graphic_state_real_copy(gpointer data)
{
	LibrettoGraphicState *gstate = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = libretto_graphic_state_new(obj->pool);
	if (retval == NULL) {
		g_warning("Failed to duplicate a graphic state.");
		return NULL;
	}
	memcpy(retval, data, sizeof (LibrettoGraphicState));
	retval->path = hg_object_copy((HgObject *)gstate->path);
	if (retval->path == NULL)
		return NULL;
	retval->clip_path = hg_object_copy((HgObject *)gstate->clip_path);
	if (retval->clip_path == NULL)
		return NULL;
	retval->color_space = hg_object_copy((HgObject *)gstate->color_space);
	retval->font = hg_object_copy((HgObject *)gstate->font);
	retval->dashline_pattern = hg_object_copy((HgObject *)gstate->dashline_pattern);

	/* FIXME */

	return retval;
}

static void
_libretto_graphics_real_free(gpointer data)
{
	LibrettoGraphics *graphics = data;

	if (graphics->pages) {
		GList *l = graphics->pages;

		while (l) {
			hg_page_destroy(l->data);
			l = g_list_next(l);
		}
		g_list_free(graphics->pages);
	}
	/* FIXME: current_page too? */
	if (graphics->gstate_stack)
		g_list_free(graphics->gstate_stack);
}

static void
_libretto_graphics_real_set_flags(gpointer data,
				  guint    flags)
{
	LibrettoGraphics *graphics = data;
	HgMemObject *obj;
	GList *l;

#define _libretto_graphics_set_mark(member)				\
	if (member) {							\
		hg_mem_get_object__inline(member, obj);			\
		if (obj == NULL) {					\
			g_warning("Invalid object %p to be marked: Graphics", member); \
		} else {						\
			hg_mem_add_flags__inline(obj, flags, TRUE);	\
		}							\
	}

	_libretto_graphics_set_mark(graphics->current_gstate);
	for (l = graphics->gstate_stack; l != NULL; l = g_list_next(l)) {
		_libretto_graphics_set_mark(l->data);
	}

#undef _libretto_graphics_set_mark
}

static void
_libretto_graphics_real_relocate(gpointer           data,
				 HgMemRelocateInfo *info)
{
	LibrettoGraphics *graphics = data;
	GList *l;

#define _libretto_graphics_relocate(member)			\
	if ((gsize)member >= info->start &&			\
	    (gsize)member <= info->end) {			\
		member = (gpointer)((gsize)member + info->diff); \
	}

	_libretto_graphics_relocate(graphics->current_gstate);
	for (l = graphics->gstate_stack; l != NULL; l = g_list_next(l)) {
		_libretto_graphics_relocate(l->data);
	}

#undef _libretto_graphics_relocate
}

/*
 * Public Functions
 */
LibrettoGraphicState *
libretto_graphic_state_new(HgMemPool *pool)
{
	LibrettoGraphicState *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	while (1) {
		retval = hg_mem_alloc_with_flags(pool,
						 sizeof (LibrettoGraphicState),
						 HG_FL_COMPLEX);
		if (retval == NULL)
			break;
		HG_SET_MAGIC_CODE (&retval->object, HG_OBJECT_ID);
		HG_OBJECT_INIT_STATE (&retval->object);
		HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
		hg_object_set_vtable(&retval->object, &__lb_gstate_vtable);

		retval->path = hg_path_new(pool);
		if (retval->path == NULL)
			break;
		retval->clip_path = hg_path_new(pool);
		if (retval->clip_path == NULL)
			break;
		retval->color_space = hg_array_new(pool, 0); /* FIXME */
		libretto_graphic_state_color_set_rgb(retval, 0.0, 0.0, 0.0);
		retval->font = hg_dict_new(pool, 1); /* FIXME */
		retval->line_width = 1.0;
		retval->line_cap = 0;
		retval->line_join = 0;
		retval->miter_limit = 10.0;
		retval->dashline_offset = 0;
		retval->dashline_pattern = hg_array_new(pool, 0);
		if (retval->dashline_pattern == NULL)
			break;
		retval->stroke_correction = FALSE;

		return retval;
	}
	g_warning("Failed to create a graphic state.");

	return NULL;
}

LibrettoGraphics *
libretto_graphics_new(HgMemPool *pool)
{
	LibrettoGraphics *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	while (1) {
		retval = hg_mem_alloc(pool, sizeof (LibrettoGraphics));
		if (retval == NULL)
			break;
		HG_SET_MAGIC_CODE (&retval->object, HG_OBJECT_ID);
		HG_OBJECT_INIT_STATE (&retval->object);
		HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
		hg_object_set_vtable(&retval->object, &__lb_graphics_vtable);

		retval->pool = pool;
		retval->current_gstate = libretto_graphic_state_new(pool);
		retval->current_page = hg_page_new_with_pagesize(HG_PAGE_A4);
		retval->pages = g_list_append(NULL, retval->current_page);
		if (retval->current_page == NULL)
			break;
		libretto_graphics_init(retval);

		return retval;
	}
	g_warning("Failed to create a graphic state.");

	return NULL;
}

gboolean
libretto_graphics_init(LibrettoGraphics *graphics)
{
	LibrettoGraphicState *gstate;
	HgMatrix *mtx;

	g_return_val_if_fail (graphics != NULL, FALSE);

	gstate = libretto_graphics_get_state(graphics);
	mtx = hg_matrix_new(graphics->pool, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
	memcpy(&gstate->ctm, mtx, sizeof (HgMatrix));
	memcpy(&gstate->snapshot_matrix, mtx, sizeof (HgMatrix));
	hg_mem_free(mtx);
	gstate->x = 0.0;
	gstate->y = 0.0;
	hg_path_clear(gstate->path, TRUE);
	libretto_graphics_set_graphic_size(graphics, graphics->current_page->width,
					   graphics->current_page->height);
	/* FIXME: color space */
	libretto_graphic_state_color_set_rgb(gstate, 0.0, 0.0, 0.0);
	gstate->line_width = 1.0;
	gstate->line_cap = 0;
	gstate->line_join = 0;
	gstate->miter_limit = 10.0;
	/* FIXME: dashline */

	return TRUE;
}

gboolean
libretto_graphics_initclip(LibrettoGraphics *graphics)
{
	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_page != NULL, FALSE);

	hg_path_clear(graphics->current_gstate->clip_path, TRUE);
	hg_path_moveto(graphics->current_gstate->clip_path, 0, 0);
	hg_path_lineto(graphics->current_gstate->clip_path,
		       graphics->current_page->width, 0);
	hg_path_lineto(graphics->current_gstate->clip_path,
		       graphics->current_page->width, graphics->current_page->height);
	hg_path_lineto(graphics->current_gstate->clip_path,
		       0, graphics->current_page->height);

	return hg_path_close(graphics->current_gstate->clip_path);
}

gboolean
libretto_graphics_set_page_size(LibrettoGraphics *graphics,
				HgPageSize        size)
{
	gdouble width, height;

	g_return_val_if_fail (graphics != NULL, FALSE);

	hg_page_get_size(size, &width, &height);

	return libretto_graphics_set_graphic_size(graphics, width, height);
}

gboolean
libretto_graphics_set_graphic_size(LibrettoGraphics *graphics,
				   gdouble           width,
				   gdouble           height)
{
	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	if (graphics->current_page->node) {
		g_warning("Can't change the page size after the rendering code is invoked.");
		return FALSE;
	}

	return libretto_graphics_initclip(graphics);
}

gboolean
libretto_graphics_save(LibrettoGraphics *graphics)
{
	gpointer data;

	g_return_val_if_fail (graphics != NULL, FALSE);

	data = hg_object_copy((HgObject *)graphics->current_gstate);
	graphics->gstate_stack = g_list_append(graphics->gstate_stack, data);

	return TRUE;
}

gboolean
libretto_graphics_restore(LibrettoGraphics *graphics)
{
	GList *l;
	LibrettoGraphicState *gstate;

	g_return_val_if_fail (graphics != NULL, FALSE);

	l = g_list_last(graphics->gstate_stack);
	if (l != NULL) {
		if (l->prev)
			l->prev->next = NULL;
		else
			graphics->gstate_stack = NULL;
		gstate = l->data;
		g_list_free_1(l);
		hg_mem_free(graphics->current_gstate);
		graphics->current_gstate = gstate;
	}

	return TRUE;
}

gboolean
libretto_graphics_show_page(LibrettoGraphics *graphics)
{
	GList *l;

	g_return_val_if_fail (graphics != NULL, FALSE);

	l = g_list_last(graphics->pages);
	if (l != NULL && l->data != graphics->current_page)
		graphics->pages = g_list_append(graphics->pages, graphics->current_page);
	graphics->current_page = hg_page_new_with_size(graphics->current_page->width,
						       graphics->current_page->height);
	/* FIXME */
	return libretto_graphics_init(graphics);
}

/* matrix */
gboolean
libretto_graphics_matrix_rotate(LibrettoGraphics *graphics,
				gdouble           angle)
{
	LibrettoGraphicState *gstate;
	HgMatrix *mtx, *new_ctm;

	g_return_val_if_fail (graphics != NULL, FALSE);

	mtx = hg_matrix_rotate(graphics->pool, angle);
	new_ctm = hg_matrix_multiply(graphics->pool, mtx, &graphics->current_gstate->ctm);
	memcpy(&graphics->current_gstate->ctm, new_ctm, sizeof (HgMatrix));
	hg_mem_free(mtx);
	hg_mem_free(new_ctm);

	gstate = libretto_graphics_get_state(graphics);

	return hg_path_matrix(gstate->path,
			      gstate->ctm.xx, gstate->ctm.yx,
			      gstate->ctm.xy, gstate->ctm.yy,
			      gstate->ctm.x0, gstate->ctm.y0);
}

gboolean
libretto_graphics_matrix_scale(LibrettoGraphics *graphics,
			       gdouble           x,
			       gdouble           y)
{
	LibrettoGraphicState *gstate;
	HgMatrix *mtx, *new_ctm;

	g_return_val_if_fail (graphics != NULL, FALSE);

	mtx = hg_matrix_scale(graphics->pool, x, y);
	new_ctm = hg_matrix_multiply(graphics->pool, mtx, &graphics->current_gstate->ctm);
	memcpy(&graphics->current_gstate->ctm, new_ctm, sizeof (HgMatrix));
	hg_mem_free(mtx);
	hg_mem_free(new_ctm);

	gstate = libretto_graphics_get_state(graphics);

	return hg_path_matrix(gstate->path,
			      gstate->ctm.xx, gstate->ctm.yx,
			      gstate->ctm.xy, gstate->ctm.yy,
			      gstate->ctm.x0, gstate->ctm.y0);
}

gboolean
libretto_graphics_matrix_translate(LibrettoGraphics *graphics,
				   gdouble           x,
				   gdouble           y)
{
	LibrettoGraphicState *gstate;
	HgMatrix *mtx, *new_ctm;

	g_return_val_if_fail (graphics != NULL, FALSE);

	mtx = hg_matrix_translate(graphics->pool, x, y);
	new_ctm = hg_matrix_multiply(graphics->pool, mtx, &graphics->current_gstate->ctm);
	memcpy(&graphics->current_gstate->ctm, new_ctm, sizeof (HgMatrix));
	hg_mem_free(mtx);
	hg_mem_free(new_ctm);

	gstate = libretto_graphics_get_state(graphics);

	return hg_path_matrix(gstate->path,
			      gstate->ctm.xx, gstate->ctm.yx,
			      gstate->ctm.xy, gstate->ctm.yy,
			      gstate->ctm.x0, gstate->ctm.y0);
}

/* path */
gboolean
libretto_graphic_state_path_new(LibrettoGraphicState *gstate)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	if (gstate->path) {
		hg_path_clear(gstate->path, TRUE);
		/* snapshot_matrix needs to be updated because
		 * path might has HG_PATH_MATRIX and this operator initializes it.
		 * it won't be tracable then.
		 */
		memcpy(&gstate->snapshot_matrix,
		       &gstate->ctm,
		       sizeof (HgMatrix));

		return TRUE;
	}

	return FALSE;
}

gboolean
libretto_graphic_state_path_close(LibrettoGraphicState *gstate)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_close(gstate->path);
}

gboolean
libretto_graphic_state_path_from_clip(LibrettoGraphicState *gstate)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_copy(gstate->clip_path, gstate->path);
}

gboolean
libretto_graphic_state_get_bbox_from_path(LibrettoGraphicState *gstate,
					  gboolean              ignore_moveto,
					  HgPathBBox           *bbox)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_get_bbox(gstate->path, ignore_moveto, bbox);
}

gboolean
libretto_graphic_state_path_moveto(LibrettoGraphicState *gstate,
				   gdouble               x,
				   gdouble               y)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_moveto(gstate->path, x, y);
}

gboolean
libretto_graphic_state_path_lineto(LibrettoGraphicState *gstate,
				   gdouble               x,
				   gdouble               y)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_lineto(gstate->path, x, y);
}

gboolean
libretto_graphic_state_path_rlineto(LibrettoGraphicState *gstate,
				    gdouble               x,
				    gdouble               y)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_rlineto(gstate->path, x, y);
}

gboolean
libretto_graphic_state_path_curveto(LibrettoGraphicState *gstate,
				    gdouble               x1,
				    gdouble               y1,
				    gdouble               x2,
				    gdouble               y2,
				    gdouble               x3,
				    gdouble               y3)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_curveto(gstate->path, x1, y1, x2, y2, x3, y3);
}

gboolean
libretto_graphic_state_path_arc(LibrettoGraphicState *gstate,
				gdouble               x,
				gdouble               y,
				gdouble               r,
				gdouble               angle1,
				gdouble               angle2)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_arc(gstate->path, x, y, r, angle1, angle2);
}

/* color */
gboolean
libretto_graphic_state_color_set_rgb(LibrettoGraphicState *gstate,
				     gdouble               red,
				     gdouble               green,
				     gdouble               blue)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	gstate->color.is_rgb = TRUE;
	gstate->color.is.rgb.r = red;
	gstate->color.is.rgb.g = green;
	gstate->color.is.rgb.b = blue;

	return TRUE;
}

gboolean
libretto_graphic_state_color_set_hsv(LibrettoGraphicState *gstate,
				     gdouble               hue,
				     gdouble               saturation,
				     gdouble               value)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	gstate->color.is_rgb = FALSE;
	gstate->color.is.hsv.h = hue;
	gstate->color.is.hsv.s = saturation;
	gstate->color.is.hsv.v = value;

	return TRUE;
}

/* rendering */
gboolean
libretto_graphics_render_eofill(LibrettoGraphics *graphics)
{
	HgRender *render;
	LibrettoGraphicState *gstate;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	gstate = libretto_graphics_get_state(graphics);
	render = hg_render_eofill_new(graphics->pool,
				      &gstate->snapshot_matrix,
				      gstate->path->node,
				      &gstate->color);
	if (render == NULL)
		return FALSE;
	hg_page_append_node(graphics->current_page, render);
	hg_path_clear(gstate->path, FALSE);
	/* snapshot_matrix needs to be updated because
	 * path might has HG_PATH_MATRIX and this operator initializes it.
	 * it won't be tracable then.
	 */
	memcpy(&gstate->snapshot_matrix, &gstate->ctm, sizeof (HgMatrix));

	return TRUE;
}

gboolean
libretto_graphics_render_fill(LibrettoGraphics *graphics)
{
	LibrettoGraphicState *gstate;
	HgRender *render;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	gstate = libretto_graphics_get_state(graphics);
	render = hg_render_fill_new(graphics->pool,
				    &gstate->snapshot_matrix,
				    gstate->path->node,
				    &gstate->color);
	if (render == NULL)
		return FALSE;
	hg_page_append_node(graphics->current_page, render);
	hg_path_clear(gstate->path, FALSE);
	/* snapshot_matrix needs to be updated because
	 * path might has HG_PATH_MATRIX and this operator initializes it.
	 * it won't be tracable then.
	 */
	memcpy(&gstate->snapshot_matrix, &gstate->ctm, sizeof (HgMatrix));

	return TRUE;
}

gboolean
libretto_graphics_render_stroke(LibrettoGraphics *graphics)
{
	LibrettoGraphicState *gstate;
	HgRender *render;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	gstate = libretto_graphics_get_state(graphics);
	render = hg_render_stroke_new(graphics->pool,
				      &gstate->snapshot_matrix,
				      gstate->path->node,
				      &gstate->color,
				      gstate->line_width,
				      gstate->line_cap,
				      gstate->line_join,
				      gstate->miter_limit,
				      gstate->dashline_offset,
				      gstate->dashline_pattern);
	if (render == NULL)
		return FALSE;
	hg_page_append_node(graphics->current_page, render);
	hg_path_clear(gstate->path, FALSE);
	/* snapshot_matrix needs to be updated because
	 * path might has HG_PATH_MATRIX and this operator initializes it.
	 * it won't be tracable then.
	 */
	memcpy(&gstate->snapshot_matrix, &gstate->ctm, sizeof (HgMatrix));

	return TRUE;
}

/* state */
gboolean
libretto_graphic_state_set_linewidth(LibrettoGraphicState *gstate,
				     gdouble               width)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	gstate->line_width = width;

	return TRUE;
}

/* debugging support */
gboolean
libretto_graphics_debug(LibrettoGraphics *graphics,
			HgDebugFunc       func,
			gpointer          data)
{
	HgRender *render;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);
	g_return_val_if_fail (func != NULL, FALSE);

	render = hg_render_debug_new(graphics->pool, func, data);
	if (render == NULL)
		return FALSE;
	hg_page_append_node(graphics->current_page, render);

	return TRUE;
}
