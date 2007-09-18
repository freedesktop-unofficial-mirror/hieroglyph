/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.h
 * Copyright (C) 2007 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGSTRING_H__
#define __HIEROGLYPH_HGSTRING_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS

hg_object_t *hg_object_string_sized_new    (hg_vm_t     *vm,
					    guint16      length);
hg_object_t *hg_object_string_new          (hg_vm_t     *vm,
					    const gchar *value);
hg_object_t *hg_object_string_substring_new(hg_vm_t     *vm,
					    hg_object_t *object,
					    guint16      start_index,
					    guint16      length);
gboolean     hg_object_string_compare      (hg_object_t *object1,
					    hg_object_t *object2);

G_END_DECLS

#endif /* __HIEROGLYPH_HGSTRING_H__ */
