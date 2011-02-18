/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgplugin.h
 * Copyright (C) 2006-2011 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGPLUGIN_H__
#define __HIEROGLYPH_HGPLUGIN_H__

#include <gmodule.h>
#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

#define HG_PLUGIN_VERSION	0x0001

typedef struct _hg_plugin_vtable_t	hg_plugin_vtable_t;
typedef struct _hg_plugin_t		hg_plugin_t;
typedef struct _hg_plugin_info_t	hg_plugin_info_t;
typedef enum _hg_plugin_type_t		hg_plugin_type_t;
typedef hg_plugin_t * (* hg_plugin_new_func_t) (hg_mem_t  *mem);

struct _hg_plugin_vtable_t {
	hg_bool_t (* init)     (void);
	hg_bool_t (* finalize) (void);
	hg_bool_t (* load)     (hg_plugin_t  *plugin,
				hg_pointer_t  vm);
	hg_bool_t (* unload)   (hg_plugin_t  *plugin,
				hg_pointer_t  vm);
};
struct _hg_plugin_t {
	hg_mem_t           *mem;
	hg_quark_t          self;
	hg_plugin_vtable_t *vtable;
	GModule            *module;
	hg_bool_t           is_loaded;
	hg_pointer_t        user_data;
};
struct _hg_plugin_info_t {
	hg_uint_t           version;
	hg_plugin_vtable_t *vtable;
};
enum _hg_plugin_type_t {
	HG_PLUGIN_EXTENSION = 1,
	HG_PLUGIN_DEVICE,
	HG_PLUGIN_END
};


hg_plugin_t *hg_plugin_open   (hg_mem_t         *mem,
                               const hg_char_t  *name,
                               hg_plugin_type_t  type);
hg_plugin_t *hg_plugin_new    (hg_mem_t         *mem,
                               hg_plugin_info_t *info);
void         hg_plugin_destroy(hg_plugin_t      *plugin);
hg_bool_t    hg_plugin_load   (hg_plugin_t      *plugin,
                               hg_pointer_t      vm);
hg_bool_t    hg_plugin_unload (hg_plugin_t      *plugin,
                               hg_pointer_t      vm);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGPLUGIN_H__ */
