/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgplugin.c
 * Copyright (C) 2006-2010 Akira TAGOH
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

#include "hgplugin.proto"

#define CHECK_SYMBOL(_mod_,_sym_,_err_)					\
	G_STMT_START {							\
		g_module_symbol(_mod_, #_sym_, &(_sym_));		\
		if ((_sym_) == NULL) {					\
			g_set_error(&(_err_), HG_ERROR, HG_VM_e_VMerror, \
				    g_module_error());			\
		}							\
	} G_STMT_END

/*< private >*/
static hg_plugin_t *
_hg_plugin_load(hg_mem_t     *mem,
		const gchar  *filename,
		GError      **error)
{
	GModule *module;
	hg_plugin_t *retval = NULL;
	gpointer plugin_new = NULL;
	GError *err = NULL;

	if ((module = g_module_open(filename,
				    G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL)) != NULL) {
		CHECK_SYMBOL (module, plugin_new, err);
		if (plugin_new == NULL)
			goto error;

		retval = ((hg_plugin_new_func_t)plugin_new) (mem, &err);
		if (retval) {
			retval->module = module;
		} else {
			if (err == NULL)
				g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
					    "Unable to create a plugin instance: %s", filename);
			goto error;
		}
	} else {
		g_set_error(&err, HG_ERROR, HG_VM_e_undefinedfilename,
			    g_module_error());
	}
  error:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

/*< public >*/
/**
 * hg_plugin_open:
 * @mem:
 * @name:
 * @type:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_plugin_t *
hg_plugin_open(hg_mem_t          *mem,
	       const gchar       *name,
	       hg_plugin_type_t   type,
	       GError           **error)
{
	hg_plugin_t *retval = NULL;
	gchar *realname, *modulename = NULL, *fullname;
	const gchar *modpath;
	static const gchar *typename[HG_PLUGIN_END] = {
		"",
		"extension",
		"device",
	};
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (mem != NULL, NULL, error, HG_VM_e_VMerror);
	hg_return_val_with_gerror_if_fail (name != NULL, NULL, error, HG_VM_e_VMerror);

	realname = g_path_get_basename(name);
	switch (type) {
	    case HG_PLUGIN_EXTENSION:
		    modulename = g_strdup_printf("libext-%s.so", realname);
		    break;
	    case HG_PLUGIN_DEVICE:
		    modulename = g_strdup_printf("libdevice-%s.so", realname);
		    break;
	    default:
		    g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
				"Unknown plugin type: %d", type);
		    goto error;
	}
	if ((modpath = g_getenv("HIEROGLYPH_PLUGIN_PATH")) != NULL) {
		gchar **path_list = g_strsplit(modpath, G_SEARCHPATH_SEPARATOR_S, -1);
		gchar *p, *path;
		gint i = 0;
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
				retval = _hg_plugin_load(mem, fullname, &err);
				g_free(fullname);
			}
			g_free(path);
			if (retval != NULL)
				break;
			i++;
			if (err)
				g_clear_error(&err);
		}
		g_strfreev(path_list);
	}
	if (retval == NULL) {
		fullname = g_build_filename(HIEROGLYPH_PLUGINDIR, modulename, NULL);
		retval = _hg_plugin_load(mem, fullname, &err);
		g_free(fullname);
		if (err)
			g_clear_error(&err);
	}
	if (retval == NULL) {
		g_set_error(&err, HG_ERROR, HG_VM_e_undefinedfilename,
			    "No such %s plugins found: %s",
			    typename[type],
			    realname);
		goto error;
	}
  error:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}
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

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (info != NULL, NULL);
	hg_return_val_if_fail (info->version == HG_PLUGIN_VERSION, NULL);

	q = hg_mem_alloc_with_flags(mem,
				    sizeof (hg_plugin_t),
				    HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE,
				    (gpointer *)&retval);
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
	hg_return_if_fail (plugin != NULL);

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
gboolean
hg_plugin_load(hg_plugin_t  *plugin,
	       gpointer      vm,
	       GError      **error)
{
	gboolean retval;
	GError *err = NULL;

	hg_return_val_if_fail (plugin != NULL, FALSE);
	hg_return_val_if_fail (plugin->vtable != NULL, FALSE);
	hg_return_val_if_fail (plugin->vtable->load != NULL, FALSE);
	hg_return_val_if_fail (vm != NULL, FALSE);

	retval = plugin->vtable->load(plugin, vm, &err);
	if (retval) {
		plugin->is_loaded = TRUE;
	} else {
		if (err == NULL) {
			g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
				    "Unable to load a plugin: %s",
				    g_module_name(plugin->module));
		}
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
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
gboolean
hg_plugin_unload(hg_plugin_t  *plugin,
		 gpointer      vm,
		 GError      **error)
{
	gboolean retval;
	GError *err = NULL;

	hg_return_val_if_fail (plugin != NULL, FALSE);
	hg_return_val_if_fail (plugin->vtable != NULL, FALSE);
	hg_return_val_if_fail (plugin->vtable->unload != NULL, FALSE);
	hg_return_val_if_fail (vm != NULL, FALSE);

	retval = plugin->vtable->unload(plugin, vm, &err);
	if (retval) {
		plugin->is_loaded = FALSE;
	} else {
		if (err == NULL) {
			g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
				    "Unable to unload a plugin: %s",
				    g_module_name(plugin->module));
		}
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}
