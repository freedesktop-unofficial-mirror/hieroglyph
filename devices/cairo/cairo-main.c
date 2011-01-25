/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * cairo-main.c
 * Copyright (C) 2005-2011 Akira TAGOH
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
/* GLib is still needed for the mutex lock */
#include <glib.h>
#include "hgarray.h"
#include "hgerror.h"
#include "hggstate.h"
#include "hgint.h"
#include "hgmem.h"
#include "hgpath.h"
#include "hgreal.h"
#include "hgvm.h"
#include "hgdevice.h"

typedef enum _hg_cairo_device_surface_t {
	HG_CAIRO_DEVICE_SURFACE_XLIB,
	HG_CAIRO_DEVICE_SURFACE_END
} hg_cairo_device_surface_t;
typedef struct _hg_cairo_xlib_device_t {
	hg_cairo_device_surface_t  type;
	Display                   *dpy;
	Drawable                   drawable;
	Pixmap                     pixmap;
	Region                     region;
} hg_cairo_xlib_device_t;
typedef struct _hg_cairo_device_t {
	hg_device_t      parent;
	cairo_t         *cr;
	cairo_surface_t *surface;
	union {
		hg_cairo_device_surface_t type;
		hg_cairo_xlib_device_t    xlib;
	} u;
} hg_cairo_device_t;

static void _hg_cairo_device_new_path  (gpointer  user_data);
static void _hg_cairo_device_close_path(gpointer  user_data);
static void _hg_cairo_device_moveto    (gpointer  user_data,
                                        gdouble   x,
                                        gdouble   y);
static void _hg_cairo_device_rmoveto   (gpointer  user_data,
                                        gdouble   rx,
                                        gdouble   ry);
static void _hg_cairo_device_lineto    (gpointer  user_data,
                                        gdouble   x,
                                        gdouble   y);
static void _hg_cairo_device_rlineto   (gpointer  user_data,
                                        gdouble   rx,
                                        gdouble   ry);
static void _hg_cairo_device_curveto   (gpointer  user_data,
                                        gdouble   x1,
                                        gdouble   y1,
                                        gdouble   x2,
                                        gdouble   y2,
                                        gdouble   x3,
                                        gdouble   y3);
static void _hg_cairo_device_rcurveto  (gpointer  user_data,
                                        gdouble   rx1,
                                        gdouble   ry1,
                                        gdouble   rx2,
                                        gdouble   ry2,
                                        gdouble   rx3,
                                        gdouble   ry3);


static hg_path_operate_vtable_t __vtable = {
	_hg_cairo_device_new_path,
	_hg_cairo_device_close_path,
	_hg_cairo_device_moveto,
	_hg_cairo_device_rmoveto,
	_hg_cairo_device_lineto,
	_hg_cairo_device_rlineto,
	_hg_cairo_device_curveto,
	_hg_cairo_device_rcurveto
};

G_LOCK_DEFINE_STATIC (cairo);

/*< private >*/
static void
_hg_cairo_device_new_path(gpointer user_data)
{
	cairo_new_path((cairo_t *)user_data);
}

static void
_hg_cairo_device_close_path(gpointer user_data)
{
	cairo_close_path((cairo_t *)user_data);
}

static void
_hg_cairo_device_moveto(gpointer user_data,
			gdouble  x,
			gdouble  y)
{
	cairo_move_to((cairo_t *)user_data, x, y);
}

static void
_hg_cairo_device_rmoveto(gpointer user_data,
			 gdouble  rx,
			 gdouble  ry)
{
	cairo_rel_move_to((cairo_t *)user_data, rx, ry);
}

static void
_hg_cairo_device_lineto(gpointer user_data,
			gdouble  x,
			gdouble  y)
{
	cairo_line_to((cairo_t *)user_data, x, y);
}

static void
_hg_cairo_device_rlineto(gpointer user_data,
			 gdouble  rx,
			 gdouble  ry)
{
	cairo_rel_line_to((cairo_t *)user_data, rx, ry);
}

static void
_hg_cairo_device_curveto(gpointer user_data,
			 gdouble  x1,
			 gdouble  y1,
			 gdouble  x2,
			 gdouble  y2,
			 gdouble  x3,
			 gdouble  y3)
{
	cairo_curve_to((cairo_t *)user_data, x1, y1, x2, y2, x3, y3);
}

static void
_hg_cairo_device_rcurveto(gpointer user_data,
			  gdouble  rx1,
			  gdouble  ry1,
			  gdouble  rx2,
			  gdouble  ry2,
			  gdouble  rx3,
			  gdouble  ry3)
{
	cairo_rel_curve_to((cairo_t *)user_data, rx1, ry1, rx2, ry2, rx3, ry3);
}

static void
_hg_cairo_device_set_ctm(hg_cairo_device_t *device,
			 hg_matrix_t       *matrix)
{
	cairo_matrix_t mtx_, trans;

	matrix->mtx.xx = 1.0;
	matrix->mtx.yy = 1.0;
	cairo_matrix_init(&mtx_, matrix->mtx.xx, matrix->mtx.yx,
			  matrix->mtx.xy, matrix->mtx.yy,
			  matrix->mtx.x0, matrix->mtx.y0);
	cairo_matrix_init(&trans, 1.0, 0.0,
			  0.0, -1.0,
			  0.0, device->parent.params->page_size.height);
	cairo_matrix_multiply(&trans, &mtx_, &trans);
	cairo_set_matrix(device->cr, &trans);
}

static void
_hg_cairo_device_set_color(hg_cairo_device_t *device,
			   hg_color_t        *color)
{
	if (color->type == HG_COLOR_RGB ||
	    color->type == HG_COLOR_GRAY) {
		cairo_set_source_rgb(device->cr,
				     color->is.rgb.red,
				     color->is.rgb.green,
				     color->is.rgb.blue);
	} else if (color->type == HG_COLOR_HSB) {
		gdouble r, g, b, h, s, v, f, p, q, t;

		if (color->is.hsb.saturation == 0.0) {
			r = color->is.hsb.brightness;
			g = color->is.hsb.brightness;
			b = color->is.hsb.brightness;
		} else {
			h = color->is.hsb.hue * 6.0;
			s = color->is.hsb.saturation;
			v = color->is.hsb.brightness;

			if (HG_REAL_EQUAL (h, 6.0))
				h = 0.0;

			f = h - (int) h;
			p = v * (1.0 - s);
			q = v * (1.0 - s * f);
			t = v * (1.0 - s * (1.0 - f));

			switch ((int)h) {
			    case 0:
				    r = v;
				    g = t;
				    b = p;
				    break;
			    case 1:
				    r = q;
				    g = v;
				    b = p;
				    break;
			    case 2:
				    r = p;
				    g = v;
				    b = t;
				    break;
			    case 3:
				    r = p;
				    g = q;
				    b = v;
				    break;
			    case 4:
				    r = t;
				    g = p;
				    b = v;
				    break;
			    case 5:
				    r = v;
				    g = p;
				    b = q;
				    break;
			    default:
				    g_assert_not_reached();
			}
		}
		cairo_set_source_rgb(device->cr, r, g, b);
	}
}

static void
_hg_cairo_device_set_line_cap(hg_cairo_device_t *device,
			      hg_linecap_t       linecap)
{
	cairo_line_cap_t cap;

	switch (linecap) {
	    case HG_LINECAP_BUTT:
		    cap = CAIRO_LINE_CAP_BUTT;
		    break;
	    case HG_LINECAP_ROUND:
		    cap = CAIRO_LINE_CAP_ROUND;
		    break;
	    case HG_LINECAP_SQUARE:
		    cap = CAIRO_LINE_CAP_SQUARE;
		    break;
	    default:
		    g_warning("unknown line cap");
		    cap = CAIRO_LINE_CAP_BUTT;
		    break;
	}
	cairo_set_line_cap(device->cr, cap);
}

static void
_hg_cairo_device_set_line_join(hg_cairo_device_t *device,
			       hg_linejoin_t      linejoin)
{
	cairo_line_join_t join;

	switch (linejoin) {
	    case HG_LINEJOIN_MITER:
		    join = CAIRO_LINE_JOIN_MITER;
		    break;
	    case HG_LINEJOIN_ROUND:
		    join = CAIRO_LINE_JOIN_ROUND;
		    break;
	    case HG_LINEJOIN_BEVEL:
		    join = CAIRO_LINE_JOIN_BEVEL;
		    break;
	    default:
		    g_warning("unknown line join");
		    join = CAIRO_LINE_JOIN_MITER;
		    break;
	}
	cairo_set_line_join(device->cr, join);
}

static void
_hg_cairo_device_set_dash(hg_cairo_device_t *device,
			  hg_quark_t         qdash,
			  gdouble            dash_offset)
{
	hg_array_t *a;
	hg_mem_t *m = hg_mem_get(hg_quark_get_mem_id(qdash));
	hg_quark_t q;
	gsize len, i;
	gdouble *dashes;

	hg_return_if_lock_fail (a, m, qdash);

	len = hg_array_length(a);
	dashes = g_new(gdouble, len);

	for (i = 0; i < len; i++) {
		q = hg_array_get(a, i, NULL);
		if (HG_IS_QINT (q)) {
			dashes[i] = HG_INT (q);
		} else {
			dashes[i] = HG_REAL (q);
		}
	}
	cairo_set_dash(device->cr, dashes, len, dash_offset);

	g_free(dashes);
	hg_mem_unlock_object(m, qdash);
}

static void
_hg_cairo_device_install(hg_device_t  *device,
			 hg_vm_t      *vm,
			 GError      **error)
{
	hg_cairo_device_t *cdev = (hg_cairo_device_t *)device;
	gint screen;
	gulong black, white;
	cairo_matrix_t mtx;

	device->params->page_size.width = 600;
	device->params->page_size.height = 600;
	cdev->u.type = HG_CAIRO_DEVICE_SURFACE_XLIB;

	switch (cdev->u.type) {
	    case HG_CAIRO_DEVICE_SURFACE_XLIB:
		    cdev->u.xlib.dpy = XOpenDisplay(NULL);
		    if (cdev->u.xlib.dpy == NULL) {
			    g_set_error(error, HG_ERROR, HG_VM_e_VMerror,
					"Unable to open a display");
			    return;
		    }
		    screen = DefaultScreen(cdev->u.xlib.dpy);
		    black = BlackPixel(cdev->u.xlib.dpy, screen);
		    white = WhitePixel(cdev->u.xlib.dpy, screen);
		    cdev->u.xlib.drawable = XCreateSimpleWindow(cdev->u.xlib.dpy,
								DefaultRootWindow(cdev->u.xlib.dpy),
								0, 0,
								device->params->page_size.width,
								device->params->page_size.height,
								0, black, white);
		    cdev->u.xlib.pixmap = XCreatePixmap(cdev->u.xlib.dpy,
							DefaultRootWindow(cdev->u.xlib.dpy),
							device->params->page_size.width,
							device->params->page_size.height,
							DefaultDepth(cdev->u.xlib.dpy, screen));

		    XSelectInput(cdev->u.xlib.dpy, cdev->u.xlib.drawable, ExposureMask);
		    XMapWindow(cdev->u.xlib.dpy, cdev->u.xlib.drawable);
		    cdev->surface = cairo_xlib_surface_create(cdev->u.xlib.dpy,
							      (Drawable)cdev->u.xlib.pixmap,
							      DefaultVisual (cdev->u.xlib.dpy, screen),
							      device->params->page_size.width,
							      device->params->page_size.height);
		    break;
	    default:
		    g_set_error(error, HG_ERROR, HG_VM_e_VMerror,
				"Invalid surface.");
		    return;
	}
	cdev->cr = cairo_create(cdev->surface);
	if (cairo_status(cdev->cr) != CAIRO_STATUS_SUCCESS) {
		g_set_error(error, HG_ERROR, HG_VM_e_VMerror,
			    "Unable to create a cairo instance");
		return;
	}
	/* The page needs to be transformed.
	 * it's wrong side up comparing with PostScript's.
	 */
	cairo_matrix_init(&mtx, 1.0, 0.0, 0.0, -1.0, 0.0, device->params->page_size.height);
	cairo_set_matrix(cdev->cr, &mtx);
}

static gboolean
_hg_cairo_device_is_pending_draw(hg_device_t *device)
{
	hg_cairo_device_t *cdev = (hg_cairo_device_t *)device;

	switch (cdev->u.type) {
	    case HG_CAIRO_DEVICE_SURFACE_XLIB:
		    G_LOCK (cairo);

		    if (cdev->u.xlib.region) {
			    XRectangle ext;
			    GC gc;

			    XClipBox(cdev->u.xlib.region, &ext);
			    gc = XCreateGC(cdev->u.xlib.dpy,
					   cdev->u.xlib.pixmap,
					   0, NULL);
			    XCopyArea(cdev->u.xlib.dpy, cdev->u.xlib.pixmap,
				      cdev->u.xlib.drawable, gc,
				      ext.x, ext.y,
				      ext.width, ext.height,
				      ext.x, ext.y);
			    XFlush(cdev->u.xlib.dpy);
			    XFreeGC(cdev->u.xlib.dpy, gc);
			    XDestroyRegion(cdev->u.xlib.region);
			    cdev->u.xlib.region = NULL;
		    }

		    G_UNLOCK (cairo);

		    return cdev->u.xlib.dpy && XPending(cdev->u.xlib.dpy);
	    default:
		    return FALSE;
	}
}

static void
_hg_cairo_device_draw(hg_device_t *device)
{
	hg_cairo_device_t *cdev = (hg_cairo_device_t *)device;

	switch (cdev->u.type) {
	    case HG_CAIRO_DEVICE_SURFACE_XLIB:
		    if (XPending(cdev->u.xlib.dpy)) {
			    XEvent xev;
			    XRectangle r;

			    XNextEvent(cdev->u.xlib.dpy, &xev);
			    switch (xev.xany.type) {
				case Expose:
					G_LOCK (cairo);

					if (!cdev->u.xlib.region)
						cdev->u.xlib.region = XCreateRegion();

					r.x = xev.xexpose.x;
					r.y = xev.xexpose.y;
					r.width = xev.xexpose.width;
					r.height = xev.xexpose.height;
					XUnionRectWithRegion(&r,
							     cdev->u.xlib.region,
							     cdev->u.xlib.region);

					G_UNLOCK (cairo);
					break;
				case NoExpose:
					break;
				default:
					break;
			    }
		    }
		    break;
	    default:
		    break;
	}
}

static gboolean
_hg_cairo_device_get_ctm(hg_device_t *device,
			 hg_matrix_t *matrix)
{
	matrix->mtx.xx = 1.0;
	matrix->mtx.yx = 0.0;
	matrix->mtx.xy = 0.0;
	matrix->mtx.yy = 1.0;
	matrix->mtx.x0 = 0.0;
	matrix->mtx.y0 = 0.0;

	return TRUE;
}

static gboolean
_hg_cairo_device_eofill(hg_device_t  *device,
			hg_gstate_t  *gstate)
{
	hg_cairo_device_t *cdev = (hg_cairo_device_t *)device;
	hg_quark_t qpath = hg_gstate_get_path(gstate);
	hg_path_t *path;
	hg_matrix_t mtx;
	XRectangle r;

	hg_return_val_if_lock_fail (path,
				    gstate->o.mem,
				    qpath,
				    FALSE);

	hg_gstate_get_ctm(gstate, &mtx);
	_hg_cairo_device_set_ctm(cdev, &mtx);
	cairo_new_path(cdev->cr);
	if (!hg_path_operate(path, &__vtable, cdev->cr, NULL))
		return FALSE;
	_hg_cairo_device_set_color(cdev, &gstate->color);

	cairo_set_fill_rule(cdev->cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_fill(cdev->cr);

	G_LOCK (cairo);

	if (!cdev->u.xlib.region) {
		cdev->u.xlib.region = XCreateRegion();
		r.x = 0;
		r.y = 0;
		r.width = device->params->page_size.width;
		r.height = device->params->page_size.height;
		XUnionRectWithRegion(&r, cdev->u.xlib.region, cdev->u.xlib.region);
	}

	G_UNLOCK (cairo);

	return TRUE;
}

static gboolean
_hg_cairo_device_fill(hg_device_t  *device,
		      hg_gstate_t  *gstate)
{
	hg_cairo_device_t *cdev = (hg_cairo_device_t *)device;
	hg_quark_t qpath = hg_gstate_get_path(gstate);
	hg_path_t *path;
	hg_matrix_t mtx;
	XRectangle r;
	gboolean retval = TRUE;

	hg_return_val_if_lock_fail (path,
				    gstate->o.mem,
				    qpath,
				    FALSE);

	hg_gstate_get_ctm(gstate, &mtx);
	_hg_cairo_device_set_ctm(cdev, &mtx);
	cairo_new_path(cdev->cr);
	if (!hg_path_operate(path, &__vtable, cdev->cr, NULL)) {
		retval = FALSE;
		goto finalize;
	}
	_hg_cairo_device_set_color(cdev, &gstate->color);

	cairo_set_fill_rule(cdev->cr, CAIRO_FILL_RULE_WINDING);
	cairo_fill(cdev->cr);

	G_LOCK (cairo);

	if (!cdev->u.xlib.region) {
		cdev->u.xlib.region = XCreateRegion();
		r.x = 0;
		r.y = 0;
		r.width = device->params->page_size.width;
		r.height = device->params->page_size.height;
		XUnionRectWithRegion(&r, cdev->u.xlib.region, cdev->u.xlib.region);
	}

	G_UNLOCK (cairo);
  finalize:
	hg_mem_unlock_object(gstate->o.mem, qpath);

	return TRUE;
}

static gboolean
_hg_cairo_device_stroke(hg_device_t  *device,
			hg_gstate_t  *gstate)
{
	hg_cairo_device_t *cdev = (hg_cairo_device_t *)device;
	hg_quark_t qpath = hg_gstate_get_path(gstate);
	hg_path_t *path;
	hg_matrix_t mtx;
	XRectangle r;

	hg_return_val_if_lock_fail (path,
				    gstate->o.mem,
				    qpath,
				    FALSE);

	hg_gstate_get_ctm(gstate, &mtx);
	_hg_cairo_device_set_ctm(cdev, &mtx);
	cairo_new_path(cdev->cr);
	if (!hg_path_operate(path, &__vtable, cdev->cr, NULL))
		return FALSE;
	_hg_cairo_device_set_color(cdev, &gstate->color);
	cairo_set_line_width(cdev->cr, gstate->linewidth);
	_hg_cairo_device_set_line_cap(cdev, gstate->linecap);
	cairo_set_miter_limit(cdev->cr, gstate->miterlen);
	_hg_cairo_device_set_line_join(cdev, gstate->linejoin);
	_hg_cairo_device_set_dash(cdev, gstate->qdashpattern, gstate->dash_offset);

	cairo_stroke(cdev->cr);

	G_LOCK (cairo);

	if (!cdev->u.xlib.region) {
		cdev->u.xlib.region = XCreateRegion();
		r.x = 0;
		r.y = 0;
		r.width = device->params->page_size.width;
		r.height = device->params->page_size.height;
		XUnionRectWithRegion(&r, cdev->u.xlib.region, cdev->u.xlib.region);
	}

	G_UNLOCK (cairo);

	return TRUE;
}

/*< public >*/
hg_device_t *
hg_module_init(void)
{
	hg_cairo_device_t *retval = g_new0(hg_cairo_device_t, 1);
	hg_device_t *dev = (hg_device_t *)retval;

	dev->install         = _hg_cairo_device_install;
	dev->is_pending_draw = _hg_cairo_device_is_pending_draw;
	dev->draw            = _hg_cairo_device_draw;
	dev->get_ctm         = _hg_cairo_device_get_ctm;
	dev->eofill          = _hg_cairo_device_eofill;
	dev->fill            = _hg_cairo_device_fill;
	dev->stroke          = _hg_cairo_device_stroke;

	return dev;
}

void
hg_module_finalize(hg_device_t *device)
{
	g_free(device);
}
