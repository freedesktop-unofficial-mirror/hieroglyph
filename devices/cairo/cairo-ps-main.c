/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * cairo-ps-main.c
 * Copyright (C) 2005,2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#include <config.h>
#endif

#include <unistd.h>
#include <cairo/cairo-ps.h>
#include <glib.h>
#include <hieroglyph/debug.h>
#include <hieroglyph/hglog.h>
#include "hgdevice-cairo.h"


/* Private Functions */
static cairo_status_t
_cairo_ps_embedded_write_stream(void                *closure,
				const unsigned char *data,
				unsigned int         length)
{
	HgCairoDevice *device = HG_CAIRO_DEVICE (closure);
	size_t written;

	written = write(device->u.ps.fd, data, length);

	if (written == length)
		return CAIRO_STATUS_SUCCESS;
	else
		return CAIRO_STATUS_WRITE_ERROR;
}

/* Public Functions */
HgDevice *
device_open(HgPageInfo *info)
{
	HgDevice *device;
	HgCairoDevice *cdev;
	gchar *filename = NULL;
	gint fd;

	TRACE_ENTER;

	fd = g_file_open_tmp("ps-embedded-XXXXXX", &filename, NULL);
	if (fd == -1) {
		hg_log_warning("Failed to open a file descriptor.");
		if (filename)
			g_free(filename);

		return NULL;
	}
	device = hieroglyph_cairo_device_new();
	cdev = HG_CAIRO_DEVICE (device);
	cdev->surface = cairo_ps_surface_create_for_stream(_cairo_ps_embedded_write_stream,
							   device,
							   info->width,
							   info->height);
	cdev->reference = cairo_create(cdev->surface);
	cdev->u.type = HG_DEVICE_CAIRO_PS_SURFACE;
	cdev->u.ps.output_filename = filename;
	cdev->u.ps.fd = fd;

	if (cairo_status(cdev->reference) != CAIRO_STATUS_SUCCESS) {
		hg_log_warning("Failed to create an reference of cairo.");
	}

	TRACE_LEAVE;

	return device;
}

void
device_close(HgDevice *device)
{
	HgCairoDevice *cdev;

	g_return_if_fail (HG_IS_CAIRO_DEVICE (device));

	TRACE_ENTER;

	hieroglyph_stream_write(device->ostream);

	cdev = HG_CAIRO_DEVICE (device);
	if (cdev->u.ps.fd >= 0)
		close(cdev->u.ps.fd);
	if (cdev->u.ps.output_filename != NULL)
		g_free(cdev->u.ps.output_filename);

	cairo_destroy(cdev->reference);
	cairo_surface_destroy(cdev->surface);

	TRACE_LEAVE;
}
