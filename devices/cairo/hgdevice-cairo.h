/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice-cairo.h
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
#ifndef __HG_DEVICE_CAIRO_H__
#define __HG_DEVICE_CAIRO_H__

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgdevice.h>

G_BEGIN_DECLS

typedef enum {
	HG_DEVICE_CAIRO_XLIB_SURFACE = 1,
} HgCairoDeviceType;

typedef struct _HieroGlyphCairoXlibDevice	HgCairoXlibDevice;
typedef struct _HieroGlyphCairoDevice		HgCairoDevice;

struct _HieroGlyphCairoXlibDevice {
	HgCairoDeviceType  type;
	Display           *dpy;
	Drawable           drawable;
};

struct _HieroGlyphCairoDevice {
	HgDevice  device;
	struct {
		gboolean (* set_page_size) (HgCairoDevice *device,
					    gdouble        width,
					    gdouble        height);
	} vtable;
	union {
		HgCairoDeviceType type;
		HgCairoXlibDevice xlib;
	} u;
	cairo_surface_t *surface;
	cairo_t         *reference;
};

HgDevice *hg_cairo_device_new(void);

G_END_DECLS

#endif /* __HG_DEVICE_CAIRO_H__ */
