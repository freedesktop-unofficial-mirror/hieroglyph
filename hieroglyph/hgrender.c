/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgrender.c
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
#include "hgrender.h"
#include "hgmem.h"


static void _hg_render_fill_real_set_flags  (gpointer           data,
					     guint              flags);
static void _hg_render_fill_real_relocate   (gpointer           data,
					     HgMemRelocateInfo *info);
static void _hg_render_stroke_real_set_flags(gpointer           data,
					     guint              flags);
static void _hg_render_stroke_real_relocate (gpointer           data,
					     HgMemRelocateInfo *info);
static void _hg_render_debug_real_set_flags (gpointer           data,
					     guint              flags);
static void _hg_render_debug_real_relocate  (gpointer           data,
					     HgMemRelocateInfo *info);


static HgObjectVTable __hg_render_fill_vtable = {
	.free      = NULL,
	.set_flags = _hg_render_fill_real_set_flags,
	.relocate  = _hg_render_fill_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};
static HgObjectVTable __hg_render_stroke_vtable = {
	.free      = NULL,
	.set_flags = _hg_render_stroke_real_set_flags,
	.relocate  = _hg_render_stroke_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};
static HgObjectVTable __hg_render_debug_vtable = {
	.free      = NULL,
	.set_flags = _hg_render_debug_real_set_flags,
	.relocate  = _hg_render_debug_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_hg_render_fill_real_set_flags(gpointer data,
			       guint    flags)
{
	HgRender *render = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(render->u.fill.path, obj);
	if (obj == NULL) {
		g_warning("[BUG] Invalid object %p to be marked: HgRenderFill.", render->u.fill.path);
	} else {
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
}

static void
_hg_render_fill_real_relocate(gpointer           data,
			      HgMemRelocateInfo *info)
{
	HgRender *render = data;

	if ((gsize)render->u.fill.path >= info->start &&
	    (gsize)render->u.fill.path <= info->end) {
		render->u.fill.path = (gpointer)((gsize)render->u.fill.path + info->diff);
	}
}

static void
_hg_render_stroke_real_set_flags(gpointer data,
				 guint    flags)
{
	HgRender *render = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(render->u.stroke.path, obj);
	if (obj == NULL) {
		g_warning("[BUG] Invalid object %p to be marked: HgRenderStroke->path.", render->u.stroke.path);
	} else {
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
	hg_mem_get_object__inline(render->u.stroke.dashline_pattern, obj);
	if (obj == NULL) {
		g_warning("[BUG] Invalid object %p to be marked: HgRenderStroke->dashline_pattern.", render->u.stroke.dashline_pattern);
	} else {
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
}

static void
_hg_render_stroke_real_relocate(gpointer           data,
				HgMemRelocateInfo *info)
{
	HgRender *render = data;

	if ((gsize)render->u.stroke.path >= info->start &&
	    (gsize)render->u.stroke.path <= info->end) {
		render->u.stroke.path = (gpointer)((gsize)render->u.stroke.path + info->diff);
	}
	if ((gsize)render->u.stroke.dashline_pattern >= info->start &&
	    (gsize)render->u.stroke.dashline_pattern <= info->end) {
		render->u.stroke.dashline_pattern = (gpointer)((gsize)render->u.stroke.dashline_pattern + info->diff);
	}
}

static void
_hg_render_debug_real_set_flags(gpointer data,
				guint    flags)
{
	HgRender *render = data;
	HgMemObject *obj;

	if (render->u.debug.data) {
		hg_mem_get_object__inline(render->u.debug.data, obj);
		if (obj != NULL) {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_render_debug_real_relocate(gpointer           data,
			       HgMemRelocateInfo *info)
{
	HgRender *render = data;

	if (render->u.debug.data) {
		if ((gsize)render->u.debug.data >= info->start &&
		    (gsize)render->u.debug.data <= info->end) {
			render->u.debug.data = (gpointer)((gsize)render->u.debug.data + info->diff);
		}
	}
}

/*
 * Public Functions
 */
HgRender *
hg_render_eofill_new(HgMemPool     *pool,
		     HgMatrix      *ctm,
		     HgPathNode    *path,
		     const HgColor *color)
{
	HgRender *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (color != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (HgRender));
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__hg_render_fill_vtable;

	retval->u.type = HG_RENDER_EOFILL;
	memcpy(&retval->u.fill.mtx, ctm, sizeof (HgMatrix));
	retval->u.fill.path = path;
	memcpy(&retval->u.fill.color, color, sizeof (HgColor));

	return retval;
}

HgRender *
hg_render_fill_new(HgMemPool     *pool,
		   HgMatrix      *ctm,
		   HgPathNode    *path,
		   const HgColor *color)
{
	HgRender *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (color != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (HgRender));
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__hg_render_fill_vtable;

	retval->u.type = HG_RENDER_FILL;
	memcpy(&retval->u.fill.mtx, ctm, sizeof (HgMatrix));
	retval->u.fill.path = path;
	memcpy(&retval->u.fill.color, color, sizeof (HgColor));

	return retval;
}

HgRender *
hg_render_stroke_new(HgMemPool     *pool,
		     HgMatrix      *ctm,
		     HgPathNode    *path,
		     const HgColor *color,
		     gdouble        line_width,
		     gint           line_cap,
		     gint           line_join,
		     gdouble        miter_limit,
		     gdouble        dashline_offset,
		     HgArray       *dashline_pattern)
{
	HgRender *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (color != NULL, NULL);
	g_return_val_if_fail (dashline_pattern != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (HgRender));
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__hg_render_stroke_vtable;

	retval->u.type = HG_RENDER_STROKE;
	memcpy(&retval->u.stroke.mtx, ctm, sizeof (HgMatrix));
	retval->u.stroke.path = path;
	memcpy(&retval->u.stroke.color, color, sizeof (HgColor));
	retval->u.stroke.line_width = line_width;
	retval->u.stroke.line_cap = line_cap;
	retval->u.stroke.line_join = line_join;
	retval->u.stroke.miter_limit = miter_limit;
	retval->u.stroke.dashline_offset = dashline_offset;
	retval->u.stroke.dashline_pattern = hg_object_copy((HgObject *)dashline_pattern);

	return retval;
}

/* debugging renderer */
HgRender *
hg_render_debug_new(HgMemPool   *pool,
		    HgDebugFunc  func,
		    gpointer     data)
{
	HgRender *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (func != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (HgRender));
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__hg_render_debug_vtable;

	retval->u.type = HG_RENDER_DEBUG;
	retval->u.debug.func = func;
	retval->u.debug.data = data;

	return retval;
}
