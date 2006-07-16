/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * cairo-xlib.c
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
#include <hieroglyph/hgdevice.h>
#include <hieroglyph/vm.h>
#include <hieroglyph/hggraphics.h>


int
main(int argc, char **argv)
{
	HG_MEM_INIT;

	HgVM *vm;
	HgGraphics *graphics;
	HgDevice *device;
	GList *l, *prev = NULL;
	gint i;
	GOptionContext *ctxt = g_option_context_new("PostScriptFile.ps");
	GError *error = NULL;
	const gchar *psfile = NULL;

	if (!g_option_context_parse(ctxt, &argc, &argv, &error)) {
		g_print("Option parse error.\n");
		return 1;
	}

	device = hg_device_new("x11");
	if (device == NULL) {
		g_print("Failed to open a x11 device\n");
		return 1;
	}

	hg_vm_init();

	vm = hg_vm_new(VM_EMULATION_LEVEL_1);

	if (argc > 1)
		psfile = argv[1];

	if (!hg_vm_startjob(vm, psfile, TRUE)) {
		g_print("Failed to start a job.\n");
		return 1;
	}

	if (!hg_vm_has_error(vm)) {
		graphics = hg_vm_get_graphics(vm);
		for (l = graphics->pages, i = 1; l != NULL; l = g_list_next(l), i++) {
			hg_device_draw(device, l->data);
			g_print("%d page\n", i);
			sleep(1);
		}
		prev = g_list_last(graphics->pages);
		if (prev != NULL &&
		    graphics->current_page != prev->data &&
		    graphics->current_page->node)
			hg_device_draw(device, graphics->current_page);
		sleep(5);
	}

	hg_vm_finalize();
	g_option_context_free(ctxt);

	return 0;
}
