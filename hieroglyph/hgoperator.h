/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator.h
 * Copyright (C) 2005-2007 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGOPERATOR_H__
#define __HIEROGLYPH_HGOPERATOR_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS

hg_object_t *hg_object_operator_new            (hg_vm_t              *vm,
                                                hg_system_encoding_t  enc,
						hg_operator_func_t    func) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_operator_new_with_custom(hg_vm_t              *vm,
                                                gchar                *name,
						hg_operator_func_t    func) G_GNUC_WARN_UNUSED_RESULT;
gboolean     hg_object_operator_initialize     (hg_vm_t              *vm,
						hg_emulation_type_t   level);
gboolean     hg_object_operator_finalize       (hg_vm_t              *vm,
						hg_emulation_type_t   level);
gboolean     hg_object_operator_invoke         (hg_vm_t              *vm,
						hg_object_t          *object);
gboolean     hg_object_operator_compare        (hg_object_t          *object1,
                                                hg_object_t          *object2);
gchar       *hg_object_operator_dump           (hg_object_t          *object,
                                                gboolean              verbose) G_GNUC_MALLOC;
const gchar *hg_object_operator_get_name       (hg_system_encoding_t  encoding) G_GNUC_CONST;

G_END_DECLS

#endif /* __HIEROGLYPH_HGOPERATOR_H__ */
