/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGDEVICE_H__
#define __HIEROGLYPH_HGDEVICE_H__

#include <gmodule.h>
#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

typedef hg_device_t *	(* hg_device_init_func_t)     (void);
typedef void		(* hg_device_finalize_func_t) (hg_device_t *device);
typedef gboolean        (* hg_device_operator_t)      (hg_device_t *device,
						       hg_gstate_t *gstate);

struct _hg_device_t {
	/*< private >*/
	GModule                   *module;
	hg_device_finalize_func_t  finalizer;

	/*< protected >*/
	hg_device_operator_t fill;
	hg_device_operator_t stroke;

	/*< public >*/
	
};


hg_device_t *hg_device_open  (const gchar  *name);
void         hg_device_close (hg_device_t  *device);
gboolean     hg_device_fill  (hg_device_t  *device,
			      hg_gstate_t  *gstate,
			      GError      **error);
gboolean     hg_device_stroke(hg_device_t  *device,
			      hg_gstate_t  *gstate,
			      GError      **error);

/* null device */
hg_device_t *hg_device_null_new(void);

/* modules */
hg_device_t *hg_init    (void);
void         hg_finalize(hg_device_t *device);

G_END_DECLS

#endif /* __HIEROGLYPH_HGDEVICE_H__ */
