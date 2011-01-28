/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * unittest-main.c
 * Copyright (C) 2006-2011 Akira TAGOH
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

#include <glib.h>
#define PLUGIN
#include "hgarray.h"
#include "hgbool.h"
#include "hgdict.h"
#include "hgint.h"
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
{
	hg_quark_t arg0, qverbose, qattrs, qexp, qaerror, qeerror, qerrorat, q;
	hg_quark_t qastack, qestack;
	hg_string_t *sexp;
	hg_dict_t *d;
	gboolean verbose = FALSE, result = TRUE;
	gint32 attrs = 0;
	gchar *cexp;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	d = HG_VM_LOCK (vm, arg0, error);
	if (d == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	qverbose = hg_dict_lookup(d, HG_QNAME (vm->name, "verbose"), error);
	if (qverbose != Qnil && HG_IS_QBOOL (qverbose)) {
		verbose = HG_BOOL (qverbose);
	}
	qattrs = hg_dict_lookup(d, HG_QNAME (vm->name, "attrsmask"), error);
	if (qattrs != Qnil && HG_IS_QINT (qattrs)) {
		attrs = HG_INT (qattrs);
	}

	qexp = hg_dict_lookup(d, HG_QNAME (vm->name, "expression"), error);
	q = hg_vm_quark_to_string(vm, qexp, TRUE, (gpointer *)&sexp, error);
	if (q == Qnil) {
		cexp = g_strdup("--%unknown--");
	} else {
		cexp = hg_string_get_cstr(sexp);
	}
	hg_string_free(sexp, TRUE);

	qaerror = hg_dict_lookup(d, HG_QNAME (vm->name, "actualerror"), error);
	qeerror = hg_dict_lookup(d, HG_QNAME (vm->name, "expectederror"), error);
	qerrorat = hg_dict_lookup(d, HG_QNAME (vm->name, "errorat"), error);
	if (qaerror == Qnil || qeerror == Qnil || qerrorat == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		goto error;
	} else if (!hg_vm_quark_compare(vm, qaerror, qeerror)) {
		if (verbose) {
			hg_quark_t qa, qe, qf, qp;
			hg_string_t *sa, *se, *sp;
			hg_file_t *f;
			gchar *csa, *cse, *csp;

			qa = hg_vm_quark_to_string(vm, qaerror, TRUE, (gpointer *)&sa, error);
			if (qa == Qnil) {
				csa = g_strdup("--%unknown--");
			} else {
				csa = hg_string_get_cstr(sa);
			}
			hg_string_free(sa, TRUE);
			qe = hg_vm_quark_to_string(vm, qeerror, TRUE, (gpointer *)&se, error);
			if (qe == Qnil) {
				cse = g_strdup("--%unknown--");
			} else {
				cse = hg_string_get_cstr(se);
			}
			hg_string_free(se, TRUE);
			qp = hg_vm_quark_to_string(vm, qerrorat, TRUE, (gpointer *)&sp, error);
			if (qp == Qnil) {
				csp = g_strdup("--%unknown--");
			} else {
				csp = hg_string_get_cstr(sp);
			}
			hg_string_free(sp, TRUE);

			qf = hg_vm_get_io(vm, HG_FILE_IO_STDERR, error);
			f = HG_VM_LOCK (vm, qf, error);
			hg_file_append_printf(f, "Expression: %s - expected error is %s, but actual error was %s at %s\n",
					      cexp, cse, csa, csp);
			HG_VM_UNLOCK (vm, qf);
			g_free(csa);
			g_free(cse);
			g_free(csp);
		}
		result = FALSE;
	}

	qastack = hg_dict_lookup(d, HG_QNAME (vm->name, "actualostack"), error);
	qestack = hg_dict_lookup(d, HG_QNAME (vm->name, "expectedostack"), error);
	if (qastack == Qnil || qestack == Qnil ||
	    !HG_IS_QARRAY (qastack) ||
	    !HG_IS_QARRAY (qestack)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		goto error;
	} else {
		hg_string_t *sa, *se;
		hg_quark_t qa, qe, qf;
		gchar *csa, *cse;
		hg_file_t *f;

		qa = hg_vm_quark_to_string(vm, qastack, TRUE, (gpointer *)&sa, error);
		if (qa == Qnil) {
			csa = g_strdup("--%unknown--");
		} else {
			csa = hg_string_get_cstr(sa);
		}
		hg_string_free(sa, TRUE);
		qe = hg_vm_quark_to_string(vm, qestack, TRUE, (gpointer *)&se, error);
		if (qe == Qnil) {
			cse = g_strdup("--%unknown--");
		} else {
			cse = hg_string_get_cstr(se);
		}
		hg_string_free(se, TRUE);

		if (!hg_vm_quark_compare_content(vm, qastack, qestack)) {
			if (verbose) {
				qf = hg_vm_get_io(vm, HG_FILE_IO_STDERR, error);
				f = HG_VM_LOCK (vm, qf, error);
				hg_file_append_printf(f, "Expression: %s - expected ostack is %s, but actual ostack was %s\n",
						      cexp, cse, csa);
			}
			result = FALSE;
		}
		retval = TRUE;

		g_free(cse);
		g_free(csa);
	}

  error:
	g_free(cexp);
	HG_VM_UNLOCK (vm, arg0);

	hg_stack_drop(ostack, error);
	STACK_PUSH (ostack, HG_QBOOL (result && retval));
} DEFUNC_OPER_END

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

	hg_vm_use_global_mem(vm, is_global);

	estack = hg_vm_stack_new(vm, 256);
	retval = hg_vm_eval_from_cstring(vm, "{(hg_unittest.ps) runlibfile} stopped {(** Unable to initialize the unittest extension plugin: reason: /) $error /errorname get 256 string cvs .concatstring = (\n) = } if", -1,
					 NULL, estack, NULL, TRUE, error);
	hg_stack_free(estack);

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
	hg_return_val_with_gerror_if_fail (mem != NULL, NULL, error, HG_VM_e_VMerror);

	return hg_plugin_new(mem,
			     &__unittest_plugin_info);
}
