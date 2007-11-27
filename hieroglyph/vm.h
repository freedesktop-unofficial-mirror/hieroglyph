/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * vm.h
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
#ifndef __HIEROGLYPH__VM_H__
#define __HIEROGLYPH__VM_H__

#include <hieroglyph/hgtypes.h>


hg_vm_t             *hg_vm_new                       (void) G_GNUC_WARN_UNUSED_RESULT;
void                 hg_vm_destroy                   (hg_vm_t            *vm);
gpointer             hg_vm_malloc                    (hg_vm_t            *vm,
						      gsize               size) G_GNUC_WARN_UNUSED_RESULT;
gpointer             hg_vm_realloc                   (hg_vm_t            *vm,
						      gpointer            object,
						      gsize               size) G_GNUC_WARN_UNUSED_RESULT;
void                hg_vm_mfree                      (hg_vm_t            *vm,
                                                      gpointer            data);
guchar              hg_vm_get_object_format          (hg_vm_t            *vm);
gboolean            hg_vm_get_current_allocation_mode(hg_vm_t            *vm);
void                hg_vm_set_current_allocation_mode(hg_vm_t            *vm,
                                                      gboolean            is_global);
void                hg_vm_get_attributes             (hg_vm_t            *vm,
                                                      hg_attribute_t     *attr);
void                hg_vm_set_error                  (hg_vm_t            *vm,
                                                      hg_error_t          error);
hg_error_t          hg_vm_get_error                  (hg_vm_t            *vm);
void                hg_vm_clear_error                (hg_vm_t            *vm);
hg_object_t        *hg_vm_get_io                     (hg_vm_t            *vm,
                                                      hg_filetype_t       iotype);
void                hg_vm_set_io                     (hg_vm_t            *vm,
                                                      hg_object_t        *object);
gboolean            hg_vm_initialize                 (hg_vm_t            *vm);
gboolean            hg_vm_finalize                   (hg_vm_t            *vm);
hg_emulationtype_t  hg_vm_get_emulation_level        (hg_vm_t            *vm);
gboolean            hg_vm_set_emulation_level        (hg_vm_t            *vm,
                                                      hg_emulationtype_t  level);
gboolean            hg_vm_stepi                      (hg_vm_t            *vm,
                                                      gboolean           *is_proceeded);
gboolean            hg_vm_step                       (hg_vm_t            *vm);
hg_object_t        *hg_vm_get_dict                   (hg_vm_t            *vm);
hg_object_t        *hg_vm_dict_lookup                (hg_vm_t            *vm,
                                                      hg_object_t        *object);
gboolean            hg_vm_dict_remove                (hg_vm_t            *vm,
                                                      const gchar        *name,
                                                      gboolean            remove_all);
hg_object_t        *hg_vm_get_currentdict            (hg_vm_t            *vm);
hg_object_t         *hg_vm_name_lookup               (hg_vm_t            *vm,
						      const gchar        *name) G_GNUC_WARN_UNUSED_RESULT;


#endif /* __HIEROGLYPH__VM_H__ */
