/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.c
 * Copyright (C) 2005-2006 Akira TAGOH
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
#include <gmodule.h>
#include "hgdevice.h"
#include "hglog.h"

/*
 * Private Functions
 */
static HgDevice *
_hg_device_open(const gchar *filename)
{
	GModule *module;
	HgDevice *retval = NULL;
	gpointer open_symbol = NULL, close_symbol = NULL;

	if ((module = g_module_open(filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)) != NULL) {
		g_module_symbol(module, "device_open", &open_symbol);
		g_module_symbol(module, "device_close", &close_symbol);
		if (open_symbol && close_symbol) {
			retval = ((HgDeviceOpenFunc)open_symbol) ();
		} else {
			hg_log_warning(g_module_error());
			g_module_close(module);

			return NULL;
		}
#ifdef DEBUG_MODULES
	} else {
		hg_log_warning(g_module_error());
#endif /* DEBUG_MODULES */
	}

	return retval;
}

/*
 * Public Functions
 */
HgDevice *
hg_device_new(const gchar *name)
{
	HgDevice *retval = NULL;
	gchar *realname, *modulename, *fullmodname;
	const gchar *modpath;

	g_return_val_if_fail (name != NULL, NULL);

	realname = g_path_get_basename(name);
	modulename = g_strdup_printf("lib%s-device.so", realname);
	if ((modpath = g_getenv("HIEROGLYPH_DEVICE_PATH")) != NULL) {
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
				fullmodname = g_build_filename(path, modulename, NULL);
				retval = _hg_device_open(fullmodname);
				g_free(fullmodname);
			}
			g_free(path);
			if (retval != NULL) {
				break;
			}
			i++;
		}
		g_strfreev(path_list);
	}
	if (retval == NULL) {
		fullmodname = g_build_filename(HIEROGLYPH_DEVICEDIR, modulename, NULL);
		retval = _hg_device_open(fullmodname);
		g_free(fullmodname);
	}
	if (retval == NULL)
		hg_log_warning("No `%s' device module found.", realname);

	g_free(realname);
	g_free(modulename);

	return retval;
}

void
hg_device_destroy(HgDevice *device)
{
	g_return_if_fail (device != NULL);
}

gboolean
hg_device_draw(HgDevice *device,
	       HgPage   *page)
{
	GList *l;
	HgRender *render;
	gboolean retval = TRUE;

	g_return_val_if_fail (device != NULL, FALSE);
	g_return_val_if_fail (page != NULL, FALSE);
	g_return_val_if_fail (device->vtable != NULL, FALSE);

	if (device->vtable->initialize) {
		if (!device->vtable->initialize(device, page))
			return FALSE;
	} else {
		/* page initialization function is a must */
		return FALSE;
	}
	for (l = page->node; l != NULL; l = g_list_next(l)) {
		render = l->data;

		switch (render->u.type) {
		    case HG_RENDER_EOFILL:
			    if (device->vtable->eofill)
				    if (!device->vtable->eofill(device, &render->u.fill))
					    retval = FALSE;
			    break;
		    case HG_RENDER_FILL:
			    if (device->vtable->fill)
				    if (!device->vtable->fill(device, &render->u.fill))
					    retval = FALSE;
			    break;
		    case HG_RENDER_STROKE:
			    if (device->vtable->stroke)
				    if (!device->vtable->stroke(device, &render->u.stroke))
					    retval = FALSE;
			    break;
		    case HG_RENDER_DEBUG:
			    if (render->u.debug.func)
				    render->u.debug.func(render->u.debug.data);
			    break;
		    default:
			    hg_log_warning("Unknown rendering code %d\n", render->u.type);
			    break;
		}
	}
	if (device->vtable->finalize) {
		if (!device->vtable->finalize(device))
			return FALSE;
	} else {
		/* page finalization function is a must */
		return FALSE;
	}

	return retval;
}
