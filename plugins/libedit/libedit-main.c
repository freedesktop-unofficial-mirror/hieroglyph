/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * libedit-main.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef USE_LIBEDIT
#error Please install libedit and run configure again.  This plugin is for libedit only.
#endif /* !USE_LIBEDIT */

#include <editline/readline.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hglineedit.h>
#include <hieroglyph/hglog.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgplugins.h>
#include <hieroglyph/hgstack.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/operator.h>
#include <hieroglyph/vm.h>


#define BUILD_OP(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		hg_operator_build_operator__inline(_libedit_op_, vm, pool, sdict, name, func, __hg_op); \
		if (__hg_op != NULL) {					\
			__libedit_operator_list[HG_libedit_op_##func] = __hg_op; \
			hg_mem_add_root_node(pool, __hg_op);		\
		}							\
	} G_STMT_END
#define REMOVE_OP(pool, sdict, name, func)				\
	G_STMT_START {							\
		HgValueNode *__hg_node, *__hg_key;			\
		HgOperator *__hg_op;					\
									\
		HG_VALUE_MAKE_NAME_STATIC (pool, __hg_key, #name);	\
		if ((__hg_node = hg_dict_lookup(sdict, __hg_key)) != NULL) { \
			__hg_op = HG_VALUE_GET_OPERATOR (__hg_node);	\
			if (__hg_op == __libedit_operator_list[HG_libedit_op_##func]) { \
				hg_dict_remove(sdict, __hg_key);	\
			}						\
			hg_mem_remove_root_node(pool, __hg_op);		\
			hg_mem_free(__hg_op);				\
		}							\
		hg_mem_free(__hg_key);					\
	} G_STMT_END
#define DEFUNC_OP(func)					\
	static gboolean					\
	_libedit_op_##func(HgOperator *op,		\
			 gpointer    data)		\
	{						\
		HgVM *vm = data;			\
		gboolean retval = FALSE;
#define DEFUNC_OP_END				\
		return retval;			\
	}

typedef enum {
	HG_libedit_op_loadhistory = 0,
	HG_libedit_op_statementedit,
	HG_libedit_op_savehistory,
	HG_libedit_op_END
} HgOperatorLibeditEncoding;


static gboolean  plugin_init                             (void);
static gboolean  plugin_finalize                         (void);
static gboolean  plugin_load                             (HgPlugin *plugin,
							  HgVM     *vm);
static gboolean  plugin_unload                           (HgPlugin *plugin,
							  HgVM     *vm);
static gchar    *_hg_line_edit__libedit_real_get_line    (HgLineEdit  *lineedit,
							  const gchar *prompt);
static void      _hg_line_edit__libedit_real_add_history (HgLineEdit  *lineedit,
							  const gchar *strings);
static void      _hg_line_edit__libedit_real_load_history(HgLineEdit  *lineedit,
							  const gchar *filename);
static void      _hg_line_edit__libedit_real_save_history(HgLineEdit  *lineedit,
							  const gchar *filename);


static HgPluginVTable vtable = {
	.init     = plugin_init,
	.finalize = plugin_finalize,
	.load     = plugin_load,
	.unload   = plugin_unload,
};

static const HgLineEditVTable lineedit_vtable = {
	.get_line     = _hg_line_edit__libedit_real_get_line,
	.add_history  = _hg_line_edit__libedit_real_add_history,
	.load_history = _hg_line_edit__libedit_real_load_history,
	.save_history = _hg_line_edit__libedit_real_save_history,
};
HgOperator *__libedit_operator_list[HG_libedit_op_END];

/*
 * Operators
 */
DEFUNC_OP (loadhistory)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgString *hs;
	gchar *filename, *histfile;
	const gchar *env;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hs = HG_VALUE_GET_STRING (node);
		env = g_getenv("HOME");
		filename = g_path_get_basename(hg_string_get_string(hs));
		if (env != NULL) {
			histfile = g_build_filename(env, filename, NULL);
		} else {
			histfile = g_strdup(filename);
		}
		retval = hg_line_edit_load_history(hg_vm_get_line_editor(vm),
						   histfile);
		g_free(histfile);
		g_free(filename);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (statementedit)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgString *hs;
	HgFileObject *file;
	gchar *line, *prompt;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hs = HG_VALUE_GET_STRING (node);
		prompt = g_strndup(hg_string_get_string(hs), hg_string_maxlength(hs));
		line = hg_line_edit_get_statement(hg_vm_get_line_editor(vm),
						  prompt);
		g_free(prompt);
		if (line == NULL || line[0] == 0) {
			_hg_operator_set_error(vm, op, VM_e_undefinedfilename);
			if (line)
				g_free(line);
			break;
		}
		file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER,
					  HG_FILE_MODE_READ,
					  "%statementedit",
					  line, -1);
		g_free(line);
		if (file == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_FILE (node, file);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (savehistory)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgString *hs;
	gchar *filename, *histfile;
	const gchar *env;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hs = HG_VALUE_GET_STRING (node);
		env = g_getenv("HOME");
		filename = g_path_get_basename(hg_string_get_string(hs));
		if (env != NULL) {
			histfile = g_build_filename(env, filename, NULL);
		} else {
			histfile = g_strdup(filename);
		}
		retval = hg_line_edit_save_history(hg_vm_get_line_editor(vm),
						   histfile);
		g_free(histfile);
		g_free(filename);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

/*
 * Private Functions
 */
static gboolean
plugin_init(void)
{
	gint i;

	for (i = 0; i < HG_libedit_op_END; i++) {
		__libedit_operator_list[i] = NULL;
	}

	return TRUE;
}

static gboolean
plugin_finalize(void)
{
	return TRUE;
}

static gboolean
plugin_load(HgPlugin *plugin,
	    HgVM     *vm)
{
	HgDict *systemdict;
	HgMemPool *pool;
	gboolean is_global, error;
	HgLineEdit *editor;
	HgStack *estack;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		hg_log_warning("already loaded.");
		return FALSE;
	}
	plugin->user_data = hg_vm_get_line_editor(vm);

	editor = hg_line_edit_new(hg_vm_get_current_pool(vm),
				  &lineedit_vtable,
				  __hg_file_stdin,
				  __hg_file_stdout);
	hg_vm_set_line_editor(vm, editor);

	systemdict = hg_vm_get_dict_systemdict(vm);
	is_global = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);

	BUILD_OP (vm, pool, systemdict, .loadhistory, loadhistory);
	BUILD_OP (vm, pool, systemdict, .statementedit, statementedit);
	BUILD_OP (vm, pool, systemdict, .savehistory, savehistory);

	estack = hg_stack_new(pool, 256);
	hg_vm_eval(vm, "/.libedit.old..statementedit /..statementedit load def /..statementedit {.promptmsg //null exch .statementedit exch pop} bind def",
		   NULL, estack, NULL, &error);
	hg_mem_free(estack);
	hg_vm_use_global_pool(vm, is_global);

	return TRUE;
}

static gboolean
plugin_unload(HgPlugin *plugin,
	      HgVM     *vm)
{
	HgMemPool *pool;
	HgDict *systemdict;
	gboolean is_global, error;
	HgStack *estack;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		hg_log_warning("not yet loaded.");
		return FALSE;
	}
	hg_vm_set_line_editor(vm, plugin->user_data);

	systemdict = hg_vm_get_dict_systemdict(vm);
	is_global = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);

	REMOVE_OP (pool, systemdict, .loadhistory, loadhistory);
	REMOVE_OP (pool, systemdict, .statementedit, statementedit);
	REMOVE_OP (pool, systemdict, .savehistory, savehistory);

	estack = hg_stack_new(pool, 256);
	hg_vm_eval(vm, "/..statementedit /.libedit.old..statementedit load def",
		   NULL, estack, NULL, &error);
	hg_mem_free(estack);

	hg_vm_use_global_pool(vm, is_global);

	plugin->user_data = NULL;

	return TRUE;
}

static gchar *
_hg_line_edit__libedit_real_get_line(HgLineEdit  *lineedit,
				     const gchar *prompt)
{
	gchar *retval;

	if (prompt == NULL)
		retval = readline("");
	else
		retval = readline(prompt);

	return retval;
}

static void
_hg_line_edit__libedit_real_add_history(HgLineEdit  *lineedit,
					const gchar *strings)
{
	g_return_if_fail (strings != NULL);

	add_history(strings);
}

static void
_hg_line_edit__libedit_real_load_history(HgLineEdit  *lineedit,
					 const gchar *filename)
{
	g_return_if_fail (filename != NULL);

	read_history(filename);
}

static void
_hg_line_edit__libedit_real_save_history(HgLineEdit  *lineedit,
					 const gchar *filename)
{
	g_return_if_fail (filename != NULL);

	write_history(filename);
}

/*
 * Public Functions
 */
HgPlugin *
plugin_new(HgMemPool *pool)
{
	HgPlugin *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_plugin_new(pool, HG_PLUGIN_EXTENSION);
	retval->version = HG_PLUGIN_VERSION;
	retval->vtable = &vtable;

	return retval;
}
