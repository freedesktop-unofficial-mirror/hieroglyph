/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgs.c
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
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/vm.h>


static gboolean
_hgs_arg_define_cb(const char *option_name,
		   const char *value,
		   gpointer    data)
{
	gboolean retval = FALSE;
	HgVM *vm = data;
	HgValueNode *n1, *n2;
	gboolean mode = hg_vm_is_global_pool_used(vm);
	HgMemPool *pool;
	HgDict *systemdict = hg_vm_get_dict_systemdict(vm);

	if (value && *value) {
		hg_vm_use_global_pool(vm, TRUE);
		pool = hg_vm_get_current_pool(vm);

		/* FIXME: validate characters in value */
		n1 = hg_vm_get_name_node(vm, value);
		HG_VALUE_MAKE_BOOLEAN (pool, n2, TRUE);
		hg_dict_insert(pool, systemdict, n1, n2);

		hg_vm_use_global_pool(vm, mode);
		retval = TRUE;
	}

	return retval;
}

static gboolean
_hgs_arg_load_plugin_cb(const char *option_name,
			const char *value,
			gpointer    data)
{
	gboolean retval = FALSE;
	HgVM *vm = data;

	if (value && *value) {
		hg_vm_load_plugin(vm, value);
	}

	return retval;
}

int
main(int    argc,
     char **argv)
{
	HgVM *vm;
	GOptionContext *ctxt = g_option_context_new("<PostScript file>");
	GOptionGroup *group;
	GOptionEntry entries[] = {
		{"define", 'd', 0, G_OPTION_ARG_CALLBACK, _hgs_arg_define_cb, "Define a variable in systemdict.", "SYMBOL"},
		{"load-plugin", 'l', 0, G_OPTION_ARG_CALLBACK, _hgs_arg_load_plugin_cb, "Load a plugin", "NAME"},
		{NULL}
	};

	GError *error = NULL;
	const gchar *psfile = NULL;

	HG_MEM_INIT;
	hg_vm_init();

	vm = hg_vm_new(VM_EMULATION_LEVEL_1);

	group = g_option_group_new(NULL, NULL, NULL, vm, NULL);
	g_option_context_set_main_group(ctxt, group);
	g_option_context_add_main_entries(ctxt, entries, NULL);
	if (!g_option_context_parse(ctxt, &argc, &argv, &error)) {
		g_print("Option parse error.\n");
		hg_vm_finalize();
		return 1;
	}

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
