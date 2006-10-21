/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggraphics.c
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
#include "hggraphics.h"
#include "hglog.h"
#include "hgmem.h"
#include "hgarray.h"
#include "hgdict.h"
#include "hgmatrix.h"
#include "hgpage.h"
#include "hgpath.h"
#include "hgrender.h"


static void     _hg_graphic_state_real_set_flags(gpointer           data,
						 guint              flags);
static void     _hg_graphic_state_real_relocate (gpointer           data,
						 HgMemRelocateInfo *info);
static gpointer _hg_graphic_state_real_copy     (gpointer           data);
static void     _hg_graphics_real_free          (gpointer           data);
static void     _hg_graphics_real_set_flags     (gpointer           data,
						 guint              flags);
static void     _hg_graphics_real_relocate      (gpointer           data,
						 HgMemRelocateInfo *info);


static HgObjectVTable __hg_gstate_vtable = {
	.free      = NULL,
	.set_flags = _hg_graphic_state_real_set_flags,
	.relocate  = _hg_graphic_state_real_relocate,
	.dup       = NULL,
	.copy      = _hg_graphic_state_real_copy,
	.to_string = NULL,
};

static HgObjectVTable __hg_graphics_vtable = {
	.free      = _hg_graphics_real_free,
	.set_flags = _hg_graphics_real_set_flags,
	.relocate  = _hg_graphics_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_hg_graphic_state_real_set_flags(gpointer data,
				 guint    flags)
{
	HgGraphicState *gstate = data;
	HgMemObject *obj;

#define _hg_graphic_state_set_mark(member)				\
	if (gstate->member) {						\
		hg_mem_get_object__inline(gstate->member, obj);		\
		if (obj == NULL) {					\
			hg_log_warning("Invalid object %p to be marked: Graphics", gstate->member); \
		} else {						\
			hg_mem_add_flags__inline(obj, flags, TRUE);	\
		}							\
	}

	_hg_graphic_state_set_mark(path);
	_hg_graphic_state_set_mark(clip_path);
	_hg_graphic_state_set_mark(color_space);
	_hg_graphic_state_set_mark(font);
	_hg_graphic_state_set_mark(dashline_pattern);

#undef _hg_graphic_state_set_mark
}

static void
_hg_graphic_state_real_relocate(gpointer           data,
				HgMemRelocateInfo *info)
{
	HgGraphicState *gstate = data;

#define _hg_graphic_state_relocate(member)				\
	if ((gsize)gstate->member >= info->start &&			\
	    (gsize)gstate->member <= info->end) {			\
		gstate->member = (gpointer)((gsize)gstate->member + info->diff); \
	}

	_hg_graphic_state_relocate(path);
	_hg_graphic_state_relocate(clip_path);
	_hg_graphic_state_relocate(color_space);
	_hg_graphic_state_relocate(font);
	_hg_graphic_state_relocate(dashline_pattern);

#undef _hg_graphic_state_relocate
}

static gpointer
_hg_graphic_state_real_copy(gpointer data)
{
	HgGraphicState *gstate = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_graphic_state_new(obj->pool);
	if (retval == NULL) {
		hg_log_warning("Failed to duplicate a graphic state.");
		return NULL;
	}
	memcpy(retval, data, sizeof (HgGraphicState));
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
_hg_graphics_real_free(gpointer data)
{
	HgGraphics *graphics = data;

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
_hg_graphics_real_set_flags(gpointer data,
			    guint    flags)
{
	HgGraphics *graphics = data;
	HgMemObject *obj;
	GList *l;

#define _hg_graphics_set_mark(member)					\
	if (member) {							\
		hg_mem_get_object__inline(member, obj);			\
		if (obj == NULL) {					\
			hg_log_warning("Invalid object %p to be marked: Graphics", member); \
		} else {						\
			hg_mem_add_flags__inline(obj, flags, TRUE);	\
		}							\
	}

	_hg_graphics_set_mark(graphics->current_gstate);
	for (l = graphics->gstate_stack; l != NULL; l = g_list_next(l)) {
		_hg_graphics_set_mark(l->data);
	}

#undef _hg_graphics_set_mark
}

static void
_hg_graphics_real_relocate(gpointer           data,
			   HgMemRelocateInfo *info)
{
	HgGraphics *graphics = data;
	GList *l;

#define _hg_graphics_relocate(member)				\
	if ((gsize)member >= info->start &&			\
	    (gsize)member <= info->end) {			\
		member = (gpointer)((gsize)member + info->diff); \
	}

	_hg_graphics_relocate(graphics->current_gstate);
	for (l = graphics->gstate_stack; l != NULL; l = g_list_next(l)) {
		_hg_graphics_relocate(l->data);
	}

#undef _hg_graphics_relocate
}

/*
 * Public Functions
 */
HgGraphicState *
hg_graphic_state_new(HgMemPool *pool)
{
	HgGraphicState *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	while (1) {
		retval = hg_mem_alloc_with_flags(pool,
						 sizeof (HgGraphicState),
						 HG_FL_HGOBJECT | HG_FL_COMPLEX);
		if (retval == NULL)
			break;
		HG_OBJECT_INIT_STATE (&retval->object);
		HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
		hg_object_set_vtable(&retval->object, &__hg_gstate_vtable);

		retval->path = hg_path_new(pool);
		if (retval->path == NULL)
			break;
		retval->clip_path = hg_path_new(pool);
		if (retval->clip_path == NULL)
			break;
		retval->color_space = hg_array_new(pool, 0); /* FIXME */
		hg_graphic_state_color_set_rgb(retval, 0.0, 0.0, 0.0);
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
	hg_log_warning("Failed to create a graphic state.");

	return NULL;
}

HgGraphics *
hg_graphics_new(HgMemPool *pool)
{
	HgGraphics *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	while (1) {
		retval = hg_mem_alloc_with_flags(pool, sizeof (HgGraphics),
						 HG_FL_HGOBJECT);
		if (retval == NULL)
			break;
		HG_OBJECT_INIT_STATE (&retval->object);
		HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
		hg_object_set_vtable(&retval->object, &__hg_graphics_vtable);

		retval->pool = pool;
		retval->current_gstate = hg_graphic_state_new(pool);
		retval->current_page = hg_page_new_with_pagesize(HG_PAGE_A4);
		retval->pages = g_list_append(NULL, retval->current_page);
		retval->gstate_stack = NULL;
		if (retval->current_page == NULL)
			break;
		hg_graphics_init(retval);

		return retval;
	}
	hg_log_warning("Failed to create a graphic state.");

	return NULL;
}

gboolean
hg_graphics_init(HgGraphics *graphics)
{
	HgGraphicState *gstate;
	HgMatrix *mtx;

	g_return_val_if_fail (graphics != NULL, FALSE);

	gstate = hg_graphics_get_state(graphics);
	mtx = hg_matrix_new(graphics->pool, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
	memcpy(&gstate->ctm, mtx, sizeof (HgMatrix));
	memcpy(&gstate->snapshot_matrix, mtx, sizeof (HgMatrix));
	hg_mem_free(mtx);
	gstate->x = 0.0;
	gstate->y = 0.0;
	hg_path_clear(gstate->path, TRUE);
	hg_graphics_set_graphic_size(graphics, graphics->current_page->width,
				     graphics->current_page->height);
	/* FIXME: color space */
	hg_graphic_state_color_set_rgb(gstate, 0.0, 0.0, 0.0);
	gstate->line_width = 1.0;
	gstate->line_cap = 0;
	gstate->line_join = 0;
	gstate->miter_limit = 10.0;
	/* FIXME: dashline */

	return TRUE;
}

gboolean
hg_graphics_initclip(HgGraphics *graphics)
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
hg_graphics_set_page_size(HgGraphics *graphics,
			  HgPageSize  size)
{
	gdouble width, height;

	g_return_val_if_fail (graphics != NULL, FALSE);

	hg_page_get_size(size, &width, &height);

	return hg_graphics_set_graphic_size(graphics, width, height);
}

gboolean
hg_graphics_set_graphic_size(HgGraphics *graphics,
			     gdouble     width,
			     gdouble     height)
{
	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	if (graphics->current_page->node) {
		hg_log_warning("Can't change the page size after the rendering code is invoked.");
		return FALSE;
	}

	return hg_graphics_initclip(graphics);
}

gboolean
hg_graphics_save(HgGraphics *graphics)
{
	gpointer data;

	g_return_val_if_fail (graphics != NULL, FALSE);

	data = hg_object_copy((HgObject *)graphics->current_gstate);
	graphics->gstate_stack = g_list_append(graphics->gstate_stack, data);

	return TRUE;
}

gboolean
hg_graphics_restore(HgGraphics *graphics)
{
	GList *l;
	HgGraphicState *gstate;

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
hg_graphics_show_page(HgGraphics *graphics)
{
	GList *l;

	g_return_val_if_fail (graphics != NULL, FALSE);

	l = g_list_last(graphics->pages);
	if (l != NULL && l->data != graphics->current_page)
		graphics->pages = g_list_append(graphics->pages, graphics->current_page);
	graphics->current_page = hg_page_new_with_size(graphics->current_page->width,
						       graphics->current_page->height);
	/* FIXME */
	return hg_graphics_init(graphics);
}

/* matrix */
gboolean
hg_graphics_matrix_rotate(HgGraphics *graphics,
			  gdouble     angle)
{
	HgGraphicState *gstate;
	HgMatrix *mtx, *new_ctm;

	g_return_val_if_fail (graphics != NULL, FALSE);

	mtx = hg_matrix_rotate(graphics->pool, angle);
	new_ctm = hg_matrix_multiply(graphics->pool, mtx, &graphics->current_gstate->ctm);
	memcpy(&graphics->current_gstate->ctm, new_ctm, sizeof (HgMatrix));
	hg_mem_free(mtx);
	hg_mem_free(new_ctm);

	gstate = hg_graphics_get_state(graphics);

	return hg_path_matrix(gstate->path,
			      gstate->ctm.xx, gstate->ctm.yx,
			      gstate->ctm.xy, gstate->ctm.yy,
			      gstate->ctm.x0, gstate->ctm.y0);
}

gboolean
hg_graphics_matrix_scale(HgGraphics *graphics,
			 gdouble     x,
			 gdouble     y)
{
	HgGraphicState *gstate;
	HgMatrix *mtx, *new_ctm;

	g_return_val_if_fail (graphics != NULL, FALSE);

	mtx = hg_matrix_scale(graphics->pool, x, y);
	new_ctm = hg_matrix_multiply(graphics->pool, mtx, &graphics->current_gstate->ctm);
	memcpy(&graphics->current_gstate->ctm, new_ctm, sizeof (HgMatrix));
	hg_mem_free(mtx);
	hg_mem_free(new_ctm);

	gstate = hg_graphics_get_state(graphics);

	return hg_path_matrix(gstate->path,
			      gstate->ctm.xx, gstate->ctm.yx,
			      gstate->ctm.xy, gstate->ctm.yy,
			      gstate->ctm.x0, gstate->ctm.y0);
}

gboolean
hg_graphics_matrix_translate(HgGraphics *graphics,
			     gdouble     x,
			     gdouble     y)
{
	HgGraphicState *gstate;
	HgMatrix *mtx, *new_ctm;

	g_return_val_if_fail (graphics != NULL, FALSE);

	mtx = hg_matrix_translate(graphics->pool, x, y);
	new_ctm = hg_matrix_multiply(graphics->pool, mtx, &graphics->current_gstate->ctm);
	memcpy(&graphics->current_gstate->ctm, new_ctm, sizeof (HgMatrix));
	hg_mem_free(mtx);
	hg_mem_free(new_ctm);

	gstate = hg_graphics_get_state(graphics);

	return hg_path_matrix(gstate->path,
			      gstate->ctm.xx, gstate->ctm.yx,
			      gstate->ctm.xy, gstate->ctm.yy,
			      gstate->ctm.x0, gstate->ctm.y0);
}

/* path */
gboolean
hg_graphic_state_path_new(HgGraphicState *gstate)
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
hg_graphic_state_path_close(HgGraphicState *gstate)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_close(gstate->path);
}

gboolean
hg_graphic_state_path_from_clip(HgGraphicState *gstate)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_copy(gstate->clip_path, gstate->path);
}

gboolean
hg_graphic_state_get_bbox_from_path(HgGraphicState *gstate,
				    gboolean        ignore_moveto,
				    HgPathBBox     *bbox)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_get_bbox(gstate->path, ignore_moveto, bbox);
}

gboolean
hg_graphic_state_path_moveto(HgGraphicState *gstate,
			     gdouble         x,
			     gdouble         y)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_moveto(gstate->path, x, y);
}

gboolean
hg_graphic_state_path_lineto(HgGraphicState *gstate,
			     gdouble         x,
			     gdouble         y)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_lineto(gstate->path, x, y);
}

gboolean
hg_graphic_state_path_rlineto(HgGraphicState *gstate,
			      gdouble         x,
			      gdouble         y)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_rlineto(gstate->path, x, y);
}

gboolean
hg_graphic_state_path_curveto(HgGraphicState *gstate,
			      gdouble         x1,
			      gdouble         y1,
			      gdouble         x2,
			      gdouble         y2,
			      gdouble         x3,
			      gdouble         y3)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_curveto(gstate->path, x1, y1, x2, y2, x3, y3);
}

gboolean
hg_graphic_state_path_arc(HgGraphicState *gstate,
			  gdouble         x,
			  gdouble         y,
			  gdouble         r,
			  gdouble         angle1,
			  gdouble         angle2)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	return hg_path_arc(gstate->path, x, y, r, angle1, angle2);
}

/* color */
gboolean
hg_graphic_state_color_set_rgb(HgGraphicState *gstate,
			       gdouble         red,
			       gdouble         green,
			       gdouble         blue)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	gstate->color.is_rgb = TRUE;
	gstate->color.is.rgb.r = red;
	gstate->color.is.rgb.g = green;
	gstate->color.is.rgb.b = blue;

	return TRUE;
}

gboolean
hg_graphic_state_color_set_hsv(HgGraphicState *gstate,
			       gdouble         hue,
			       gdouble         saturation,
			       gdouble         value)
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
hg_graphics_render_eofill(HgGraphics *graphics)
{
	HgRender *render;
	HgGraphicState *gstate;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	gstate = hg_graphics_get_state(graphics);
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
hg_graphics_render_fill(HgGraphics *graphics)
{
	HgGraphicState *gstate;
	HgRender *render;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	gstate = hg_graphics_get_state(graphics);
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
hg_graphics_render_stroke(HgGraphics *graphics)
{
	HgGraphicState *gstate;
	HgRender *render;

	g_return_val_if_fail (graphics != NULL, FALSE);
	g_return_val_if_fail (graphics->current_gstate != NULL, FALSE);

	gstate = hg_graphics_get_state(graphics);
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
hg_graphic_state_set_linewidth(HgGraphicState *gstate,
			       gdouble         width)
{
	g_return_val_if_fail (gstate != NULL, FALSE);

	gstate->line_width = width;

	return TRUE;
}

/* debugging support */
gboolean
hg_graphics_debug(HgGraphics *graphics,
		  HgDebugFunc func,
		  gpointer    data)
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
