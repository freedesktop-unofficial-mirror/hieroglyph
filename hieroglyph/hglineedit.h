/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglineedit.h
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
#ifndef __HG_LINEEDIT_H__
#define __HG_LINEEDIT_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS


HgLineEdit *hg_line_edit_new        (HgMemPool              *pool,
				     const HgLineEditVTable *vtable,
				     HgFileObject           *stdin,
				     HgFileObject           *stdout);
gchar      *hg_line_edit_get_line   (HgLineEdit             *lineedit,
				     const gchar            *prompt,
				     gboolean                history);
gchar    *hg_line_edit_get_statement(HgLineEdit             *lineedit,
                                     const gchar            *prompt);
gboolean  hg_line_edit_load_history (HgLineEdit             *lineedit,
				     const gchar            *filename);
gboolean  hg_line_edit_save_history (HgLineEdit             *lineedit,
				     const gchar            *filename);
void      hg_line_edit_set_stdin    (HgLineEdit             *lineedit,
				     HgFileObject           *stdin);
void      hg_line_edit_set_stdout   (HgLineEdit             *lineedit,
				     HgFileObject           *stdout);


G_END_DECLS


#endif /* __HG_LINEEDIT_H__ */
