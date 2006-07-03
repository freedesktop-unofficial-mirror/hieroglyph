/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * vm.h
 * Copyright (C) 2005-2006 Akira TAGOH
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
#ifndef __LIBRETTO_VM_H__
#define __LIBRETTO_VM_H__

#include <hieroglyph/hgtypes.h>
#include <libretto/lbtypes.h>

G_BEGIN_DECLS

#define LB_LOCAL_POOL_SIZE	240000 * 100
#define LB_GLOBAL_POOL_SIZE	240000 * 100
#define LB_GRAPHIC_POOL_SIZE	240000 * 50


/* initializer */
void     libretto_vm_init          (void);
void     libretto_vm_finalize      (void);
gboolean libretto_vm_is_initialized(void);

/* virtual machine */
LibrettoVM           *libretto_vm_new                (LibrettoEmulationType  type);
void                  libretto_vm_set_emulation_level(LibrettoVM            *vm,
						      LibrettoEmulationType  type);
LibrettoEmulationType libretto_vm_get_emulation_level(LibrettoVM            *vm);
HgStack              *libretto_vm_get_ostack         (LibrettoVM            *vm);
HgStack              *libretto_vm_get_estack         (LibrettoVM            *vm);
void                  libretto_vm_set_estack         (LibrettoVM            *vm,
						      HgStack               *estack);
HgStack              *libretto_vm_get_dstack         (LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_errordict (LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_error     (LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_statusdict(LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_serverdict(LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_font      (LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_systemdict(LibrettoVM            *vm);
HgDict               *libretto_vm_get_dict_globalfont(LibrettoVM            *vm);
HgMemPool            *libretto_vm_get_current_pool   (LibrettoVM            *vm);
gboolean              libretto_vm_is_global_pool_used(LibrettoVM            *vm);
void                  libretto_vm_use_global_pool    (LibrettoVM            *vm,
						      gboolean               use_global);
LibrettoGraphics    *libretto_vm_get_graphics        (LibrettoVM            *vm);
gint32               libretto_vm_get_current_time    (LibrettoVM            *vm);
GRand               *libretto_vm_get_random_generator(LibrettoVM            *vm);
HgFileObject        *libretto_vm_get_io              (LibrettoVM            *vm,
						      LibrettoIOType         type);
void                 libretto_vm_set_io              (LibrettoVM            *vm,
						      LibrettoIOType         type,
						      HgFileObject          *file);
guint                libretto_vm_get_save_level      (LibrettoVM            *vm);
HgValueNode          *libretto_vm_get_name_node      (LibrettoVM            *vm,
						      const gchar           *name);
HgValueNode          *libretto_vm_lookup             (LibrettoVM            *vm,
						      HgValueNode           *key);
HgValueNode          *libretto_vm_lookup_with_string (LibrettoVM            *vm,
						      const gchar           *key);
gboolean              libretto_vm_startjob           (LibrettoVM            *vm,
						      const gchar           *initializer,
						      gboolean               encapsulated);
gboolean              libretto_vm_has_error          (LibrettoVM            *vm);
void                  libretto_vm_clear_error        (LibrettoVM            *vm);
void                  libretto_vm_reset_error        (LibrettoVM            *vm);
void                  libretto_vm_set_error          (LibrettoVM            *vm,
						      HgValueNode           *errnode,
						      LibrettoVMError        error,
						      gboolean               drop_self);
void                  libretto_vm_set_error_from_file(LibrettoVM            *vm,
						      HgValueNode           *errnode,
						      HgFileObject          *file,
						      gboolean               drop_self);
gboolean              libretto_vm_main               (LibrettoVM            *vm);
gboolean              libretto_vm_run                (LibrettoVM            *vm,
						      const gchar           *file);


/* internal use */
HgValueNode *_libretto_vm_get_name_node(LibrettoVM       *vm,
					const gchar      *name);

G_END_DECLS

#endif /* __LIBRETTO_VM_H__ */
