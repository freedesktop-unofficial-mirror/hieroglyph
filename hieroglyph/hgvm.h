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
#include <hieroglyph/hglineedit.h>
#include <hieroglyph/hgscanner.h>
#include <hieroglyph/hgstack.h>

G_BEGIN_DECLS

#define HG_VM_LOCK(_v_,_q_,_e_)						\
	(hg_vm_lock_object((_v_),(_q_),__PRETTY_FUNCTION__,(_e_)))
#define HG_VM_UNLOCK(_v_,_q_)			\
	(hg_vm_unlock_object((_v_),(_q_)))
#define HG_VM_ATTRIBUTE(_a_)			\
	((_a_) << HG_QUARK_TYPE_BIT_ACCESS1)
#define HG_VM_ATTRIBUTE1(_a_)			\
	((_a_) >> 1)

typedef enum _hg_vm_access_t		hg_vm_access_t;
typedef enum _hg_vm_stack_type_t	hg_vm_stack_type_t;
typedef enum _hg_vm_user_params_name_t	hg_vm_user_params_name_t;
typedef enum _hg_vm_sys_params_name_t	hg_vm_sys_params_name_t;
typedef enum _hg_vm_pdev_params_name_t	hg_vm_pdev_params_name_t;

enum _hg_vm_access_t {
	HG_VM_ACCESS_EXECUTABLE = HG_ACCESS_EXECUTABLE,
	HG_VM_ACCESS_READABLE   = HG_ACCESS_READABLE,
	HG_VM_ACCESS_WRITABLE   = HG_ACCESS_WRITABLE,
	HG_VM_ACCESS_EDITABLE   = HG_ACCESS_EDITABLE,
	HG_VM_ACCESS_END
};
enum _hg_vm_stack_type_t {
	HG_VM_STACK_OSTACK = 0,
	HG_VM_STACK_ESTACK,
	HG_VM_STACK_DSTACK,
	HG_VM_STACK_GSTATE,
	HG_VM_STACK_END
};
enum _hg_vm_user_params_name_t {
	HG_VM_user_MaxOpStack = 0,
	HG_VM_user_MaxExecStack,
	HG_VM_user_MaxDictStack,
	HG_VM_user_MaxGStateStack,
	HG_VM_user_END
};
enum _hg_vm_sys_params_name_t {
	HG_VM_sys_END
};
enum _hg_vm_pdev_params_name_t {
	HG_VM_pdev_END
};
struct _hg_vm_user_params_t {
	gsize max_op_stack;
	gsize max_exec_stack;
	gsize max_dict_stack;
	gsize max_gstate_stack;
};
struct _hg_vm_sys_params_t {
};
struct _hg_vm_pdev_params_t {
};
struct _hg_vm_t {
	hg_quark_t           self;
	hg_name_t           *name;
	hg_vm_state_t        vm_state;
	hg_vm_user_params_t  user_params;
	hg_mem_t            *mem[HG_VM_MEM_END];
	gint                 mem_id[HG_VM_MEM_END];
	hg_quark_t           qio[HG_FILE_IO_END];
	hg_stack_t          *stacks[HG_VM_STACK_END];
	GPtrArray           *stacks_stack;
	hg_quark_t           qerror_name[HG_VM_e_END];
	hg_quark_t           quparams_name[HG_VM_user_END];
	hg_quark_t           qsparams_name[HG_VM_sys_END];
	hg_quark_t           qpdevparams_name[HG_VM_pdev_END];
	hg_quark_t           qerror;
	hg_quark_t           qsystemdict;
	hg_quark_t           qglobaldict;
	hg_quark_t           qinternaldict;
	hg_quark_t           qgstate;
	hg_scanner_t        *scanner;
	hg_lineedit_t       *lineedit;
	hg_vm_langlevel_t    language_level;
	gboolean             is_initialized:1;
	gboolean             hold_lang_level:1;
	gboolean             shutdown:1;
	gboolean             has_error:1;
	gint                 error_code;
	guint                qattributes;
	guint                n_nest_scan;
	GHashTable          *params;
	GHashTable          *plugin_table;
	GList               *plugin_list;
	GRand               *rand_;
	guint32              rand_seed;
	GTimeVal             initiation_time;
	hg_device_t         *device;
};


/* hg_vm_t */
gboolean           hg_vm_init                  (void);
void               hg_vm_tini                  (void);
hg_vm_t           *hg_vm_new                   (void);
void               hg_vm_destroy               (hg_vm_t            *vm);
hg_quark_t         hg_vm_malloc                (hg_vm_t            *vm,
                                                gsize               size,
                                                gpointer           *ret);
hg_quark_t         hg_vm_realloc               (hg_vm_t            *vm,
                                                hg_quark_t          qdata,
                                                gsize               size,
                                                gpointer           *ret);
void               hg_vm_mfree                 (hg_vm_t            *vm,
                                                hg_quark_t          qdata);
gpointer           hg_vm_lock_object           (hg_vm_t            *vm,
                                                hg_quark_t          qdata,
                                                const gchar        *pretty_function,
                                                GError            **error);
void               hg_vm_unlock_object         (hg_vm_t            *vm,
                                                hg_quark_t          qdata);
void               hg_vm_set_default_attributes(hg_vm_t            *vm,
                                                guint               qattributes);
hg_mem_t          *hg_vm_get_mem               (hg_vm_t            *vm);
hg_mem_t          *hg_vm_get_mem_from_quark    (hg_vm_t            *vm,
                                                hg_quark_t          qdata);
void               hg_vm_use_global_mem        (hg_vm_t            *vm,
                                                gboolean            flag);
gboolean           hg_vm_is_global_mem_used    (hg_vm_t            *vm);
hg_quark_t         hg_vm_get_io                (hg_vm_t            *vm,
                                                hg_file_io_t        type,
						GError            **error);
void               hg_vm_set_io                (hg_vm_t            *vm,
                                                hg_file_io_t        type,
                                                hg_quark_t          file);
gboolean           hg_vm_setup                 (hg_vm_t            *vm,
                                                hg_vm_langlevel_t   lang_level,
                                                hg_quark_t          stdin,
                                                hg_quark_t          stdout,
                                                hg_quark_t          stderr);
void               hg_vm_finish                (hg_vm_t            *vm);
hg_vm_langlevel_t  hg_vm_get_language_level    (hg_vm_t            *vm);
gboolean           hg_vm_set_language_level    (hg_vm_t            *vm,
                                                hg_vm_langlevel_t   level);
void               hg_vm_hold_language_level   (hg_vm_t            *vm,
                                                gboolean            flag);
hg_quark_t         hg_vm_dict_lookup           (hg_vm_t            *vm,
                                                hg_quark_t          qname);
gboolean           hg_vm_dict_remove           (hg_vm_t            *vm,
                                                hg_quark_t          qname,
                                                gboolean            remove_all);
hg_quark_t         hg_vm_get_dict              (hg_vm_t            *vm);
gboolean           hg_vm_stepi                 (hg_vm_t            *vm,
                                                gboolean           *is_proceeded);
gboolean           hg_vm_step                  (hg_vm_t            *vm);
gboolean           hg_vm_main_loop             (hg_vm_t            *vm);
gchar             *hg_vm_find_libfile          (hg_vm_t            *vm G_GNUC_UNUSED,
                                                const gchar        *filename);
gboolean           hg_vm_eval                  (hg_vm_t            *vm,
                                                hg_quark_t          qeval,
                                                hg_stack_t         *ostack,
                                                hg_stack_t         *estack,
                                                hg_stack_t         *dstack,
                                                gboolean            protect_systemdict,
                                                GError            **error);
gboolean           hg_vm_eval_from_cstring     (hg_vm_t            *vm,
                                                const gchar        *cstring,
                                                gssize              clen,
                                                hg_stack_t         *ostack,
                                                hg_stack_t         *estack,
                                                hg_stack_t         *dstack,
                                                gboolean            protect_systemdict,
                                                GError            **error);
gboolean           hg_vm_eval_from_file        (hg_vm_t            *vm,
                                                const gchar        *initfile,
                                                hg_stack_t         *ostack,
                                                hg_stack_t         *estack,
                                                hg_stack_t         *dstack,
                                                gboolean            protect_systemdict,
                                                GError            **error);
gint               hg_vm_get_error_code        (hg_vm_t            *vm);
void               hg_vm_set_error_code        (hg_vm_t            *vm,
                                                gint                error_code);
gboolean           hg_vm_startjob              (hg_vm_t            *vm,
                                                hg_vm_langlevel_t   lang_level,
                                                const gchar        *initfile,
                                                gboolean            encapsulated);
void               hg_vm_shutdown              (hg_vm_t            *vm,
                                                gint                error_code);
hg_quark_t         hg_vm_get_gstate            (hg_vm_t            *vm);
void               hg_vm_set_gstate            (hg_vm_t            *vm,
                                                hg_quark_t          qgstate);
hg_quark_t         hg_vm_get_user_params       (hg_vm_t            *vm,
                                                gpointer           *ret,
                                                GError            **error);
void               hg_vm_set_user_params       (hg_vm_t            *vm,
                                                hg_quark_t          qdict,
                                                GError            **error);
void               hg_vm_set_rand_seed         (hg_vm_t            *vm,
                                                guint32             seed);
guint32            hg_vm_get_rand_seed         (hg_vm_t            *vm);
guint32            hg_vm_rand_int              (hg_vm_t            *vm);
guint32            hg_vm_get_current_time      (hg_vm_t            *vm);
gboolean           hg_vm_has_error             (hg_vm_t            *vm);
void               hg_vm_clear_error           (hg_vm_t            *vm);
void               hg_vm_reset_error           (hg_vm_t            *vm);
gboolean           hg_vm_set_error             (hg_vm_t            *vm,
                                                hg_quark_t          qdata,
                                                hg_vm_error_t       error);
gboolean           hg_vm_set_error_from_gerror (hg_vm_t            *vm,
                                                hg_quark_t          qdata,
                                                GError             *error);
void               hg_vm_reserved_spool_dump   (hg_vm_t            *vm,
                                                hg_mem_t           *mem,
                                                hg_file_t          *ofile);
gboolean           hg_vm_add_plugin            (hg_vm_t            *vm,
                                                const gchar        *name,
                                                GError            **error);
void               hg_vm_load_plugins          (hg_vm_t            *vm);
gboolean           hg_vm_remove_plugin         (hg_vm_t            *vm,
                                                const gchar        *name,
                                                GError            **error);
void               hg_vm_unload_plugins        (hg_vm_t            *vm);
gboolean           hg_vm_set_device            (hg_vm_t            *vm,
                                                const gchar        *name);
void               hg_vm_add_param             (hg_vm_t            *vm,
                                                const gchar        *name,
                                                hg_vm_value_t      *value);

/* hg_array_t */
gboolean   hg_vm_array_set(hg_vm_t     *vm,
                           hg_quark_t   qarray,
                           hg_quark_t   qdata,
                           gsize        index,
                           gboolean     force,
                           GError     **error);
hg_quark_t hg_vm_array_get(hg_vm_t     *vm,
                           hg_quark_t   qarray,
                           gsize        index,
                           gboolean     force,
                           GError     **error);

/* hg_dict_t */
gboolean hg_vm_dict_add(hg_vm_t     *vm,
			hg_quark_t   qdict,
			hg_quark_t   qkey,
			hg_quark_t   qval,
			gboolean     force,
			GError     **error);

/* hg_quark_t */
gboolean   hg_vm_quark_gc_mark               (hg_vm_t     *vm,
                                              hg_quark_t   qdata,
                                              GError     **error);
hg_quark_t hg_vm_quark_copy                  (hg_vm_t     *vm,
                                              hg_quark_t   qdata,
                                              gpointer    *ret,
                                              GError     **error);
hg_quark_t hg_vm_quark_to_string             (hg_vm_t     *vm,
                                              hg_quark_t   qdata,
                                              gboolean     ps_like_syntax,
                                              gpointer    *ret,
                                              GError     **error);
gboolean   hg_vm_quark_compare               (hg_vm_t     *vm,
                                              hg_quark_t   qdata1,
                                              hg_quark_t   qdata2);
gboolean   hg_vm_quark_compare_content       (hg_vm_t     *vm,
                                              hg_quark_t   qdata1,
                                              hg_quark_t   qdata2);
void       hg_vm_quark_set_default_attributes(hg_vm_t     *vm,
                                              hg_quark_t  *qdata);
void       hg_vm_quark_set_attributes        (hg_vm_t     *vm,
                                              hg_quark_t  *qdata,
                                              gboolean     readable,
                                              gboolean     writable,
                                              gboolean     executable,
                                              gboolean     editable);
void       hg_vm_quark_set_readable          (hg_vm_t     *vm,
                                              hg_quark_t  *qdata,
                                              gboolean     flag);
gboolean   hg_vm_quark_is_readable           (hg_vm_t     *vm,
                                              hg_quark_t  *qdata);
void       hg_vm_quark_set_writable          (hg_vm_t     *vm,
                                              hg_quark_t  *qdata,
                                              gboolean     flag);
gboolean   hg_vm_quark_is_writable           (hg_vm_t     *vm,
                                              hg_quark_t  *qdata);
void       hg_vm_quark_set_executable        (hg_vm_t     *vm,
                                              hg_quark_t  *qdata,
                                              gboolean     flag);
gboolean   hg_vm_quark_is_executable         (hg_vm_t     *vm,
                                              hg_quark_t  *qdata);
void       hg_vm_quark_set_editable          (hg_vm_t     *vm,
                                              hg_quark_t  *qdata,
                                              gboolean     flag);
gboolean   hg_vm_quark_is_editable           (hg_vm_t     *vm,
                                              hg_quark_t  *qdata);

/* hg_stack_t */
hg_stack_t *hg_vm_stack_new (hg_vm_t    *vm,
                             gsize       size);
void        hg_vm_stack_dump(hg_vm_t    *vm,
                             hg_stack_t *stack,
                             hg_file_t  *output);

/* hg_vm_value_t */
void           hg_vm_value_free       (gpointer     data);
hg_vm_value_t *hg_vm_value_boolean_new(gboolean     value);
hg_vm_value_t *hg_vm_value_integer_new(gint32       value);
hg_vm_value_t *hg_vm_value_real_new   (gdouble      value);
hg_vm_value_t *hg_vm_value_string_new (const gchar *value);

G_END_DECLS

#endif /* __HIEROGLYPH_HGVM_H__ */
