/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgplugins.h
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
#ifndef __HG_PLUGINS_H__
#define __HG_PLUGINS_H__

#include <hieroglyph/hgtypes.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define HG_PLUGIN_VERSION			0x0001
#define HG_PLUGIN_GET_PLUGIN_TYPE(_obj)		((HgPluginType)HG_OBJECT_GET_USER_DATA (&(_obj)->object))


struct _HieroGlyphPluginVTable {
	gboolean (* init)     (void);
	gboolean (* finalize) (void);
	gboolean (* load)     (HgPlugin *plugin,
			       HgVM     *vm);
	gboolean (* unload)   (HgPlugin *plugin,
			       HgVM     *vm);
};

struct _HieroGlyphPlugin {
	HgObject        object;
	gint16          version;
	HgPluginVTable *vtable;
	GModule        *module;
	gpointer        user_data;
	gboolean        is_loaded;
};


HgPlugin *hg_plugin_new   (HgMemPool    *pool,
			   HgPluginType  type);
HgPlugin *hg_plugin_open  (HgMemPool    *pool,
			   const gchar  *name,
			   HgPluginType  type);
gboolean  hg_plugin_load  (HgPlugin     *plugin,
			   HgVM         *vm);
gboolean  hg_plugin_unload(HgPlugin     *plugin,
			   HgVM         *vm);

G_END_DECLS

#endif /* __HG_PLUGINS_H__ */
