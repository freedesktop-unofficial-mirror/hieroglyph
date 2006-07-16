/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * libedit-main.c
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

#ifndef USE_LIBEDIT
#error Please install libedit and run configure again.  This plugin is for libedit only.
#endif /* !USE_LIBEDIT */

#include <editline/readline.h>
#include <hieroglyph/hgplugins.h>
#include <hieroglyph/hglineedit.h>
#include <hieroglyph/vm.h>


static gboolean  plugin_init                             (void);
static gboolean  plugin_finalize                         (void);
static gboolean  plugin_load                             (HgPlugin *plugin,
							  HgVM     *vm);
static gboolean  plugin_unload                           (HgPlugin *plugin,
							  HgVM     *vm);
static gchar    *_hg_line_edit__libedit_real_get_line    (const gchar *prompt);
static void      _hg_line_edit__libedit_real_add_history (const gchar *strings);
static void      _hg_line_edit__libedit_real_load_history(const gchar *filename);
static void      _hg_line_edit__libedit_real_save_history(const gchar *filename);


static HgPluginVTable vtable = {
	.init     = plugin_init,
	.finalize = plugin_finalize,
	.load     = plugin_load,
	.unload   = plugin_unload,
};

static const HgLineEditVTable lineedit_vtable = {
	.get_line     = _hg_line_edit__libedit_real_get_line,
	.add_history  = _hg_line_edit__libedit_real_add_history,
	.load_history = _hg_line_edit__libedit_real_load_history,
	.save_history = _hg_line_edit__libedit_real_save_history,
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
	HgLineEdit *editor;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		g_warning("already loaded.");
		return FALSE;
	}
	plugin->user_data = hg_vm_get_line_editor(vm);

	editor = hg_line_edit_new(hg_vm_get_current_pool(vm),
				  &lineedit_vtable);
	hg_vm_set_line_editor(vm, editor);

	return TRUE;
}

static gboolean
plugin_unload(HgPlugin *plugin,
	      HgVM     *vm)
{
	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		g_warning("not yet loaded.");
		return FALSE;
	}
	hg_vm_set_line_editor(vm, plugin->user_data);

	return TRUE;
}

static gchar *
_hg_line_edit__libedit_real_get_line(const gchar *prompt)
{
	gchar *retval;

	if (prompt == NULL)
		retval = readline("");
	else
		retval = readline(prompt);

	return retval;
}

static void
_hg_line_edit__libedit_real_add_history(const gchar *strings)
{
	g_return_if_fail (strings != NULL);

	add_history(strings);
}

static void
_hg_line_edit__libedit_real_load_history(const gchar *filename)
{
	g_return_if_fail (filename != NULL);

	read_history(filename);
}

static void
_hg_line_edit__libedit_real_save_history(const gchar *filename)
{
	g_return_if_fail (filename != NULL);

	write_history(filename);
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
