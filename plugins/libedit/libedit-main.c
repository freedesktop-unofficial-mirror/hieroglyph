/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * libedit-main.c
 * Copyright (C) 2006-2010 Akira TAGOH
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef USE_LIBEDIT
#error Please install libedit and/or the development package of it and run configure again.  This plugin is for libedit only.
#endif

#include <readline.h>
#define PLUGIN
#include "hgdict.h"
#include "hgplugin.h"
#include "hgoperator.h"
#include "hgstack.h"
#include "hgvm.h"


static gchar    *_libedit_get_line    (hg_lineedit_t  *lineedit,
                                       const gchar    *prompt,
                                       gpointer        user_data);
static void      _libedit_add_history (hg_lineedit_t  *lineedit,
                                       const gchar    *history,
                                       gpointer        user_data);
static gboolean  _libedit_load_history(hg_lineedit_t  *lineedit,
                                       const gchar    *historyfile,
                                       gpointer        user_data);
static gboolean  _libedit_save_history(hg_lineedit_t  *lineedit,
                                       const gchar    *historyfile,
                                       gpointer        user_data);

PROTO_PLUGIN (libedit);

typedef enum {
	HG_libedit_enc_loadhistory = 0,
	HG_libedit_enc_savehistory,
	HG_libedit_enc_END
} hg_libedit_encoding_t;
static hg_lineedit_vtable_t __libedit_lineedit_vtable = {
	.get_line = _libedit_get_line,
	.add_history = _libedit_add_history,
	.load_history = _libedit_load_history,
	.save_history = _libedit_save_history,
};
static hg_quark_t __libedit_enc_list[HG_libedit_enc_END];

/*< private >*/
static gchar *
_libedit_get_line(hg_lineedit_t *lineedit,
		  const gchar   *prompt,
		  gpointer       user_data)
{
	gchar *retval;

	if (prompt == NULL)
		retval = readline("");
	else
		retval = readline(prompt);

	return retval;
}

static void
_libedit_add_history(hg_lineedit_t *lineedit,
		     const gchar   *history,
		     gpointer       user_data)
{
	hg_return_if_fail (history != NULL);

	if (history[0] == 0 ||
	    history[0] == '\n' ||
	    history[0] == '\r' ||
	    (history[0] == '\r' && history[1] == '\n'))
		return;

	add_history(history);
}

static gboolean
_libedit_load_history(hg_lineedit_t *lineedit,
		      const gchar   *historyfile,
		      gpointer       user_data)
{
	hg_return_val_if_fail (historyfile != NULL, FALSE);

	read_history(historyfile);

	return TRUE;
}

static gboolean
_libedit_save_history(hg_lineedit_t *lineedit,
		      const gchar   *historyfile,
		      gpointer       user_data)
{
	hg_return_val_if_fail (historyfile != NULL, FALSE);

	write_history(historyfile);

	return TRUE;
}

/* -filename- .loadhistory - */
DEFUNC_OPER (private_loadhistory)
G_STMT_START {
	hg_quark_t arg0;
	hg_string_t *s;
	gchar *cstr, *filename, *histfile;
	const gchar *env;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QSTRING (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg0, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	cstr = hg_string_get_cstr(s);
	HG_VM_UNLOCK (vm, arg0);

	filename = g_path_get_basename(cstr);
	env = g_getenv("HOME");
	if (env != NULL) {
		histfile = g_build_filename(env, filename, NULL);
	} else {
		histfile = g_strdup(filename);
	}
	retval = hg_lineedit_load_history(vm->lineedit, histfile);

	g_free(histfile);
	g_free(filename);
	g_free(cstr);

	hg_stack_drop(ostack, error);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* -filename- .savehistory - */
DEFUNC_OPER (private_savehistory)
G_STMT_START {
	hg_quark_t arg0;
	hg_string_t *s;
	gchar *cstr, *filename, *histfile;
	const gchar *env;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QSTRING (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg0, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	cstr = hg_string_get_cstr(s);
	HG_VM_UNLOCK (vm, arg0);

	filename = g_path_get_basename(cstr);
	env = g_getenv("HOME");
	if (env != NULL) {
		histfile = g_build_filename(env, filename, NULL);
	} else {
		histfile = g_strdup(filename);
	}
	retval = hg_lineedit_save_history(vm->lineedit, histfile);

	g_free(histfile);
	g_free(filename);
	g_free(cstr);

	hg_stack_drop(ostack, error);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

static gboolean
_libedit_init(void)
{
	gint i;

	for (i = 0; i < HG_libedit_enc_END; i++) {
		__libedit_enc_list[i] = Qnil;
	}

	return TRUE;
}

static gboolean
_libedit_finalize(void)
{
	return TRUE;
}

static gboolean
_libedit_load(hg_plugin_t  *plugin,
	      gpointer      vm_,
	      GError      **error)
{
	hg_vm_t *vm = vm_;
	hg_dict_t *dict;
	gboolean is_global;
	gint i;

	if (plugin->user_data != NULL) {
		g_warning("plugin is already loaded.");
		return TRUE;
	}
	plugin->user_data = vm->lineedit;

	is_global = hg_vm_is_global_mem_used(vm);
	dict = HG_VM_LOCK (vm, vm->qsystemdict, error);
	if (dict == NULL)
		return FALSE;
	hg_vm_use_global_mem(vm, TRUE);

	vm->lineedit = hg_lineedit_new(hg_vm_get_mem(vm),
				       &__libedit_lineedit_vtable,
				       NULL, NULL);

	__libedit_enc_list[HG_libedit_enc_loadhistory] = REG_ENC (vm->name, .loadhistory, private_loadhistory);
	__libedit_enc_list[HG_libedit_enc_savehistory] = REG_ENC (vm->name, .savehistory, private_savehistory);

	for (i = 0; i < HG_libedit_enc_END; i++) {
		if (__libedit_enc_list[i] == Qnil)
			return FALSE;
		REG_OPER (dict, __libedit_enc_list[i]);
	}

	hg_vm_use_global_mem(vm, is_global);

	return TRUE;
}

static gboolean
_libedit_unload(hg_plugin_t  *plugin,
		gpointer      vm_,
		GError      **error)
{
	hg_vm_t *vm G_GNUC_UNUSED = vm_;

	if (plugin->user_data == NULL) {
		g_warning("plugin not loaded.");
		return FALSE;
	}
	hg_lineedit_destroy(vm->lineedit);
	vm->lineedit = plugin->user_data;
	plugin->user_data = NULL;

	return TRUE;
}

/*< public >*/
hg_plugin_t *
plugin_new(hg_mem_t  *mem,
	   GError   **error)
{
	hg_return_val_with_gerror_if_fail (mem != NULL, NULL, error);

	return hg_plugin_new(mem,
			     &__libedit_plugin_info);
}
