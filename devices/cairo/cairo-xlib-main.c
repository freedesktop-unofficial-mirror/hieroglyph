/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * cairo-xlib-main.c
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

#include <cairo/cairo-xlib.h>
#include "hgdevice-cairo.h"


/*
 * Private Functions
 */
static gboolean
_cairo_xlib_real_set_page_size(HgCairoDevice *device,
			       gdouble        width,
			       gdouble        height)
{
	/* resize Window */
	g_print("%fx%f\n", width, height);
	XResizeWindow(device->u.xlib.dpy,
		      device->u.xlib.drawable,
		      width, height);
	cairo_xlib_surface_set_size(device->surface, width, height);
	XFlush(device->u.xlib.dpy);
	g_warning("%s: FIXME: implement me", __FUNCTION__);

	return TRUE;
}

/*
 * Public Functions
 */
HgDevice *
device_open(void)
{
	HgCairoDevice *cdev;
	HgDevice *dev;
	unsigned long black, white;
	int screen;

	dev = hg_cairo_device_new();
	cdev = (HgCairoDevice *)dev;

	cdev->vtable.set_page_size = _cairo_xlib_real_set_page_size;

	cdev->u.type = HG_DEVICE_CAIRO_XLIB_SURFACE;
	cdev->u.xlib.dpy = XOpenDisplay(NULL);
	if (cdev->u.xlib.dpy == NULL) {
		g_warning("Failed to open a display.");
		hg_device_destroy(dev);
		return NULL;
	}
	screen = DefaultScreen(cdev->u.xlib.dpy);
	black = BlackPixel(cdev->u.xlib.dpy, screen);
	white = WhitePixel(cdev->u.xlib.dpy, screen);
	cdev->u.xlib.drawable = XCreateSimpleWindow(cdev->u.xlib.dpy,
						    DefaultRootWindow(cdev->u.xlib.dpy),
						    400,
						    400,
						    400, 400, 0,
						    black, white);
	XMapWindow(cdev->u.xlib.dpy, cdev->u.xlib.drawable);

	cdev->surface = cairo_xlib_surface_create(cdev->u.xlib.dpy,
						  cdev->u.xlib.drawable,
						  XDefaultVisual(cdev->u.xlib.dpy, screen),
						  400, 400);
	cdev->reference = cairo_create(cdev->surface);
	if (cairo_status(cdev->reference) != CAIRO_STATUS_SUCCESS) {
		g_warning("Failed to create an reference of cairo.");
		hg_device_destroy(dev);
		return NULL;
	}

	return dev;
}

void
device_close(HgDevice *device)
{
	HgCairoDevice *cdev;

	g_return_if_fail (device != NULL);

	cdev = (HgCairoDevice *)device;
	cairo_destroy(cdev->reference);
	cairo_surface_destroy(cdev->surface);
	XDestroyWindow(cdev->u.xlib.dpy, cdev->u.xlib.drawable);
	XCloseDisplay(cdev->u.xlib.dpy);
}
