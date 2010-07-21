/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgvm.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGVM_H__
#define __HIEROGLYPH_HGVM_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgscanner.h>
#include <hieroglyph/hgstack.h>

G_BEGIN_DECLS

#define HG_VM_LOCK(_v_,_q_,_e_)						\
	(hg_vm_lock_object((_v_),(_q_),__PRETTY_FUNCTION__,(_e_)))
#define HG_VM_UNLOCK(_v_,_q_)			\
	(hg_vm_unlock_object((_v_),(_q_)))

typedef enum _hg_vm_mem_type_t		hg_vm_mem_type_t;
typedef enum _hg_vm_stack_type_t	hg_vm_stack_type_t;
typedef enum _hg_vm_error_t		hg_vm_error_t;

enum _hg_vm_mem_type_t {
	HG_VM_MEM_GLOBAL = 0,
	HG_VM_MEM_LOCAL = 1,
	HG_VM_MEM_END
};
enum _hg_vm_stack_type_t {
	HG_VM_STACK_OSTACK = 0,
	HG_VM_STACK_ESTACK,
	HG_VM_STACK_DSTACK,
	HG_VM_STACK_END
};
enum _hg_vm_error_t {
	HG_VM_e_dictfull = 1,
	HG_VM_e_dictstackoverflow,
	HG_VM_e_dictstackunderflow,
	HG_VM_e_execstackoverflow,
	HG_VM_e_handleerror,
	HG_VM_e_interrupt,
	HG_VM_e_invalidaccess,
	HG_VM_e_invalidexit,
	HG_VM_e_invalidfileaccess,
	HG_VM_e_invalidfont,
	HG_VM_e_invalidrestore,
	HG_VM_e_ioerror,
	HG_VM_e_limitcheck,
	HG_VM_e_nocurrentpoint,
	HG_VM_e_rangecheck,
	HG_VM_e_stackoverflow,
	HG_VM_e_stackunderflow,
	HG_VM_e_syntaxerror,
	HG_VM_e_timeout,
	HG_VM_e_typecheck,
	HG_VM_e_undefined,
	HG_VM_e_undefinedfilename,
	HG_VM_e_undefinedresult,
	HG_VM_e_unmatchedmark,
	HG_VM_e_unregistered,
	HG_VM_e_VMerror,
	HG_VM_e_configurationerror,
	HG_VM_e_undefinedresource,
	HG_VM_e_END
};
struct _hg_vm_t {
	hg_quark_t         self;
	hg_name_t         *name;
	hg_vm_mem_type_t   current_mem_index;
	hg_mem_t          *mem[HG_VM_MEM_END];
	gint               mem_id[HG_VM_MEM_END];
	hg_quark_t         qio[HG_FILE_IO_END];
	hg_stack_t        *stacks[HG_VM_STACK_END];
	hg_quark_t         qerror_name[HG_VM_e_END];
	hg_quark_t         qerror;
	hg_quark_t         qsystemdict;
	hg_quark_t         qglobaldict;
	hg_scanner_t      *scanner;
	hg_vm_langlevel_t  language_level;
	gboolean           is_initialized:1;
	gboolean           shutdown:1;
	gboolean           has_error:1;
	gint               error_code;
};


gboolean           hg_vm_init              (void);
void               hg_vm_tini              (void);
hg_vm_t           *hg_vm_new               (void);
void               hg_vm_destroy           (hg_vm_t           *vm);
hg_quark_t         hg_vm_malloc            (hg_vm_t           *vm,
					    gsize              size,
					    gpointer          *ret);
hg_quark_t         hg_vm_realloc           (hg_vm_t           *vm,
					    hg_quark_t         qdata,
					    gsize              size,
					    gpointer          *ret);
void               hg_vm_mfree             (hg_vm_t           *vm,
					    hg_quark_t         qdata);
gpointer           hg_vm_lock_object       (hg_vm_t           *vm,
					    hg_quark_t         qdata,
					    const gchar       *pretty_function,
					    GError           **error);
void               hg_vm_unlock_object     (hg_vm_t           *vm,
					    hg_quark_t         qdata);
hg_quark_t         hg_vm_quark_copy        (hg_vm_t           *vm,
					    hg_quark_t         qdata,
					    gpointer          *ret);
hg_quark_t         hg_vm_quark_to_string   (hg_vm_t           *vm,
					    hg_quark_t         qdata,
					    gpointer          *ret);
hg_mem_t          *hg_vm_get_mem           (hg_vm_t           *vm);
void               hg_vm_use_global_mem    (hg_vm_t           *vm,
					    gboolean           flag);
gboolean           hg_vm_is_global_mem_used(hg_vm_t           *vm);
hg_quark_t         hg_vm_get_io            (hg_vm_t           *vm,
					    hg_file_io_t       type);
void               hg_vm_set_io            (hg_vm_t           *vm,
					    hg_file_io_t       type,
					    hg_quark_t         file);
gboolean           hg_vm_setup             (hg_vm_t           *vm,
					    hg_vm_langlevel_t  lang_level,
					    hg_quark_t         stdin,
					    hg_quark_t         stdout,
					    hg_quark_t         stderr);
void               hg_vm_finish            (hg_vm_t           *vm);
hg_vm_langlevel_t  hg_vm_get_language_level(hg_vm_t           *vm);
gboolean           hg_vm_set_language_level(hg_vm_t           *vm,
					    hg_vm_langlevel_t  level);
hg_quark_t         hg_vm_dict_lookup       (hg_vm_t           *vm,
					    hg_quark_t         qname);
gboolean           hg_vm_dict_remove       (hg_vm_t           *vm,
					    hg_quark_t         qname,
					    gboolean           remove_all);
hg_quark_t         hg_vm_get_dict          (hg_vm_t           *vm);
gboolean           hg_vm_stepi             (hg_vm_t           *vm,
					    gboolean          *is_proceeded);
gboolean           hg_vm_step              (hg_vm_t           *vm);
gboolean           hg_vm_main_loop         (hg_vm_t           *vm);
gboolean           hg_vm_eval              (hg_vm_t           *vm,
					    hg_quark_t         qeval,
					    hg_stack_t        *ostack,
					    hg_stack_t        *estack,
					    hg_stack_t        *dstack,
					    GError           **error);
gboolean           hg_vm_eval_from_cstring (hg_vm_t           *vm,
					    const gchar       *cstring,
					    hg_stack_t        *ostack,
					    hg_stack_t        *estack,
					    hg_stack_t        *dstack,
					    GError           **error);
gboolean           hg_vm_eval_from_file    (hg_vm_t           *vm,
					    const gchar       *initfile,
					    hg_stack_t        *ostack,
					    hg_stack_t        *estack,
					    hg_stack_t        *dstack,
					    GError           **error);
gint               hg_vm_get_error_code    (hg_vm_t           *vm);
void               hg_vm_set_error_code    (hg_vm_t           *vm,
					    gint               error_code);
gboolean           hg_vm_startjob          (hg_vm_t           *vm,
					    hg_vm_langlevel_t  lang_level,
					    const gchar       *initfile,
					    gboolean           encapsulated);
void               hg_vm_shutdown          (hg_vm_t           *vm,
					    gint               error_code);
gboolean           hg_vm_set_error         (hg_vm_t           *vm,
					    hg_quark_t         qdata,
					    hg_vm_error_t      error);

G_END_DECLS

#endif /* __HIEROGLYPH_HGVM_H__ */