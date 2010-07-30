/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglineedit.h
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
#ifndef __HIEROGLYPH_HGLINEEDIT_H__
#define __HIEROGLYPH_HGLINEEDIT_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgfile.h>

G_BEGIN_DECLS

typedef struct _hg_lineedit_vtable_t	hg_lineedit_vtable_t;
typedef struct _hg_lineedit_t		hg_lineedit_t;

struct _hg_lineedit_vtable_t {
	gchar *  (* get_line)     (hg_lineedit_t *lineedit,
				   const gchar   *prompt,
				   gpointer       user_data);
	void     (* add_history)  (hg_lineedit_t *lineedit,
				   const gchar   *history,
				   gpointer       user_data);
	gboolean (* load_history) (hg_lineedit_t *lineedit,
				   const gchar   *historyfile,
				   gpointer       user_data);
	gboolean (* save_history) (hg_lineedit_t *lineedit,
				   const gchar   *historyfile,
				   gpointer       user_data);
};


hg_lineedit_vtable_t *hg_lineedit_get_default_vtable(void) G_GNUC_CONST;
hg_lineedit_t        *hg_lineedit_new               (hg_mem_t                   *mem,
                                                     const hg_lineedit_vtable_t *vtable,
                                                     hg_file_t                  *infile,
                                                     hg_file_t                  *outfile);
void                  hg_lineedit_destroy           (hg_lineedit_t              *lineedit);
gchar                *hg_lineedit_get_line          (hg_lineedit_t              *lineedit,
                                                     const gchar                *prompt,
                                                     gboolean                    enable_history);
gchar                *hg_lineedit_get_statement     (hg_lineedit_t              *lineedit,
                                                     const gchar                *prompt,
                                                     gboolean                    enable_history);
gboolean              hg_lineedit_load_history      (hg_lineedit_t              *lineedit,
                                                     const gchar                *historyfile);
gboolean              hg_lineedit_save_history      (hg_lineedit_t              *lineedit,
                                                     const gchar                *historyfile);
void                  hg_lineedit_set_infile        (hg_lineedit_t              *lineedit,
                                                     hg_file_t                  *infile);
void                  hg_lineedit_set_outfile       (hg_lineedit_t              *lineedit,
                                                     hg_file_t                  *outfile);

G_END_DECLS

#endif /* __HIEROGLYPH_HGLINEEDIT_H__ */
