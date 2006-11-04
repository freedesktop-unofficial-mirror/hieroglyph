/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgplugins.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "hgplugins.h"
#include "hgmem.h"
#include "hglog.h"


static void _hg_plugin_real_free     (gpointer data);
static void _hg_plugin_real_set_flags(gpointer data,
				      guint    flags);


static HgObjectVTable __hg_plugin_vtable = {
	.free      = _hg_plugin_real_free,
	.set_flags = _hg_plugin_real_set_flags,
	.relocate  = NULL,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_hg_plugin_real_free(gpointer data)
{
	HgPlugin *plugin = data;

	if (plugin->vtable->finalize) {
		plugin->vtable->finalize();
	}
	if (plugin->module) {
		g_module_close(plugin->module);
		plugin->module = NULL;
	}
}

static void
_hg_plugin_real_set_flags(gpointer data,
			  guint    flags)
{
	HgPlugin *plugin = data;
	HgMemObject *obj;

	if (plugin->user_data) {
		hg_mem_get_object__inline(plugin->user_data, obj);
		if (obj != NULL) {
			hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static HgPlugin *
_hg_plugin_load(HgMemPool   *pool,
		const gchar *filename)
{
	GModule *module;
	HgPlugin *retval = NULL;
	gpointer plugin_new = NULL;

	if ((module = g_module_open(filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)) != NULL) {
#define _CHECK_SYMBOL(_sym)						\
		g_module_symbol(module, #_sym, &(_sym));		\
		if ((_sym) == NULL) {					\
			hg_log_warning(g_module_error());			\
			g_module_close(module);				\
			return NULL;					\
		}

		_CHECK_SYMBOL (plugin_new);

#undef _CHECK_SYMBOL

		/* initialize a plugin and get a HgPlugin instance */
		retval = ((HgPluginNewFunc)plugin_new) (pool);
		if (retval == NULL) {
			hg_log_warning("Failed to create an instance of the plugin `%s'", filename);
			g_module_close(module);
		} else if (retval->version < HG_PLUGIN_VERSION ||
			   retval->vtable == NULL) {
			hg_log_warning("`%s' is an invalid plugin.", filename);
			g_module_close(module);
			retval = NULL;
		} else {
			retval->module = module;
		}
	} else {
		hg_log_debug(DEBUG_PLUGIN, g_module_error());
	}

	return retval;
}

/*
 * Public Functions
 */
HgPlugin *
hg_plugin_new(HgMemPool    *pool,
	      HgPluginType  type)
{
	HgPlugin *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool,
					 sizeof (HgPlugin),
					 HG_FL_HGOBJECT | HG_FL_COMPLEX);
	if (retval == NULL) {
		hg_log_warning("Failed to create a plugin.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_plugin_vtable);
	HG_OBJECT_SET_USER_DATA (&retval->object, type);
	retval->version = 0;
	retval->vtable = NULL;
	retval->module = NULL;
	retval->user_data = NULL;

	return retval;
}

HgPlugin *
hg_plugin_open(HgMemPool    *pool,
	       const gchar  *name,
	       HgPluginType  type)
{
	HgPlugin *retval = NULL;
	gchar *realname, *modulename, *fullmodname;
	const gchar *modpath;
	const gchar *typenames[HG_PLUGIN_END] = {
		"",
		"extension",
		"device"
	};

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	realname = g_path_get_basename(name);
	switch (type) {
	    case HG_PLUGIN_EXTENSION:
		    modulename = g_strdup_printf("libext-%s.so", realname);
		    break;
	    case HG_PLUGIN_DEVICE:
		    modulename = g_strdup_printf("libdevice-%s.so", realname);
		    break;
	    default:
		    hg_log_warning("Unknown plugin type: %d", type);
		    return NULL;
	}
	if ((modpath = g_getenv("HIEROGLYPH_PLUGIN_PATH")) != NULL) {
		gchar **path_list = g_strsplit(modpath, G_SEARCHPATH_SEPARATOR_S, -1);
		gchar *p, *path;
		gint i = 0;
		size_t len;

		while (path_list[i]) {
			p = path_list[i];

			/* trims leading and trailing whitespace from path */
			while (*p && g_ascii_isspace(*p))
				p++;
			len = strlen(p);
			while (len > 0 && g_ascii_isspace(p[len - 1]))
				len--;
			path = g_strndup(p, len);

			if (path[0] != '\0') {
				fullmodname = g_build_filename(path, modulename, NULL);
				retval = _hg_plugin_load(pool, fullmodname);
				g_free(fullmodname);
			}
			g_free(path);
			if (retval != NULL)
				break;
			i++;
		}
		g_strfreev(path_list);
	}
	if (retval == NULL) {
		fullmodname = g_build_filename(HIEROGLYPH_PLUGINDIR, modulename, NULL);
		retval = _hg_plugin_load(pool, fullmodname);
		g_free(fullmodname);
	}
	if (retval == NULL) {
		hg_log_warning("No `%s' %s plugin module found.", realname, typenames[type]);
	} else {
		/* initialize a plugin */
		if (retval->vtable->init == NULL ||
		    !retval->vtable->init()) {
			hg_log_warning("Failed to initialize a plugin `%s'", realname);
			retval = NULL;
		}
	}

	g_free(realname);
	g_free(modulename);

	return retval;
}

gboolean
hg_plugin_load(HgPlugin *plugin,
	       HgVM     *vm)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (plugin->vtable != NULL && plugin->vtable->load != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	retval = plugin->vtable->load(plugin, vm);
	if (retval) {
		plugin->is_loaded = TRUE;
	} else {
		hg_log_warning("Failed to load a plugin `%s'",
			       g_module_name(plugin->module));
	}

	return retval;
}

gboolean
hg_plugin_unload(HgPlugin *plugin,
		 HgVM     *vm)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (plugin->vtable != NULL && plugin->vtable->unload != NULL, FALSE);
	g_return_val_if_fail (plugin->is_loaded == TRUE, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	retval = plugin->vtable->unload(plugin, vm);
	if (retval)
		plugin->is_loaded = FALSE;

	return retval;
}
