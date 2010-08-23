/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * unittest-main.c
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

#define PLUGIN
#include "hgdict.h"
#include "hgplugin.h"
#include "hgoperator.h"
#include "hgvm.h"


PROTO_PLUGIN (unittest);

typedef enum {
	HG_unittest_enc_validatetestresult,
	HG_unittest_enc_END
} hg_unittest_encoding_t;
static hg_quark_t __unittest_enc_list[HG_unittest_enc_END];

/*< private >*/
DEFUNC_OPER (private_validatetestresult)
G_STMT_START {
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

static gboolean
_unittest_init(void)
{
	gint i;

	for (i = 0; i < HG_unittest_enc_END; i++) {
		__unittest_enc_list[i] = Qnil;
	}

	return TRUE;
}

static gboolean
_unittest_finalize(void)
{
	return TRUE;
}

static gboolean
_unittest_load(hg_plugin_t  *plugin,
	       gpointer      vm_,
	       GError      **error)
{
	hg_vm_t *vm = vm_;
	hg_dict_t *dict;
	gboolean is_global, retval;
	gint i;
	hg_stack_t *estack;

	if (plugin->user_data != NULL) {
		g_warning("plugin is already loaded.");
		return TRUE;
	}
	plugin->user_data = plugin; /* dummy */

	is_global = hg_vm_is_global_mem_used(vm);
	dict = HG_VM_LOCK (vm, vm->qsystemdict, error);
	if (dict == NULL)
		return FALSE;
	hg_vm_use_global_mem(vm, TRUE);

	__unittest_enc_list[HG_unittest_enc_validatetestresult] = REG_ENC (vm->name, .validatetestresult, private_validatetestresult);

	for (i = 0; i < HG_unittest_enc_END; i++) {
		if (__unittest_enc_list[i] == Qnil)
			return FALSE;
		REG_OPER (dict, __unittest_enc_list[i]);
	}

	estack = hg_vm_stack_new(vm, 256);
	retval = hg_vm_eval_from_cstring(vm, "{(hg_unittest.ps) runlibfile} stopped {(** Unable to initialize the unittest extension plugin\n) =} if",
					 NULL, estack, NULL, error);
	hg_stack_free(estack);

	hg_vm_use_global_mem(vm, is_global);

	return retval;
}

static gboolean
_unittest_unload(hg_plugin_t  *plugin,
		 gpointer      vm_,
		 GError      **error)
{
	if (plugin->user_data == NULL) {
		g_warning("plugin not loaded.");
		return FALSE;
	}
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
			     &__unittest_plugin_info);
}
