/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * pse.c
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
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/vm.h>

GList *plugin_list = NULL;

static gboolean
_pse_arg_plugin_cb(const char *option_name,
		   const char *value,
		   gpointer    data)
{
	gboolean retval = TRUE;

	if (value && *value) {
		plugin_list = g_list_append(plugin_list, g_strdup(value));
	} else {
		retval = FALSE;
	}

	return retval;
}

int
main(int argc, char **argv)
{
	HgVM *vm;
	GOptionContext *ctxt = g_option_context_new("PostScriptFile.ps");
	GOptionEntry entries[] = {
		{"plugin", 'l', 0, G_OPTION_ARG_CALLBACK, _pse_arg_plugin_cb, "A plugin to be loaded.", "NAME"},
		{NULL}
	};
	GError *error = NULL;
	const gchar *psfile = NULL;
	GList *l;

	g_option_context_add_main_entries(ctxt, entries, NULL);
	if (!g_option_context_parse(ctxt, &argc, &argv, &error)) {
		g_print("Option parse error.\n");
		return 1;
	}

	HG_MEM_INIT;
	hg_vm_init();

	vm = hg_vm_new(VM_EMULATION_LEVEL_1);
	for (l = plugin_list; l != NULL; l = g_list_next(l)) {
		hg_vm_load_plugin(vm, l->data);
		g_free(l->data);
	}
	g_list_free(plugin_list);
	plugin_list = NULL;

	if (argc > 1)
		psfile = argv[1];

	if (!hg_vm_startjob(vm, psfile, TRUE)) {
		g_print("Failed to start a job.\n");
		return 1;
	}

	hg_vm_finalize();
	g_option_context_free(ctxt);

	return 0;
}
