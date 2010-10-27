/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "hgerror.h"
#include "hgdevice.h"

/*< private >*/
static hg_device_t *
_hg_device_open(const gchar *modulename)
{
	GModule *module;
	hg_device_t *retval = NULL;
	gpointer dev_init = NULL, dev_fini = NULL;

	if ((module = g_module_open(modulename, G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL)) != NULL) {
		g_module_symbol(module, "hg_init", &dev_init);
		g_module_symbol(module, "hg_finalize", &dev_fini);

		if (!dev_init || !dev_fini) {
			g_warning(g_module_error());
			g_module_close(module);

			return NULL;
		}

		retval = ((hg_device_init_func_t)dev_init)();

		if (retval) {
			retval->module = module;
			retval->finalizer = dev_fini;
		}
#if defined (HG_DEBUG) && defined (HG_MODULE_DEBUG)
	} else {
		g_warning(g_module_error());
#endif /* HG_DEBUG && HG_MODULE_DEBUG */
	}

	return retval;
}

/*< public >*/
/**
 * hg_device_open:
 * @name:
 *
 * FIXME
 *
 * Returns:
 */
hg_device_t *
hg_device_open(const gchar *name)
{
	hg_device_t *retval = NULL;
	gchar *basename, *modulename, *fullname;
	const gchar *modpath;

	hg_return_val_if_fail (name != NULL, NULL);

	basename = g_path_get_basename(name);
	modulename = g_strdup_printf("libhgdev-%s.so", basename);
	if ((modpath = g_getenv("HIEROGLYPH_DEVICE_PATH")) != NULL) {
		gchar **path_list = g_strsplit(modpath, G_SEARCHPATH_SEPARATOR_S, -1);
		gchar *p, *path;
		gint i = 0;
		gsize len;

		for (i = 0; path_list[i] != NULL; i++) {
			p = path_list[i];

			while (*p && g_ascii_isspace(*p))
				p++;
			len = strlen(p);
			while (len > 0 && g_ascii_isspace(p[len - 1]))
				len--;
			path = g_strndup(p, len);
			if (path[0] != 0) {
				fullname = g_build_filename(path, modulename, NULL);
				retval = _hg_device_open(fullname);
				g_free(fullname);
			}
			g_free(path);
			if (retval)
				break;
		}

		g_strfreev(path_list);
	}
	if (retval == NULL) {
		fullname = g_build_filename(HIEROGLYPH_DEVICEDIR, modulename, NULL);
		retval = _hg_device_open(fullname);
		g_free(fullname);
	}
	if (retval == NULL) {
		g_warning("No such device module: %s", basename);
	}

	g_free(modulename);
	g_free(basename);

	return retval;
}

/**
 * hg_device_destroy:
 * @device:
 *
 * FIXME
 */
void
hg_device_close(hg_device_t *device)
{
	GModule *module;

	hg_return_if_fail (device != NULL);
	hg_return_if_fail (device->module != NULL);
	hg_return_if_fail (device->finalizer != NULL);

	module = device->module;
	device->finalizer(device);

	g_module_close(module);
}
