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

G_BEGIN_DECLS

#define HG_LOCAL_POOL_SIZE	240000 * 100
#define HG_GLOBAL_POOL_SIZE	240000 * 100
#define HG_GRAPHIC_POOL_SIZE	240000 * 50


typedef enum {
	VM_EMULATION_LEVEL_1 = 1,
	VM_EMULATION_LEVEL_2,
	VM_EMULATION_LEVEL_3
} HgVMEmulationType;

typedef enum {
	VM_IO_STDIN = 1,
	VM_IO_STDOUT,
	VM_IO_STDERR,
} HgVMIOType;

typedef enum {
	VM_e_dictfull = 1,
	VM_e_dictstackoverflow,
	VM_e_dictstackunderflow,
	VM_e_execstackoverflow,
	VM_e_handleerror,
	VM_e_interrupt,
	VM_e_invalidaccess,
	VM_e_invalidexit,
	VM_e_invalidfileaccess,
	VM_e_invalidfont,
	VM_e_invalidrestore,
	VM_e_ioerror,
	VM_e_limitcheck,
	VM_e_nocurrentpoint,
	VM_e_rangecheck,
	VM_e_stackoverflow,
	VM_e_stackunderflow,
	VM_e_syntaxerror,
	VM_e_timeout,
	VM_e_typecheck,
	VM_e_undefined,
	VM_e_undefinedfilename,
	VM_e_undefinedresult,
	VM_e_unmatchedmark,
	VM_e_unregistered,
	VM_e_VMerror,
	VM_e_configurationerror,
	VM_e_undefinedresource,
	VM_e_END,
} HgVMError;


/* initializer */
void     hg_vm_init          (void);
void     hg_vm_finalize      (void);
gboolean hg_vm_is_initialized(void);

/* virtual machine */
HgVM                 *hg_vm_new                  (HgVMEmulationType  type);
void                  hg_vm_set_emulation_level  (HgVM              *vm,
						  HgVMEmulationType  type);
HgVMEmulationType     hg_vm_get_emulation_level  (HgVM              *vm);
HgStack              *hg_vm_get_ostack           (HgVM              *vm);
HgStack              *hg_vm_get_estack           (HgVM              *vm);
void                  hg_vm_set_estack           (HgVM              *vm,
						  HgStack           *estack);
HgStack              *hg_vm_get_dstack           (HgVM              *vm);
HgDict               *hg_vm_get_dict_errordict   (HgVM              *vm);
HgDict               *hg_vm_get_dict_error       (HgVM              *vm);
HgDict               *hg_vm_get_dict_statusdict  (HgVM              *vm);
HgDict               *hg_vm_get_dict_serverdict  (HgVM              *vm);
HgDict               *hg_vm_get_dict_font        (HgVM              *vm);
HgDict               *hg_vm_get_dict_systemdict  (HgVM              *vm);
HgDict               *hg_vm_get_dict_globalfont  (HgVM              *vm);
HgMemPool            *hg_vm_get_current_pool     (HgVM              *vm);
gboolean              hg_vm_is_global_pool_used  (HgVM              *vm);
void                  hg_vm_use_global_pool      (HgVM              *vm,
						  gboolean           use_global);
HgGraphics           *hg_vm_get_graphics         (HgVM              *vm);
gint32                hg_vm_get_current_time     (HgVM              *vm);
GRand                *hg_vm_get_random_generator (HgVM              *vm);
HgFileObject         *hg_vm_get_io               (HgVM              *vm,
						  HgVMIOType         type);
void                  hg_vm_set_io               (HgVM              *vm,
						  HgVMIOType         type,
						  HgFileObject      *file);
HgLineEdit           *hg_vm_get_line_editor      (HgVM              *vm);
void                  hg_vm_set_line_editor      (HgVM              *vm,
						  HgLineEdit        *editor);
guint                 hg_vm_get_save_level       (HgVM              *vm);
HgValueNode          *hg_vm_get_name_node        (HgVM              *vm,
						  const gchar       *name);
HgValueNode          *hg_vm_lookup               (HgVM              *vm,
						  HgValueNode       *key);
HgValueNode          *hg_vm_lookup_with_string   (HgVM              *vm,
						  const gchar       *key);
gboolean              hg_vm_startjob             (HgVM              *vm,
						  const gchar       *initializer,
						  gboolean           encapsulated);
gboolean              hg_vm_has_error            (HgVM              *vm);
void                  hg_vm_clear_error          (HgVM              *vm);
void                  hg_vm_reset_error          (HgVM              *vm);
void                  hg_vm_set_error            (HgVM              *vm,
						  HgValueNode       *errnode,
						  HgVMError          error,
						  gboolean           drop_self);
void                  hg_vm_set_error_from_file  (HgVM              *vm,
						  HgValueNode       *errnode,
						  HgFileObject      *file,
						  gboolean           drop_self);
gboolean              hg_vm_main                 (HgVM              *vm);
gboolean              hg_vm_run                  (HgVM              *vm,
						  const gchar       *file);


/* internal use */
HgValueNode *_hg_vm_get_name_node(HgVM        *vm,
				  const gchar *name);

G_END_DECLS

#endif /* __HG_VM_H__ */
