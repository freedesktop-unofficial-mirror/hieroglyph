/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice-cairo.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgvaluenode.h>
#include "hgdevice-cairo.h"


static gboolean _hg_cairo_device_real_initialize(HgDevice       *device,
						 HgPage         *page);
static gboolean _hg_cairo_device_real_finalize  (HgDevice       *device);
static gboolean _hg_cairo_device_real_eofill    (HgDevice       *device,
						 HgRenderFill   *render);
static gboolean _hg_cairo_device_real_fill      (HgDevice       *device,
						 HgRenderFill   *render);
static gboolean _hg_cairo_device_real_stroke    (HgDevice       *device,
						 HgRenderStroke *render);

static void     _hg_cairo_device_set_matrix     (HgCairoDevice  *device,
						 HgMatrix       *mtx);
static gboolean _hg_cairo_device_set_path       (HgCairoDevice  *device,
						 HgPathNode     *node);
static void     _hg_cairo_device_set_line_cap   (HgCairoDevice  *device,
						 gint            line_cap);
static void     _hg_cairo_device_set_line_join  (HgCairoDevice  *device,
						 gint            line_join);


static HgDeviceVTable __hg_cairo_device_vtable = {
	.initialize = _hg_cairo_device_real_initialize,
	.finalize   = _hg_cairo_device_real_finalize,
	.eofill     = _hg_cairo_device_real_eofill,
	.fill       = _hg_cairo_device_real_fill,
	.stroke     = _hg_cairo_device_real_stroke,
};

/*
 * Private Functions
 */
#ifdef DEBUG_PATH
static void
_hg_cairo_device_print_path(HgPathNode    *node)
{
	g_print("\n");
	while (node) {
		switch (node->type) {
		    case HG_PATH_CLOSE:
			    g_print("closepath\n");
			    break;
		    case HG_PATH_MOVETO:
			    g_print("%f %f moveto\n", node->x, node->y);
			    break;
		    case HG_PATH_LINETO:
			    g_print("%f %f lineto\n", node->x, node->y);
			    break;
		    case HG_PATH_RLINETO:
			    g_print("%f %f rlineto\n", node->x, node->y);
			    break;
		    case HG_PATH_CURVETO:
			    if (node->next && node->next->next) {
				    g_print("%f %f %f %f %f %f curveto\n", node->x, node->y, node->next->x, node->next->y, node->next->next->x, node->next->next->y);
				    node = node->next->next;
			    } else {
				    g_warning("[BUG] Invalid path for curve.");
			    }
			    break;
		    case HG_PATH_ARC:
			    if (node->next && node->next->next) {
				    g_print("%f %f %f %f %f arc\n", node->x, node->y, node->next->x, node->next->next->x, node->next->next->y);
				    node = node->next->next;
			    } else {
				    g_warning("[BUG] Invalid path for arc.");
			    }
			    break;
		    case HG_PATH_MATRIX:
			    if (node->next && node->next->next) {
				    g_print("[%f %f %f %f %f %f] matrix\n", node->x, node->y, node->next->x, node->next->y, node->next->next->x, node->next->next->y);
				    node = node->next->next;
			    } else {
				    g_warning("[BUG] Invalid matrix was given.");
			    }
			    break;
		    default:
			    g_warning("[BUG] Unknown path type %d was given.", node->type);
			    break;
		}
		node = node->next;
	}
	g_print("%% end\n\n");
}
#endif /* DEBUG_PATH */

/*
 * hsv_to_rgb() is borrowed from GTK+
 */
/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
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
/* Converts from HSV to RGB */
static void
hsv_to_rgb (gdouble *h,
	    gdouble *s,
	    gdouble *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;
  
  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
      *v = *v; /* heh */
    }
  else
    {
      hue = *h * 6.0;
      saturation = *s;
      value = *v;
      
      if (hue == 6.0)
	hue = 0.0;
      
      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));
      
      switch ((int) hue)
	{
	case 0:
	  *h = value;
	  *s = t;
	  *v = p;
	  break;
	  
	case 1:
	  *h = q;
	  *s = value;
	  *v = p;
	  break;
	  
	case 2:
	  *h = p;
	  *s = value;
	  *v = t;
	  break;
	  
	case 3:
	  *h = p;
	  *s = q;
	  *v = value;
	  break;
	  
	case 4:
	  *h = t;
	  *s = p;
	  *v = value;
	  break;
	  
	case 5:
	  *h = value;
	  *s = p;
	  *v = q;
	  break;
	  
	default:
	  g_assert_not_reached ();
	}
    }
}

static gboolean
_hg_cairo_device_real_initialize(HgDevice *device,
				 HgPage   *page)
{
	HgCairoDevice *cdev = (HgCairoDevice *)device;
	cairo_matrix_t mtx;

	device->width = page->width;
	device->height = page->height;
	/* initialize the device specific things */
	if (cdev->vtable.set_page_size) {
		if (!cdev->vtable.set_page_size(cdev, page->width, page->height))
			return FALSE;
	}

	/* it needs to be transformed so that cairo's coordination
	 * system is reversed for PostScript
	 */
	cairo_matrix_init(&mtx, 1.0, 0.0, 0.0, -1.0, 0.0, page->height);
	cairo_set_matrix(cdev->reference, &mtx);

	/* FIXME */

	return TRUE;
}

static gboolean
_hg_cairo_device_real_finalize(HgDevice *device)
{
	HgCairoDevice *cdev = (HgCairoDevice *)device;

	g_warning("%s: FIXME: implement me.", __FUNCTION__);
	cairo_show_page(cdev->reference);
	XFlush(cdev->u.xlib.dpy);

	return TRUE;
}

static gboolean
_hg_cairo_device_real_eofill(HgDevice     *device,
			     HgRenderFill *render)
{
	HgCairoDevice *cdev = (HgCairoDevice *)device;

#ifdef DEBUG_PATH
	g_print("eofill\n");
	_hg_cairo_device_print_path(render->path);
#endif /* DEBUG_PATH */
	_hg_cairo_device_set_matrix(cdev, &render->mtx);
	if (!_hg_cairo_device_set_path(cdev, render->path))
		return FALSE;
	if (render->color.is_rgb) {
		cairo_set_source_rgb(cdev->reference,
				     render->color.is.rgb.r,
				     render->color.is.rgb.g,
				     render->color.is.rgb.b);
	} else {
		gdouble r, g, b;

		r = render->color.is.hsv.h;
		g = render->color.is.hsv.s;
		b = render->color.is.hsv.v;
		hsv_to_rgb(&r, &g, &b);
		cairo_set_source_rgb(cdev->reference, r, g, b);
	}
	cairo_set_fill_rule(cdev->reference, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_fill(cdev->reference);

	return TRUE;
}

static gboolean
_hg_cairo_device_real_fill(HgDevice     *device,
			   HgRenderFill *render)
{
	HgCairoDevice *cdev = (HgCairoDevice *)device;

#ifdef DEBUG_PATH
	g_print("fill\n");
	_hg_cairo_device_print_path(render->path);
#endif /* DEBUG_PATH */
	_hg_cairo_device_set_matrix(cdev, &render->mtx);
	if (!_hg_cairo_device_set_path(cdev, render->path))
		return FALSE;
	if (render->color.is_rgb) {
		cairo_set_source_rgb(cdev->reference,
				     render->color.is.rgb.r,
				     render->color.is.rgb.g,
				     render->color.is.rgb.b);
	} else {
		gdouble r, g, b;

		r = render->color.is.hsv.h;
		g = render->color.is.hsv.s;
		b = render->color.is.hsv.v;
		hsv_to_rgb(&r, &g, &b);
		cairo_set_source_rgb(cdev->reference, r, g, b);
	}
	cairo_set_fill_rule(cdev->reference, CAIRO_FILL_RULE_WINDING);
	cairo_fill(cdev->reference);

	return TRUE;
}

static gboolean
_hg_cairo_device_real_stroke(HgDevice       *device,
			     HgRenderStroke *render)
{
	HgCairoDevice *cdev = (HgCairoDevice *)device;
	gdouble *dashes, d;
	guint len, i;
	HgValueNode *node;

#ifdef DEBUG_PATH
	g_print("stroke\n");
	_hg_cairo_device_print_path(render->path);
#endif /* DEBUG_PATH */
	_hg_cairo_device_set_matrix(cdev, &render->mtx);
	if (!_hg_cairo_device_set_path(cdev, render->path))
		return FALSE;
	if (render->color.is_rgb) {
		cairo_set_source_rgb(cdev->reference,
				     render->color.is.rgb.r,
				     render->color.is.rgb.g,
				     render->color.is.rgb.b);
	} else {
		gdouble r, g, b;

		r = render->color.is.hsv.h;
		g = render->color.is.hsv.s;
		b = render->color.is.hsv.v;
		hsv_to_rgb(&r, &g, &b);
		cairo_set_source_rgb(cdev->reference, r, g, b);
	}
	cairo_set_line_width(cdev->reference, render->line_width);
	_hg_cairo_device_set_line_cap(cdev, render->line_cap);
	cairo_set_miter_limit(cdev->reference, render->miter_limit);
	_hg_cairo_device_set_line_join(cdev, render->line_join);

	len = hg_array_length(render->dashline_pattern);
	dashes = g_new(gdouble, len);
	for (i = 0; i < len; i++) {
		node = hg_array_index(render->dashline_pattern, i);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			g_warning("[BUG] Invalid object type %d was given for dashline pattern.", node->type);
			d = 0.0;
		}
		dashes[i] = d;
	}
	cairo_set_dash(cdev->reference, dashes, len, render->dashline_offset);
	cairo_stroke(cdev->reference);
	g_free(dashes);

	return TRUE;
}

static void
_hg_cairo_device_set_matrix(HgCairoDevice *device,
			    HgMatrix      *mtx)
{
	cairo_matrix_t mtx_, trans;

	cairo_matrix_init(&mtx_, mtx->xx, mtx->yx,
			  mtx->xy, mtx->yy,
			  mtx->x0, mtx->y0);
	cairo_matrix_init(&trans, 1.0, 0.0, 0.0,
			  -1.0, 0.0, device->device.height);
	cairo_matrix_multiply(&trans, &mtx_, &trans);
#ifdef DEBUG_PATH
	g_print("[%f %f %f %f %f %f] setmatrix\n", trans.xx, trans.yx, trans.xy, trans.yy, trans.x0, trans.y0);
#endif /* DEBUG_PATH */
	cairo_set_matrix(device->reference, &trans);
}

static gboolean
_hg_cairo_device_set_path(HgCairoDevice *device,
			  HgPathNode    *node)
{
	gboolean retval = TRUE;

	cairo_new_path(device->reference);
	while (node) {
		switch (node->type) {
		    case HG_PATH_CLOSE:
			    cairo_close_path(device->reference);
			    break;
		    case HG_PATH_MOVETO:
			    cairo_move_to(device->reference, node->x, node->y);
			    break;
		    case HG_PATH_LINETO:
			    cairo_line_to(device->reference, node->x, node->y);
			    break;
		    case HG_PATH_RLINETO:
			    cairo_rel_line_to(device->reference, node->x, node->y);
			    break;
		    case HG_PATH_CURVETO:
			    if (node->next && node->next->next) {
				    cairo_curve_to(device->reference,
						   node->x, node->y,
						   node->next->x, node->next->y,
						   node->next->next->x, node->next->next->y);
				    node = node->next->next;
			    } else {
				    g_warning("[BUG] Invalid path for curve.");
				    retval = FALSE;
			    }
			    break;
		    case HG_PATH_ARC:
			    if (node->next && node->next->next) {
				    cairo_arc(device->reference,
					      node->x, node->y, node->next->x,
					      node->next->next->x, node->next->next->y);
				    node = node->next->next;
			    } else {
				    g_warning("[BUG] Invalid path for arc.");
				    retval = FALSE;
			    }
			    break;
		    case HG_PATH_MATRIX:
			    if (node->next && node->next->next) {
				    cairo_matrix_t mtx_, trans;

				    cairo_matrix_init(&mtx_, node->x, node->y,
						      node->next->x, node->next->y,
						      node->next->next->x, node->next->next->y);
				    cairo_matrix_init(&trans, 1.0, 0.0, 0.0,
						      -1.0, 0.0, device->device.height);
				    cairo_matrix_multiply(&trans, &mtx_, &trans);
				    cairo_set_matrix(device->reference, &trans);
				    node = node->next->next;
			    } else {
				    g_warning("[BUG] Invalid matrix was given.");
				    retval = FALSE;
			    }
			    break;
		    default:
			    g_warning("[BUG] Unknown path type %d was given.", node->type);
			    retval = FALSE;
			    break;
		}
		node = node->next;
	}

	return retval;
}

static void
_hg_cairo_device_set_line_cap(HgCairoDevice *device,
			      gint           line_cap)
{
	cairo_line_cap_t cap;

	switch (line_cap) {
	    case 0:
		    cap = CAIRO_LINE_CAP_BUTT;
		    break;
	    case 1:
		    cap = CAIRO_LINE_CAP_ROUND;
		    break;
	    case 2:
		    cap = CAIRO_LINE_CAP_SQUARE;
		    break;
	    default:
		    g_warning("[BUG] Invalid linecap type %d", line_cap);
		    cap = CAIRO_LINE_CAP_BUTT;
		    break;
	}
	cairo_set_line_cap(device->reference, cap);
}

static void
_hg_cairo_device_set_line_join(HgCairoDevice *device,
			       gint           line_join)
{
	cairo_line_join_t join;

	switch (line_join) {
	    case 0:
		    join = CAIRO_LINE_JOIN_MITER;
		    break;
	    case 1:
		    join = CAIRO_LINE_JOIN_ROUND;
		    break;
	    case 2:
		    join = CAIRO_LINE_JOIN_BEVEL;
		    break;
	    default:
		    g_warning("[BUG] Invalid linejoin type %d", line_join);
		    join = CAIRO_LINE_JOIN_MITER;
		    break;
	}
	cairo_set_line_join(device->reference, join);
}

/*
 * Public Functions
 */
HgDevice *
hg_cairo_device_new(void)
{
	HgCairoDevice *retval;

	retval = g_new0(HgCairoDevice, 1);
	retval->device.vtable = &__hg_cairo_device_vtable;

	return (HgDevice *)retval;
}
