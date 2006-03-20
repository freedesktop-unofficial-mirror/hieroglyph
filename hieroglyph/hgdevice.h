/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.h
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
#ifndef __HG_DEVICE_H__
#define __HG_DEVICE_H__

#include <gmodule.h>
#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


typedef HgDevice * (*HgDeviceOpenFunc)  (void);
typedef void       (*HgDeviceCloseFunc) (HgDevice   *device);

struct _HieroGlyphDeviceVTable {
	gboolean (* initialize) (HgDevice          *device,
				 HgPage            *page);
	gboolean (* finalize)   (HgDevice          *device);
	gboolean (* eofill)     (HgDevice          *device,
				 HgRenderFill      *data);
	gboolean (* fill)       (HgDevice          *device,
				 HgRenderFill      *data);
	gboolean (* stroke)     (HgDevice          *device,
				 HgRenderStroke    *data);
};

struct _HieroGlyphDevice {
	HgDeviceVTable *vtable;
	gdouble         width;
	gdouble         height;
};


HgDevice *hg_device_new(const gchar *name);

/* device control functions */
void     hg_device_destroy(HgDevice *device);
gboolean hg_device_draw   (HgDevice *device,
			   HgPage   *page);

G_END_DECLS

#endif /* __HG_DEVICE_H__ */