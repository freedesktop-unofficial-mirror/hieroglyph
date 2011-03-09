/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgutils.c
 * Copyright (C) 2005-2011 Akira TAGOH
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

#include <glib.h>
#include <stdlib.h>
#include "hgmessages.h"
#include "hgutils.h"

#include "hgutils.proto.h"

/*< private >*/

/*< public >*/

/**
 * hg_find_libfile:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_char_t *
hg_find_libfile(const hg_char_t *file)
{
	const hg_char_t *env;
	hg_char_t **paths, *filename = NULL, *basename;
	hg_int_t i = 0;

	hg_return_val_if_fail (file != NULL, NULL, HG_e_VMerror);

	basename = g_path_get_basename(file);
	env = getenv("HIEROGLYPH_LIB_PATH");
	if (env) {
		paths = g_strsplit(env, ":", 0);
		if (paths) {
			for (i = 0; paths[i] != NULL; i++) {
				filename = g_build_filename(paths[i], basename, NULL);
				if (g_file_test(filename, G_FILE_TEST_EXISTS))
					break;
				g_free(filename);
				filename = NULL;
			}
			g_strfreev(paths);
		}
	}
	if (!filename) {
		filename = g_build_filename(HIEROGLYPH_LIBDIR, basename, NULL);
		if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
			g_free(filename);
			filename = NULL;
		}
	}
	if (basename)
		g_free(basename);

	return filename;
}
