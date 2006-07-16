/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * test-main.c
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

#include <hieroglyph/hgplugins.h>


static gboolean plugin_init    (void);
static gboolean plugin_finalize(void);
static gboolean plugin_load    (HgPlugin *plugin,
				HgVM     *vm);
static gboolean plugin_unload  (HgPlugin *plugin,
				HgVM     *vm);


static HgPluginVTable vtable = {
	.init     = plugin_init,
	.finalize = plugin_finalize,
	.load     = plugin_load,
	.unload   = plugin_unload,
};

/*
 * Private Functions
 */
static gboolean
plugin_init(void)
{
	return TRUE;
}

static gboolean
plugin_finalize(void)
{
	return TRUE;
}

static gboolean
plugin_load(HgPlugin *plugin,
	    HgVM     *vm)
{
	return TRUE;
}

static gboolean
plugin_unload(HgPlugin *plugin,
	      HgVM     *vm)
{
	return TRUE;
}

/*
 * Public Functions
 */
HgPlugin *
plugin_new(HgMemPool *pool)
{
	HgPlugin *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_plugin_new(pool, HG_PLUGIN_EXTENSION);
	retval->version = HG_PLUGIN_VERSION;
	retval->vtable = &vtable;

	return retval;
}
