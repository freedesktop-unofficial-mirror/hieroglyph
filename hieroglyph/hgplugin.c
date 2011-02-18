/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgplugin.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "hgmem.h"
#include "hgplugin.h"

#include "hgplugin.proto.h"

#define CHECK_SYMBOL(_mod_,_sym_)					\
	HG_STMT_START {							\
		g_module_symbol(_mod_, #_sym_, &(_sym_));		\
		if ((_sym_) == NULL) {					\
			hg_warning("%s", g_module_error());		\
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror); \
		}							\
	} HG_STMT_END

/*< private >*/
static hg_plugin_t *
_hg_plugin_load(hg_mem_t        *mem,
		const hg_char_t *filename)
{
	GModule *module;
	hg_plugin_t *retval = NULL;
	hg_pointer_t plugin_new = NULL;

	if ((module = g_module_open(filename,
				    G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL)) != NULL) {
		CHECK_SYMBOL (module, plugin_new);
		if (plugin_new == NULL)
			return NULL;

		retval = ((hg_plugin_new_func_t)plugin_new) (mem);
		if (retval) {
			retval->module = module;
		} else {
			return NULL;
		}
	}

	return retval;
}

/*< public >*/
/**
 * hg_plugin_open:
 * @mem:
 * @name:
 * @type:
 *
 * FIXME
 *
 * Returns:
 */
hg_plugin_t *
hg_plugin_open(hg_mem_t         *mem,
	       const hg_char_t  *name,
	       hg_plugin_type_t  type)
{
	hg_plugin_t *retval = NULL;
	hg_char_t *realname, *modulename = NULL, *fullname;
	const hg_char_t *modpath;
	static const hg_char_t *typename[HG_PLUGIN_END] = {
		"",
		"extension",
		"device",
	};

	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, NULL, HG_e_VMerror);

	realname = g_path_get_basename(name);
	switch (type) {
	    case HG_PLUGIN_EXTENSION:
		    modulename = g_strdup_printf("libext-%s.so", realname);
		    break;
	    case HG_PLUGIN_DEVICE:
		    modulename = g_strdup_printf("libdevice-%s.so", realname);
		    break;
	    default:
		    hg_warning("Unknown plugin type: %d", type);
		    hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		    goto finalize;
	}
	if ((modpath = g_getenv("HIEROGLYPH_PLUGIN_PATH")) != NULL) {
		hg_char_t **path_list = g_strsplit(modpath, G_SEARCHPATH_SEPARATOR_S, -1);
		hg_char_t *p, *path;
		hg_int_t i = 0;
		size_t len;

		while (path_list[i]) {
			p = path_list[i];

			while (*p && g_ascii_isspace(*p))
				p++;
			len = strlen(p);
			while (len > 0 && g_ascii_isspace(p[len - 1]))
				len--;
			path = g_strndup(p, len);

			if (path[0] != 0) {
				fullname = g_build_filename(path, modulename, NULL);
				retval = _hg_plugin_load(mem, fullname);
				g_free(fullname);
			}
			g_free(path);
			if (retval != NULL)
				break;
			i++;
		}
		g_strfreev(path_list);
	}
	if (retval == NULL) {
		fullname = g_build_filename(HIEROGLYPH_PLUGINDIR, modulename, NULL);
		retval = _hg_plugin_load(mem, fullname);
		g_free(fullname);
	}
	if (retval == NULL) {
		hg_debug(HG_MSGCAT_PLUGIN, "No such %s plugins found: %s", typename[type], realname);
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_undefinedfilename);
	}
  finalize:
	g_free(realname);
	g_free(modulename);

	return retval;
}

/**
 * hg_plugin_new:
 * @mem:
 * @info:
 *
 * FIXME
 *
 * Returns:
 */
hg_plugin_t *
hg_plugin_new(hg_mem_t         *mem,
	      hg_plugin_info_t *info)
{
	hg_plugin_t *retval;
	hg_quark_t q;

	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (info != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (info->version == HG_PLUGIN_VERSION, NULL, HG_e_VMerror);

	q = hg_mem_alloc_with_flags(mem,
				    sizeof (hg_plugin_t),
				    HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE,
				    (hg_pointer_t *)&retval);
	if (q == Qnil)
		return NULL;

	retval->self = q;
	retval->mem = mem;
	retval->vtable = info->vtable;

	return retval;
}

/**
 * hg_plugin_destroy:
 * @plugin:
 *
 * FIXME
 */
void
hg_plugin_destroy(hg_plugin_t *plugin)
{
	hg_return_if_fail (plugin != NULL, HG_e_VMerror);

	hg_mem_free(plugin->mem, plugin->self);
}

/**
 * hg_plugin_load:
 * @plugin:
 * @vm:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_plugin_load(hg_plugin_t  *plugin,
	       hg_pointer_t  vm)
{
	hg_bool_t retval;

	hg_return_val_if_fail (plugin != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (plugin->vtable != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (plugin->vtable->load != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	retval = plugin->vtable->load(plugin, vm);
	if (retval) {
		plugin->is_loaded = TRUE;
	} else {
		hg_warning("Unable to load a plugin: %s",
			   g_module_name(plugin->module));
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	return retval;
}

/**
 * hg_plugin_unload:
 * @plugin:
 * @vm:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_plugin_unload(hg_plugin_t  *plugin,
		 hg_pointer_t  vm)
{
	hg_bool_t retval;

	hg_return_val_if_fail (plugin != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (plugin->vtable != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (plugin->vtable->unload != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	retval = plugin->vtable->unload(plugin, vm);
	if (retval) {
		plugin->is_loaded = FALSE;
	} else {
		hg_warning("Unable to unload a plugin: %s",
			   g_module_name(plugin->module));
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	return retval;
}
