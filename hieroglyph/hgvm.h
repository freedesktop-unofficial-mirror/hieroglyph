/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgvm.h
 * Copyright (C) 2005-2011 Akira TAGOH
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGVM_H__
#define __HIEROGLYPH_HGVM_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgdevice.h>
#include <hieroglyph/hgerror.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hglineedit.h>
#include <hieroglyph/hgscanner.h>
#include <hieroglyph/hgstack.h>

HG_BEGIN_DECLS

#define HG_VM_LOCK(_v_,_q_)						\
	(hg_vm_quark_lock_object((_v_),(_q_),__PRETTY_FUNCTION__))
#define HG_VM_UNLOCK(_v_,_q_)				\
	(hg_vm_quark_unlock_object((_v_),(_q_)))

typedef enum _hg_vm_mem_type_t		hg_vm_mem_type_t;
typedef struct _hg_vm_state_t		hg_vm_state_t;
typedef enum _hg_vm_langlevel_t		hg_vm_langlevel_t;
typedef enum _hg_vm_trans_flags_t	hg_vm_trans_flags_t;
typedef struct _hg_vm_user_params_t	hg_vm_user_params_t;
typedef struct _hg_vm_sys_params_t	hg_vm_sys_params_t;
typedef struct _hg_vm_value_t		hg_vm_value_t;
typedef enum _hg_vm_stack_type_t	hg_vm_stack_type_t;
typedef enum _hg_vm_user_params_name_t	hg_vm_user_params_name_t;
typedef enum _hg_vm_sys_params_name_t	hg_vm_sys_params_name_t;

enum _hg_vm_stack_type_t {
	HG_VM_STACK_OSTACK = 0,
	HG_VM_STACK_ESTACK,
	HG_VM_STACK_DSTACK,
	HG_VM_STACK_GSTATE,
	HG_VM_STACK_END
};
enum _hg_vm_user_params_name_t {
	HG_VM_user_BEGIN = 0,
	HG_VM_user_MaxOpStack,
	HG_VM_user_MaxExecStack,
	HG_VM_user_MaxDictStack,
	HG_VM_user_MaxGStateStack,
	HG_VM_user_END
};
enum _hg_vm_sys_params_name_t {
	HG_VM_sys_BEGIN = 0,
	HG_VM_sys_END
};
enum _hg_vm_mem_type_t {
	HG_VM_MEM_GLOBAL = 0,
	HG_VM_MEM_LOCAL = 1,
	HG_VM_MEM_END
};
enum _hg_vm_langlevel_t {
	HG_LANG_LEVEL_1 = 0,
	HG_LANG_LEVEL_2,
	HG_LANG_LEVEL_3,
	HG_LANG_LEVEL_END
};
enum _hg_vm_trans_flags_t {
	HG_TRANS_FLAG_NONE = 0,
	HG_TRANS_FLAG_TOKEN = (1 << 0),
	HG_TRANS_FLAG_BLOCK = (1 << 1),	/* For internal */
	HG_TRANS_FLAG_END
};
struct _hg_vm_user_params_t {
	hg_usize_t max_op_stack;
	hg_usize_t max_exec_stack;
	hg_usize_t max_dict_stack;
	hg_usize_t max_gstate_stack;
};
struct _hg_vm_sys_params_t {
};
struct _hg_vm_state_t {
	hg_quark_t       self;
	hg_vm_mem_type_t current_mem_index;
	hg_int_t         n_save_objects;
};
struct _hg_vm_t {
	hg_quark_t           self;
	hg_vm_state_t       *vm_state;
	hg_vm_user_params_t  user_params;
	hg_mem_t            *mem[HG_VM_MEM_END];
	hg_int_t             mem_id[HG_VM_MEM_END];
	hg_quark_t           qio[HG_FILE_IO_END];
	hg_stack_spool_t    *stack_spooler;
	hg_stack_t          *stacks[HG_VM_STACK_END];
	GPtrArray           *stacks_stack;
	hg_quark_t           qerror_name[HG_e_END];
	hg_quark_t           quparams_name[HG_VM_user_END];
	hg_quark_t           qsparams_name[HG_VM_sys_END];
	hg_quark_t           qpdevparams_name[HG_pdev_END];
	hg_quark_t           qerror;
	hg_quark_t           qsystemdict;
	hg_quark_t           qglobaldict;
	hg_quark_t           qinternaldict;
	hg_quark_t           qgstate;
	hg_scanner_t        *scanner;
	hg_lineedit_t       *lineedit;
	hg_vm_langlevel_t    language_level;
	hg_bool_t            is_initialized:1;
	hg_bool_t            hold_lang_level:1;
	hg_bool_t            shutdown:1;
	hg_bool_t            has_error:1;
	hg_int_t             error_code;
	hg_uint_t            n_nest_scan;
	hg_quark_acl_t       default_acl;
	GHashTable          *params;
	GHashTable          *plugin_table;
	GList               *plugin_list;
	GRand               *rand_;
	hg_uint_t            rand_seed;
	GTimeVal             initiation_time;
	hg_device_t         *device;
};
struct _hg_vm_value_t {
	hg_int_t type;
	union {
		hg_bool_t  bool;
		hg_int_t   integer;
		hg_real_t  real;
		hg_char_t *string;
	} u;
};



/* hg_vm_t */
hg_bool_t          hg_vm_init               (void);
void               hg_vm_tini               (void);
hg_vm_t           *hg_vm_new                (void);
void               hg_vm_destroy            (hg_vm_t                *vm);
hg_quark_t         hg_vm_malloc             (hg_vm_t                *vm,
                                             hg_usize_t              size,
                                             hg_pointer_t           *ret);
hg_quark_t         hg_vm_realloc            (hg_vm_t                *vm,
                                             hg_quark_t              qdata,
                                             hg_usize_t              size,
                                             hg_pointer_t           *ret);
void               hg_vm_set_default_acl    (hg_vm_t                *vm,
                                             hg_quark_acl_t          acl);
hg_mem_t          *hg_vm_get_mem            (hg_vm_t                *vm);
void               hg_vm_use_global_mem     (hg_vm_t                *vm,
                                             hg_bool_t               flag);
hg_bool_t          hg_vm_is_global_mem_used (hg_vm_t                *vm);
hg_quark_t         hg_vm_get_io             (hg_vm_t                *vm,
                                             hg_file_io_t            type);
void               hg_vm_set_io             (hg_vm_t                *vm,
                                             hg_file_io_t            type,
                                             hg_quark_t              file);
hg_bool_t          hg_vm_setup              (hg_vm_t                *vm,
                                             hg_vm_langlevel_t       lang_level,
                                             hg_quark_t              stdin,
                                             hg_quark_t              stdout,
                                             hg_quark_t              stderr);
void               hg_vm_finish             (hg_vm_t                *vm);
hg_vm_langlevel_t  hg_vm_get_language_level (hg_vm_t                *vm);
hg_bool_t          hg_vm_set_language_level (hg_vm_t                *vm,
                                             hg_vm_langlevel_t       level);
void               hg_vm_hold_language_level(hg_vm_t                *vm,
                                             hg_bool_t               flag);
hg_bool_t          hg_vm_translate          (hg_vm_t                *vm,
					     hg_vm_trans_flags_t     flags);
hg_bool_t          hg_vm_main_loop          (hg_vm_t                *vm);
hg_bool_t          hg_vm_eval_cstring       (hg_vm_t                *vm,
                                             const hg_char_t        *cstring,
                                             hg_size_t               clen,
                                             hg_bool_t               protect_systemdict);
hg_bool_t          hg_vm_eval_file          (hg_vm_t                *vm,
                                             const hg_char_t        *initfile,
                                             hg_bool_t               protect_systemdict);
hg_int_t           hg_vm_get_error_code     (hg_vm_t                *vm);
void               hg_vm_set_error_code     (hg_vm_t                *vm,
                                             hg_int_t                error_code);
hg_bool_t          hg_vm_startjob           (hg_vm_t                *vm,
                                             hg_vm_langlevel_t       lang_level,
                                             const hg_char_t        *initfile,
                                             hg_bool_t               encapsulated);
void               hg_vm_shutdown           (hg_vm_t                *vm,
                                             hg_int_t                error_code);
hg_bool_t          hg_vm_is_finished        (hg_vm_t                *vm);
hg_quark_t         hg_vm_get_gstate         (hg_vm_t                *vm);
void               hg_vm_set_gstate         (hg_vm_t                *vm,
                                             hg_quark_t              qgstate);
hg_quark_t         hg_vm_get_user_params    (hg_vm_t                *vm,
                                             hg_pointer_t           *ret);
hg_bool_t          hg_vm_set_user_params    (hg_vm_t                *vm,
                                             hg_quark_t              qdict);
void               hg_vm_set_rand_seed      (hg_vm_t                *vm,
                                             hg_uint_t               seed);
hg_uint_t          hg_vm_get_rand_seed      (hg_vm_t                *vm);
hg_uint_t          hg_vm_rand_int           (hg_vm_t                *vm);
hg_uint_t          hg_vm_get_current_time   (hg_vm_t                *vm);
hg_bool_t          hg_vm_has_error          (hg_vm_t                *vm);
void               hg_vm_clear_error        (hg_vm_t                *vm);
void               hg_vm_reset_error        (hg_vm_t                *vm);
void               hg_vm_reserved_spool_dump(hg_vm_t                *vm,
                                             hg_mem_t               *mem,
                                             hg_file_t              *ofile);
hg_bool_t          hg_vm_add_plugin         (hg_vm_t                *vm,
                                             const hg_char_t        *name);
void               hg_vm_load_plugins       (hg_vm_t                *vm);
hg_bool_t          hg_vm_remove_plugin      (hg_vm_t                *vm,
                                             const hg_char_t        *name);
void               hg_vm_unload_plugins     (hg_vm_t                *vm);
hg_bool_t          hg_vm_set_device         (hg_vm_t                *vm,
                                             const hg_char_t        *name);
void               hg_vm_add_param          (hg_vm_t                *vm,
                                             const hg_char_t        *name,
                                             hg_vm_value_t          *value);

/* hg_array_t */
hg_bool_t  hg_vm_array_set(hg_vm_t    *vm,
                           hg_quark_t  qarray,
                           hg_quark_t  qdata,
                           hg_usize_t  index,
                           hg_bool_t   force);
hg_quark_t hg_vm_array_get(hg_vm_t    *vm,
                           hg_quark_t  qarray,
                           hg_usize_t  index,
                           hg_bool_t   force);

/* hg_dict_t */
hg_quark_t hg_vm_dstack_lookup  (hg_vm_t    *vm,
                                 hg_quark_t  qname);
hg_bool_t  hg_vm_dstack_remove  (hg_vm_t    *vm,
                                 hg_quark_t  qname,
                                 hg_bool_t   remove_all);
hg_quark_t hg_vm_dstack_get_dict(hg_vm_t    *vm);
hg_bool_t  hg_vm_dict_add       (hg_vm_t    *vm,
                                 hg_quark_t  qdict,
                                 hg_quark_t  qkey,
                                 hg_quark_t  qval,
                                 hg_bool_t   force);
hg_bool_t  hg_vm_dict_remove    (hg_vm_t    *vm,
                                 hg_quark_t  qdict,
                                 hg_quark_t  qkey);
hg_quark_t hg_vm_dict_lookup    (hg_vm_t    *vm,
                                 hg_quark_t  qdict,
                                 hg_quark_t  qkey,
                                 hg_bool_t   check_perms);

/* hg_quark_t */
hg_mem_t     *hg_vm_quark_get_mem        (hg_vm_t         *vm,
                                          hg_quark_t       qdata);
hg_pointer_t  hg_vm_quark_lock_object    (hg_vm_t         *vm,
                                          hg_quark_t       qdata,
                                          const hg_char_t *pretty_function);
void          hg_vm_quark_unlock_object  (hg_vm_t         *vm,
                                          hg_quark_t       qdata);
hg_bool_t     hg_vm_quark_gc_mark        (hg_vm_t         *vm,
                                          hg_quark_t       qdata);
hg_quark_t    hg_vm_quark_copy           (hg_vm_t         *vm,
                                          hg_quark_t       qdata,
                                          hg_pointer_t    *ret);
hg_quark_t    hg_vm_quark_to_string      (hg_vm_t         *vm,
                                          hg_quark_t       qdata,
                                          hg_bool_t        ps_like_syntax,
                                          hg_pointer_t    *ret);
hg_bool_t     hg_vm_quark_compare        (hg_vm_t         *vm,
                                          hg_quark_t       qdata1,
                                          hg_quark_t       qdata2);
hg_bool_t     hg_vm_quark_compare_content(hg_vm_t         *vm,
                                          hg_quark_t       qdata1,
                                          hg_quark_t       qdata2);
void          hg_vm_quark_set_default_acl(hg_vm_t         *vm,
                                          hg_quark_t      *qdata);
void          hg_vm_quark_set_acl        (hg_vm_t         *vm,
                                          hg_quark_t      *qdata,
                                          hg_quark_acl_t   acl);
void          hg_vm_quark_set_readable   (hg_vm_t         *vm,
                                          hg_quark_t      *qdata,
                                          hg_bool_t        flag);
hg_bool_t     hg_vm_quark_is_readable    (hg_vm_t         *vm,
                                          hg_quark_t      *qdata);
void          hg_vm_quark_set_writable   (hg_vm_t         *vm,
                                          hg_quark_t      *qdata,
                                          hg_bool_t        flag);
hg_bool_t     hg_vm_quark_is_writable    (hg_vm_t         *vm,
                                          hg_quark_t      *qdata);
void          hg_vm_quark_set_executable (hg_vm_t         *vm,
                                          hg_quark_t      *qdata,
                                          hg_bool_t        flag);
hg_bool_t     hg_vm_quark_is_executable  (hg_vm_t         *vm,
                                          hg_quark_t      *qdata);
void          hg_vm_quark_set_accessible (hg_vm_t         *vm,
                                          hg_quark_t      *qdata,
                                          hg_bool_t        flag);
hg_bool_t     hg_vm_quark_is_accessible  (hg_vm_t         *vm,
                                          hg_quark_t      *qdata);

/* hg_snapshot_t */
hg_error_t hg_vm_snapshot_save   (hg_vm_t    *vm);
hg_error_t hg_vm_snapshot_restore(hg_vm_t    *vm,
				  hg_quark_t  qsnapshot);

/* hg_stack_t */
hg_stack_t *hg_vm_stack_new (hg_vm_t    *vm,
                             hg_usize_t  size);
void        hg_vm_stack_dump(hg_vm_t    *vm,
                             hg_stack_t *stack,
                             hg_file_t  *output);

/* hg_string_t */
hg_char_t *hg_vm_string_get_cstr(hg_vm_t    *vm,
                                 hg_quark_t  qstring);
hg_uint_t  hg_vm_string_length  (hg_vm_t    *vm,
                                 hg_quark_t  qstring);

/* hg_vm_value_t */
void           hg_vm_value_free       (hg_pointer_t     data);
hg_vm_value_t *hg_vm_value_boolean_new(hg_bool_t        value);
hg_vm_value_t *hg_vm_value_integer_new(hg_int_t         value);
hg_vm_value_t *hg_vm_value_real_new   (hg_real_t        value);
hg_vm_value_t *hg_vm_value_string_new (const hg_char_t *value);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGVM_H__ */
