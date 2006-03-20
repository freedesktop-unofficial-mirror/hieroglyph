/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * ps2x.c
 * Copyright (C) 2005 Akira TAGOH
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <hieroglyph/hgfilter.h>
#include <hieroglyph/hgdevice.h>

int
main(int argc, char **argv)
{
	HgFilter *filter;
	HgDevice *device;
	int retval = 1;
	int fd;

	hieroglyph_init();

	if (argc < 2) {
		printf("Usage: %s <PS file>\n", argv[0]);
		return 1;
	}
	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		printf("%s\n", strerror(errno));
		return 1;
	}
	filter = hieroglyph_filter_new("ps");
	device = hieroglyph_device_new("x11");
	if (filter != NULL && device != NULL) {
		if (!hieroglyph_filter_open_stream(filter, fd, argv[1])) {
			goto ensure;
		}
		hieroglyph_filter_set_device(filter, device);
		retval = hieroglyph_filter_parse(filter);
		hieroglyph_filter_close_stream(filter);
	}

  ensure:
	if (filter != NULL)
		hieroglyph_object_unref(HG_OBJECT (filter));
	if (device != NULL)
		hieroglyph_object_unref(HG_OBJECT (device));

	return retval;
}
