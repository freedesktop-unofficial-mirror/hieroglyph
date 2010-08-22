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
#include "hieroglyph/hgdict.h"
#include "hieroglyph/hgplugin.h"
#include "hieroglyph/hgoperator.h"
#include "hieroglyph/hgstack.h"
#include "hieroglyph/hgvm.h"


static gboolean _libedit_init    (void);
static gboolean _libedit_finalize(void);
static gboolean _libedit_load    (hg_plugin_t  *plugin,
                                  gpointer      vm_,
                                  GError      **error);
static gboolean _libedit_unload  (hg_plugin_t  *plugin,
                                  gpointer      vm_,
                                  GError      **error);
hg_plugin_t     *plugin_new      (hg_mem_t     *mem,
				  GError      **error);


typedef enum {
	HG_libedit_enc_loadhistory = 0,
	HG_libedit_enc_statementedit,
	HG_libedit_enc_savehistory,
	HG_libedit_enc_END
} hg_libedit_encoding_t;
static hg_plugin_vtable_t __libedit_plugin_vtable = {
	.init     = _libedit_init,
	.finalize = _libedit_finalize,
	.load     = _libedit_load,
	.unload   = _libedit_unload,
};
static hg_plugin_info_t __libedit_plugin_info = {
	.version = HG_PLUGIN_VERSION,
	.vtable  = &__libedit_plugin_vtable,
};
static hg_quark_t __libedit_enc_list[HG_libedit_enc_END];

/*< private >*/
DEFUNC_OPER (private_loadhistory)
G_STMT_START {
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_OPER (private_statementedit)
G_STMT_START {
	g_print("fooo\n");
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_OPER (private_savehistory)
G_STMT_START {
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
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
	hg_stack_t *estack;

	is_global = hg_vm_is_global_mem_used(vm);
	dict = HG_VM_LOCK (vm, vm->qsystemdict, error);
	if (dict == NULL)
		return FALSE;
	hg_vm_use_global_mem(vm, TRUE);

	__libedit_enc_list[HG_libedit_enc_loadhistory] = REG_ENC (vm->name, .loadhistory, private_loadhistory);
	__libedit_enc_list[HG_libedit_enc_statementedit] = REG_ENC (vm->name, .statementedit, private_statementedit);
	__libedit_enc_list[HG_libedit_enc_savehistory] = REG_ENC (vm->name, .savehistory, private_savehistory);

	for (i = 0; i < HG_libedit_enc_END; i++) {
		if (__libedit_enc_list[i] == Qnil)
			return FALSE;
		REG_OPER (dict, __libedit_enc_list[i]);
	}

	estack = hg_vm_stack_new(vm, 256);
	hg_vm_eval_from_cstring(vm, "/.libedit.old..statementedit /..statementedit load def /..statementedit {.promptmsg //null exch .statementedit exch pop} bind def",
				NULL, estack, NULL, error);
	hg_stack_free(estack);
	hg_vm_use_global_mem(vm, is_global);

	return TRUE;
}

static gboolean
_libedit_unload(hg_plugin_t  *plugin,
		gpointer      vm_,
		GError      **error)
{
	hg_vm_t *vm = vm_;
	hg_stack_t *estack = hg_vm_stack_new(vm, 256);

	hg_vm_eval_from_cstring(vm, "/..statementedit /.libedit.old..statementedit load def",
				NULL, estack, NULL, error);
	hg_stack_free(estack);

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
