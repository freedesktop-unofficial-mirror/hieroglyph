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

#define PLUGIN
#include "hg.h"


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
	hg_bool_t verbose = FALSE, result = TRUE;
	hg_int_t attrs HG_GNUC_UNUSED = 0; /* XXX */
	hg_char_t *cexp;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QDICT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	d = HG_VM_LOCK (vm, arg0);
	if (d == NULL) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	qverbose = hg_dict_lookup(d, HG_QNAME ("verbose"));
	if (qverbose != Qnil && HG_IS_QBOOL (qverbose)) {
		verbose = HG_BOOL (qverbose);
	}
	qattrs = hg_dict_lookup(d, HG_QNAME ("attrsmask"));
	if (qattrs != Qnil && HG_IS_QINT (qattrs)) {
		attrs = HG_INT (qattrs);
	}

	qexp = hg_dict_lookup(d, HG_QNAME ("expression"));
	q = hg_vm_quark_to_string(vm, qexp, TRUE, (hg_pointer_t *)&sexp);
	if (q == Qnil) {
		cexp = hg_strdup("--%unknown--");
	} else {
		cexp = hg_string_get_cstr(sexp);
	}
	hg_string_free(sexp, TRUE);

	qaerror = hg_dict_lookup(d, HG_QNAME ("actualerror"));
	qeerror = hg_dict_lookup(d, HG_QNAME ("expectederror"));
	qerrorat = hg_dict_lookup(d, HG_QNAME ("errorat"));
	if (qaerror == Qnil || qeerror == Qnil || qerrorat == Qnil) {
		e = HG_e_typecheck;
		goto error;
	} else if (!hg_vm_quark_compare(vm, qaerror, qeerror)) {
		if (verbose) {
			hg_quark_t qa, qe, qf, qp;
			hg_string_t *sa, *se, *sp;
			hg_file_t *f;
			hg_char_t *csa, *cse, *csp;

			qa = hg_vm_quark_to_string(vm, qaerror, TRUE, (hg_pointer_t *)&sa);
			if (qa == Qnil) {
				csa = hg_strdup("--%unknown--");
			} else {
				csa = hg_string_get_cstr(sa);
			}
			hg_string_free(sa, TRUE);
			qe = hg_vm_quark_to_string(vm, qeerror, TRUE, (hg_pointer_t *)&se);
			if (qe == Qnil) {
				cse = hg_strdup("--%unknown--");
			} else {
				cse = hg_string_get_cstr(se);
			}
			hg_string_free(se, TRUE);
			qp = hg_vm_quark_to_string(vm, qerrorat, TRUE, (hg_pointer_t *)&sp);
			if (qp == Qnil) {
				csp = hg_strdup("--%unknown--");
			} else {
				csp = hg_string_get_cstr(sp);
			}
			hg_string_free(sp, TRUE);

			qf = hg_vm_get_io(vm, HG_FILE_IO_STDERR);
			f = HG_VM_LOCK (vm, qf);
			hg_file_append_printf(f, "Expression: %s - expected error is %s, but actual error was %s at %s\n",
					      cexp, cse, csa, csp);
			HG_VM_UNLOCK (vm, qf);
			hg_free(csa);
			hg_free(cse);
			hg_free(csp);
		}
		result = FALSE;
	}

	qastack = hg_dict_lookup(d, HG_QNAME ("actualostack"));
	qestack = hg_dict_lookup(d, HG_QNAME ("expectedostack"));
	if (qastack == Qnil || qestack == Qnil ||
	    !HG_IS_QARRAY (qastack) ||
	    !HG_IS_QARRAY (qestack)) {
		e = HG_e_typecheck;
		goto error;
	} else {
		hg_string_t *sa, *se;
		hg_quark_t qa, qe, qf;
		hg_char_t *csa, *cse;
		hg_file_t *f;

		qa = hg_vm_quark_to_string(vm, qastack, TRUE, (hg_pointer_t *)&sa);
		if (qa == Qnil) {
			csa = hg_strdup("--%unknown--");
		} else {
			csa = hg_string_get_cstr(sa);
		}
		hg_string_free(sa, TRUE);
		qe = hg_vm_quark_to_string(vm, qestack, TRUE, (hg_pointer_t *)&se);
		if (qe == Qnil) {
			cse = hg_strdup("--%unknown--");
		} else {
			cse = hg_string_get_cstr(se);
		}
		hg_string_free(se, TRUE);

		if (!hg_vm_quark_compare_content(vm, qastack, qestack)) {
			if (verbose) {
				qf = hg_vm_get_io(vm, HG_FILE_IO_STDERR);
				f = HG_VM_LOCK (vm, qf);
				hg_file_append_printf(f, "Expression: %s - expected ostack is %s, but actual ostack was %s\n",
						      cexp, cse, csa);
			}
			result = FALSE;
		}
		retval = TRUE;

		hg_free(cse);
		hg_free(csa);
	}

  error:
	hg_free(cexp);
	HG_VM_UNLOCK (vm, arg0);

	hg_stack_drop(ostack);
	STACK_PUSH (ostack, HG_QBOOL (result && retval));
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

static hg_bool_t
_unittest_init(void)
{
	hg_int_t i;

	for (i = 0; i < HG_unittest_enc_END; i++) {
		__unittest_enc_list[i] = Qnil;
	}

	return TRUE;
}

static hg_bool_t
_unittest_finalize(void)
{
	return TRUE;
}

static hg_bool_t
_unittest_load(hg_plugin_t  *plugin,
	       hg_pointer_t  vm_)
{
	hg_vm_t *vm = vm_;
	hg_dict_t *dict;
	hg_bool_t is_global, retval;
	hg_int_t i;
	const hg_char_t ps[] = "{(hg_unittest.ps) runlibfile} stopped {(** Unable to initialize the unittest extension plugin: reason: /) $error /errorname get 256 string cvs .concatstring = (\n) = } if";

	if (plugin->user_data != NULL) {
		hg_warning("plugin is already loaded.");
		return TRUE;
	}
	plugin->user_data = plugin; /* dummy */

	is_global = hg_vm_is_global_mem_used(vm);
	dict = HG_VM_LOCK (vm, vm->qsystemdict);
	if (dict == NULL)
		return FALSE;
	hg_vm_use_global_mem(vm, TRUE);

	__unittest_enc_list[HG_unittest_enc_validatetestresult] = REG_ENC (.validatetestresult, private_validatetestresult);

	for (i = 0; i < HG_unittest_enc_END; i++) {
		if (__unittest_enc_list[i] == Qnil)
			return FALSE;
		REG_OPER (dict, __unittest_enc_list[i]);
	}

	hg_vm_use_global_mem(vm, is_global);

	retval = hg_vm_eval_cstring(vm, ps, -1, TRUE);

	return retval;
}

static hg_bool_t
_unittest_unload(hg_plugin_t  *plugin,
		 hg_pointer_t  vm_)
{
	if (plugin->user_data == NULL) {
		hg_warning("plugin not loaded.");
		return FALSE;
	}
	plugin->user_data = NULL;

	return TRUE;
}

/*< public >*/
hg_plugin_t *
plugin_new(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);

	return hg_plugin_new(mem,
			     &__unittest_plugin_info);
}
