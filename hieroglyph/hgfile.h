/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.h
 * Copyright (C) 2006-2007 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGFILE_H__
#define __HIEROGLYPH_HGFILE_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS

hg_object_t *hg_object_file_new            (hg_vm_t        *vm,
					    const gchar    *filename,
					    hg_filemode_t   mode) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_file_new_from_string(hg_vm_t        *vm,
					    hg_object_t    *string,
					    hg_filemode_t   mode) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_file_new_with_custom(hg_vm_t        *vm,
					    hg_filetable_t *table,
					    hg_filemode_t   mode) G_GNUC_WARN_UNUSED_RESULT;
void         hg_object_file_free           (hg_vm_t        *vm,
					    hg_object_t    *object);
void         hg_object_file_notify_error   (hg_vm_t        *vm,
					    error_t         _errno);
gboolean     hg_object_file_compare        (hg_object_t    *object1,
					    hg_object_t    *object2);
gchar       *hg_object_file_dump           (hg_object_t    *object,
					    gboolean        verbose) G_GNUC_MALLOC;


G_END_DECLS

#endif /* __HIEROGLYPH_HGFILE_H__ */
