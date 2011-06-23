/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgvm.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <glib.h>
#include "hgarray.h"
#include "hgbool.h"
#include "hgdevice.h"
#include "hgdict.h"
#include "hggstate.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgmem.h"
#include "hgname.h"
#include "hgnull.h"
#include "hgoperator.h"
#include "hgpath.h"
#include "hgplugin.h"
#include "hgreal.h"
#include "hgscanner.h"
#include "hgsnapshot.h"
#include "hgutils.h"
#include "hgvm.h"

#include "hgvm.proto.h"

#define HG_VM_MEM_SIZE		128000
#define HG_VM_GLOBAL_MEM_SIZE	24000000
#define HG_VM_LOCAL_MEM_SIZE	1000000
#define _HG_VM_LOCK(_v_,_q_)						\
	(_hg_vm_quark_lock_object((_v_),(_q_),__PRETTY_FUNCTION__))
#define _HG_VM_UNLOCK(_v_,_q_)				\
	(_hg_vm_quark_unlock_object((_v_),(_q_)))
#undef HG_VM_DEBUG


typedef struct _hg_vm_dict_lookup_data_t {
	hg_vm_t    *vm;
	hg_quark_t  qname;
	hg_quark_t  result;
	hg_bool_t   check_perms;
} hg_vm_dict_lookup_data_t;
typedef struct _hg_vm_dict_remove_data_t {
	hg_vm_t    *vm;
	hg_quark_t  qname;
	hg_bool_t   remove_all;
	hg_bool_t   result;
} hg_vm_dict_remove_data_t;
typedef struct _hg_vm_stack_dump_data_t {
	hg_vm_t    *vm;
	hg_stack_t *stack;
	hg_file_t  *ofile;
} hg_vm_stack_dump_data_t;


hg_mem_t *__hg_vm_mem = NULL;
hg_bool_t __hg_vm_is_initialized = FALSE;

/*< private >*/
HG_INLINE_FUNC hg_mem_t *
_hg_vm_get_mem(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (vm->vm_state->current_mem_index < HG_VM_MEM_END, NULL, HG_e_VMerror);

	return vm->mem[vm->vm_state->current_mem_index];
}

HG_INLINE_FUNC hg_mem_t *
_hg_vm_quark_get_mem(hg_vm_t    *vm,
		     hg_quark_t  quark)
{
	hg_mem_t *retval = NULL;
	hg_int_t id;

	hg_return_val_if_fail (vm != NULL, NULL, HG_e_VMerror);

	id = hg_quark_get_mem_id(quark);

	if (vm->mem_id[HG_VM_MEM_GLOBAL] == id) {
		retval = vm->mem[HG_VM_MEM_GLOBAL];
	} else if (vm->mem_id[HG_VM_MEM_LOCAL] == id) {
		retval = vm->mem[HG_VM_MEM_LOCAL];
	} else {
		hg_warning("Unknown memory spooler id: %d, quark: %lx", id, quark);
		hg_error_return_val (NULL, HG_STATUS_FAILED, HG_e_VMerror);
	}

	return retval;
}

HG_INLINE_FUNC void
_hg_vm_quark_free(hg_vm_t    *vm,
		  hg_quark_t  qdata)
{
	hg_mem_t *m = _hg_vm_quark_get_mem(vm, qdata);

	hg_mem_free(m, qdata);
}

HG_INLINE_FUNC hg_pointer_t
_hg_vm_quark_lock_object(hg_vm_t         *vm,
			 hg_quark_t       qdata,
			 const hg_char_t *pretty_function)
{
	return _hg_mem_lock_object(_hg_vm_quark_get_mem(vm, qdata),
				   qdata,
				   pretty_function);
}

HG_INLINE_FUNC void
_hg_vm_quark_unlock_object(hg_vm_t    *vm,
			   hg_quark_t  qdata)
{
	hg_mem_unlock_object(_hg_vm_quark_get_mem(vm, qdata),
			     qdata);
}

static hg_bool_t
_hg_vm_real_dict_lookup(hg_mem_t     *mem,
			hg_quark_t    q,
			hg_pointer_t  data)
{
	hg_vm_dict_lookup_data_t *x = data;

	x->result = hg_vm_dict_lookup(x->vm, q, x->qname, x->check_perms);

	return x->result == Qnil && HG_ERROR_IS_SUCCESS0 ();
}

static hg_bool_t
_hg_vm_real_dict_remove(hg_mem_t     *mem,
			hg_quark_t    q,
			hg_pointer_t  data)
{
	hg_vm_dict_remove_data_t *x = data;
	hg_dict_t *dict;

	dict = _HG_VM_LOCK (x->vm, q);
	if (dict == NULL)
		return FALSE;

	x->result |= hg_dict_remove(dict, x->qname);

	_HG_VM_UNLOCK (x->vm, q);

	return !x->result || x->remove_all;
}

static hg_bool_t
_hg_vm_stack_real_dump(hg_mem_t     *mem,
		       hg_quark_t    qdata,
		       hg_pointer_t  data)
{
	hg_vm_stack_dump_data_t *ddata = data;
	hg_quark_t q;
	hg_string_t *s = NULL;
	hg_char_t *cstr = NULL;
	hg_char_t mc;

	q = hg_vm_quark_to_string(ddata->vm, qdata, TRUE, (hg_pointer_t *)&s);
	if (q != Qnil)
		cstr = hg_string_get_cstr(s);

	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata)) {
		mc = '-';
	} else {
		hg_uint_t id;

		id = hg_quark_get_mem_id(qdata);
		mc = (id == ddata->vm->mem_id[HG_VM_MEM_GLOBAL] ? 'G' : id == ddata->vm->mem_id[HG_VM_MEM_LOCAL] ? 'L' : '?');
	}
	hg_file_append_printf(ddata->ofile, "0x%016lx|%-12s|%c| %c%c%c|%s\n",
			      qdata,
			      hg_quark_get_type_name(qdata),
			      mc,
			      (hg_quark_is_readable(qdata) ? 'r' : '-'),
			      (hg_quark_is_writable(qdata) ? 'w' : '-'),
			      (hg_quark_is_executable(qdata) ? 'x' : '-'),
			      q == Qnil ? "..." : cstr);

	hg_free(cstr);
	/* this is an instant object.
	 * surely no reference to the container.
	 * so it can be safely destroyed.
	 */
	hg_string_free(s, TRUE);

	return TRUE;
}

static hg_bool_t
_hg_vm_rs_real_dump(hg_quark_t   qdata,
		    hg_pointer_t user_data)
{
	hg_file_t *file = (hg_file_t *)user_data;

	hg_file_append_printf(file, "0x%016lx\n", qdata);

	return TRUE;
}

static hg_bool_t
_hg_vm_translate(hg_vm_t             *vm,
		 hg_vm_trans_flags_t  flags)
{
	hg_stack_t *estack, *ostack;
	hg_quark_t qdata, qresult;
	hg_int_t clock_count = 0;
	hg_bool_t translate_block = (flags & HG_TRANS_FLAG_BLOCK);
	hg_bool_t translate_token = (flags & HG_TRANS_FLAG_TOKEN);
	hg_bool_t translated, block_terminated = (translate_block ? FALSE : TRUE);

	ostack = vm->stacks[HG_VM_STACK_OSTACK];
	estack = vm->stacks[HG_VM_STACK_ESTACK];

	if (hg_stack_depth(estack) == 0) {
		hg_warning("No objects in the exec stack");
		return TRUE;
	}

	do {
		translated = FALSE;
		qdata = hg_stack_index(estack, 0);
		if (!HG_ERROR_IS_SUCCESS0 ())
			return FALSE;

		hg_debug(HG_MSGCAT_VM, "Processing 0x%lx [%d]", qdata, clock_count);
		clock_count++;

	  evaluate:
		switch (hg_quark_get_type(qdata)) {
		    case HG_TYPE_REAL:
			    if (isinf(HG_REAL (qdata)))
				    hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
		    case HG_TYPE_NULL:
		    case HG_TYPE_INT:
		    case HG_TYPE_BOOL:
		    case HG_TYPE_DICT:
		    case HG_TYPE_MARK:
		    case HG_TYPE_SNAPSHOT:
		    case HG_TYPE_GSTATE:
		    push_stack:
			    if (!hg_stack_push(ostack, qdata))
				    hg_error_return (HG_STATUS_FAILED, HG_e_stackoverflow);
			    hg_stack_drop(estack);
			    translated = TRUE;
			    break;
		    case HG_TYPE_EVAL_NAME:
			    if (hg_vm_quark_is_executable(vm, &qdata)) {
				    qresult = hg_vm_dstack_lookup(vm, qdata);
				    if (qresult == Qnil)
					    hg_error_return (HG_STATUS_FAILED, HG_e_undefined);

				    /* Re-evaluate the object immediately */
				    qdata = qresult;
				    hg_debug(HG_MSGCAT_VM, "Re-processing 0x%lx", qdata);
				    if (translate_block ||
					translate_token)
					    goto push_stack;
				    goto evaluate;
			    }
			    hg_warning("Immediately evaluated name object somehow does not have an exec bit turned on: %s", hg_name_lookup(qdata));
			    goto push_stack;
		    case HG_TYPE_NAME:
			    if (translate_block ||
				translate_token)
				    goto push_stack;
			    if (hg_vm_quark_is_executable(vm, &qdata)) {
				    hg_quark_t q;

				    qresult = hg_vm_dstack_lookup(vm, qdata);
				    if (qresult == Qnil)
					    hg_error_return (HG_STATUS_FAILED, HG_e_undefined);

				    if (hg_vm_quark_is_executable(vm, &qresult)) {
					    q = hg_vm_quark_copy(vm, qresult, NULL);
					    if (q == Qnil)
						    hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
				    } else {
					    q = qresult;
				    }
				    if (!hg_stack_push(estack, q))
					    hg_error_return (HG_STATUS_FAILED, HG_e_execstackoverflow);

				    hg_stack_exch(estack);
				    hg_stack_drop(estack);
				    break;
			    }
			    goto push_stack;
		    case HG_TYPE_ARRAY:
			    if (translate_block ||
				translate_token)
				    goto push_stack;
			    if (hg_vm_quark_is_executable(vm, &qdata)) {
				    hg_array_t *a = _HG_VM_LOCK (vm, qdata);
				    hg_error_reason_t e = 0;

				    if (!a)
					    hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
				    if (hg_array_length(a) > 0) {
					    hg_stack_t *s;

					    qresult = hg_array_get(a, 0);
					    if (!HG_ERROR_IS_SUCCESS0 ()) {
						    e = HG_ERROR_GET_REASON (hg_errno);
						    goto a_error;
					    }
					    if (hg_vm_quark_is_executable(vm, &qresult) &&
						(HG_IS_QNAME (qresult) ||
						 HG_IS_QOPER (qresult))) {
						    e = HG_e_execstackoverflow;
						    s = estack;
					    } else {
						    e = HG_e_stackoverflow;
						    s = ostack;
						    translated = TRUE;
					    }
					    if (hg_stack_push(s, qresult)) {
						    e = 0;
						    if (hg_array_length(a) == 1) {
							    if (s == estack)
								    hg_stack_exch(estack);
							    hg_stack_drop(estack);
							    translated = TRUE;
						    } else {
							    hg_array_remove(a, 0);
						    }
					    }
				    } else {
					    hg_stack_drop(estack);
					    translated = TRUE;
				    }
			      a_error:
				    _HG_VM_UNLOCK (vm, qdata);
				    if (e != 0)
					    hg_error_return (HG_STATUS_FAILED, e);
				    break;
			    }
			    goto push_stack;
		    case HG_TYPE_STRING:
			    if (translate_block ||
				translate_token)
				    goto push_stack;
			    if (hg_vm_quark_is_executable(vm, &qdata)) {
				    hg_string_t *s = _HG_VM_LOCK (vm, qdata);

				    if (!s)
					    hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
				    qresult = hg_file_new_with_string(_hg_vm_get_mem(vm),
								      "%sfile",
								      HG_FILE_IO_MODE_READ,
								      s,
								      NULL,
								      NULL);
				    if (qresult == Qnil)
					    hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
				    hg_vm_quark_set_executable(vm, &qresult, TRUE);
				    _HG_VM_UNLOCK (vm, qdata);
				    hg_stack_drop(estack);
				    if (!hg_stack_push(estack, qresult))
					    hg_error_return (HG_STATUS_FAILED, HG_e_execstackoverflow);
				    break;
			    }
			    goto push_stack;
		    case HG_TYPE_OPER:
			    if (translate_block ||
				translate_token)
				    goto push_stack;
			    if (hg_operator_invoke(qdata, vm)) {
				    hg_stack_drop(estack);
#ifdef HG_DEBUG
				    if (!HG_ERROR_IS_SUCCESS0 ()) {
					    hg_warning("status conflict in hg_errno");
				    }
#endif
			    } else {
				    if (HG_ERROR_IS_SUCCESS0 () && !hg_vm_has_error(vm)) {
					    hg_critical("No error set.");
					    hg_operator_invoke(HG_QOPER (HG_enc_private_abort), vm);
				    }
			    }
			    translated = TRUE;
			    break;
		    case HG_TYPE_FILE:
			    if (!hg_vm_quark_is_executable(vm, &qdata)) {
				    goto push_stack;
			    } else {
				    hg_file_t *f = _HG_VM_LOCK (vm, qdata);
				    const hg_char_t *name;

				    if (!f)
					    hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
				    hg_scanner_attach_file(vm->scanner, f);
				    if (!hg_scanner_scan(vm->scanner, vm)) {
					    hg_bool_t is_eof = hg_file_is_eof(f);

					    _HG_VM_UNLOCK (vm, qdata);
					    if (HG_ERROR_IS_SUCCESS0 () &&
						is_eof &&
						!translate_block) {
						    hg_debug(HG_MSGCAT_VM, "EOF detected");
						    hg_stack_drop(estack);

						    return FALSE;
					    }
					    hg_error_return (HG_STATUS_FAILED, HG_e_syntaxerror);
				    }
				    _HG_VM_UNLOCK (vm, qdata);
				    qresult = hg_scanner_get_token(vm->scanner);
				    hg_vm_quark_set_default_acl(vm, &qresult);

				    /* exception for processing the executable array */
				    if (HG_IS_QNAME (qresult) &&
					(name = hg_name_lookup(qresult)) != NULL) {
					    if (!strcmp(name, "{")) {
						    qresult = _hg_vm_translate_block(vm, flags);
						    if (qresult == Qnil) {
							    translated = TRUE;
							    break;
						    }
						    if (!hg_stack_push(ostack, qresult))
							    hg_error_return (HG_STATUS_FAILED, HG_e_stackoverflow);
						    translated = TRUE;
						    break;
					    } else if (!strcmp(name, "}")) {
						    hg_size_t i, idx = 0;
						    hg_quark_t q;
						    hg_array_t *a;

						    if (!translate_block)
							    hg_error_return (HG_STATUS_FAILED, HG_e_syntaxerror);

						    idx = hg_stack_depth(ostack);
						    qresult = hg_array_new(_hg_vm_get_mem(vm),
									   idx,
									   (hg_pointer_t *)&a);
						    if (qresult == Qnil)
							    return FALSE;
						    hg_vm_quark_set_default_acl(vm, &qresult);
						    hg_vm_quark_set_executable(vm, &qresult, TRUE);
						    for (i = idx - 1; i >= 0; i--) {
							    q = hg_stack_index(ostack, i);
							    if (!HG_ERROR_IS_SUCCESS0 ()) {
								    _HG_VM_UNLOCK (vm, qresult);
								    return FALSE;
							    }
							    if (!hg_array_set(a, q, idx - i - 1, FALSE)) {
								    _HG_VM_UNLOCK (vm, qresult);
								    return FALSE;
							    }
						    }
						    _HG_VM_UNLOCK (vm, qresult);

						    for (i = 0; i <= idx; i++) {
							    hg_stack_drop(ostack);
							    if (!HG_ERROR_IS_SUCCESS0 ())
								    return FALSE;
						    }
						    if (!hg_stack_push(ostack, qresult))
							    return FALSE;

						    translated = TRUE;
						    block_terminated = TRUE;
						    break;
					    }
				    }
				    hg_stack_push(estack, qresult);
			    }
			    break;
		    default:
			    hg_warning("Unknown object type: %d", hg_quark_get_type(qdata));
			    hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
		}
	} while (!(translated && block_terminated));

	return HG_ERROR_IS_SUCCESS0 ();
}

static hg_quark_t
_hg_vm_translate_block(hg_vm_t             *vm,
		       hg_vm_trans_flags_t  flags)
{
	hg_stack_t *old_ostack;
	hg_quark_t retval = Qnil;

	vm->n_nest_scan++;

	old_ostack = vm->stacks[HG_VM_STACK_OSTACK];
	vm->stacks[HG_VM_STACK_OSTACK] = hg_vm_stack_new(vm, 65535);
	if (vm->stacks[HG_VM_STACK_OSTACK] == NULL) {
		/* unable to duplicate the stack */
		vm->stacks[HG_VM_STACK_OSTACK] = old_ostack;
		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_VMerror);
	}

	_hg_vm_translate(vm, flags | HG_TRANS_FLAG_BLOCK);
	if (HG_ERROR_IS_SUCCESS0 ()) {
		retval = hg_stack_index(vm->stacks[HG_VM_STACK_OSTACK], 0);
	}

	hg_stack_free(vm->stacks[HG_VM_STACK_OSTACK]);
	vm->stacks[HG_VM_STACK_OSTACK] = old_ostack;

	vm->n_nest_scan--;

	return retval;
}

static hg_bool_t
_hg_vm_eval(hg_vm_t    *vm,
	    hg_quark_t  qeval,
	    hg_bool_t   protect_systemdict)
{
	hg_stack_t *estack = NULL, *dstack = NULL, *old_estack = NULL, *old_dstack = NULL;
	hg_quark_t old_systemdict = Qnil;
	hg_bool_t retval = FALSE, old_state_hold_langlevel = FALSE;
	hg_vm_langlevel_t lang = 0;

	if (hg_quark_is_simple_object(qeval)) {
		/* XXX */
		return FALSE;
	}
	switch(hg_quark_get_type(qeval)) {
	    case HG_TYPE_STRING:
	    case HG_TYPE_FILE:
		    if (!hg_vm_quark_is_executable(vm, &qeval)) {
			    /* XXX */
			    return FALSE;
		    }
		    break;
	    default:
		    /* XXX */
		    return FALSE;
	}

	if (protect_systemdict) {
		hg_dict_t *d, *o;
		hg_bool_t flag = TRUE;

		old_systemdict = vm->qsystemdict;
		old_state_hold_langlevel = vm->hold_lang_level;
		lang = hg_vm_get_language_level(vm);
		o = _HG_VM_LOCK (vm, old_systemdict);
		if (o == NULL) {
			hg_warning("Unable to protect systemdict.");
			return FALSE;
		}
		vm->qsystemdict = hg_dict_dup(o, (hg_pointer_t *)&d);
		if (vm->qsystemdict == Qnil) {
			hg_warning("Unable to duplicate systemdict.");
			flag = FALSE;
			goto fini_systemdict;
		}
		hg_vm_hold_language_level(vm, FALSE);
		if (!hg_dict_add(d,
				 HG_QNAME ("systemdict"),
				 vm->qsystemdict,
				 FALSE)) {
			hg_warning("Unable to reregister systemdict");
			flag = FALSE;
			goto fini_systemdict;
		}

		/* save current dict stack */
		dstack = hg_vm_stack_new(vm, vm->user_params.max_dict_stack);
		hg_stack_foreach(vm->stacks[HG_VM_STACK_DSTACK], _hg_vm_dup_stack, dstack, TRUE);
		old_dstack = vm->stacks[HG_VM_STACK_DSTACK];
		g_ptr_array_add(vm->stacks_stack, old_dstack);
		vm->stacks[HG_VM_STACK_DSTACK] = dstack;

		/* save current exec stack */
		estack = hg_vm_stack_new(vm, vm->user_params.max_exec_stack);
		old_estack = vm->stacks[HG_VM_STACK_ESTACK];
		g_ptr_array_add(vm->stacks_stack, old_estack);
		vm->stacks[HG_VM_STACK_ESTACK] = estack;

	  fini_systemdict:
		_HG_VM_UNLOCK (vm, old_systemdict);
		_HG_VM_UNLOCK (vm, vm->qsystemdict);

		if (!flag)
			goto finalize;
	}

	hg_stack_push(vm->stacks[HG_VM_STACK_ESTACK], qeval);
	retval = hg_vm_main_loop(vm);

	if (old_estack) {
		vm->stacks[HG_VM_STACK_ESTACK] = old_estack;
		g_ptr_array_remove(vm->stacks_stack, old_estack);
		hg_stack_free(estack);
	}
	if (old_dstack) {
		vm->stacks[HG_VM_STACK_DSTACK] = old_dstack;
		g_ptr_array_remove(vm->stacks_stack, old_dstack);
		hg_stack_free(dstack);
	}
  finalize:
	if (protect_systemdict) {
		vm->qsystemdict = old_systemdict;
		if (hg_vm_get_language_level(vm) != lang) {
			hg_vm_startjob(vm, lang, "", FALSE);
		}
		hg_vm_hold_language_level(vm, old_state_hold_langlevel);
	}

	return retval;
}

static hg_quark_t
_hg_vm_quark_iterate_copy(hg_quark_t    qdata,
			  hg_pointer_t  user_data,
			  hg_pointer_t *ret)
{
	return hg_vm_quark_copy((hg_vm_t *)user_data, qdata, ret);
}

static hg_quark_t
_hg_vm_quark_iterate_to_cstr(hg_quark_t    qdata,
			     hg_pointer_t  user_data,
			     hg_pointer_t *ret)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;
	hg_string_t *s = NULL;
	hg_char_t *cstr = NULL;

	hg_vm_quark_to_string(vm, qdata, TRUE, (hg_pointer_t *)&s);
	if (s) {
		cstr = hg_string_get_cstr(s);
	}
	/* this is an instant object.
	 * surely no reference to the container.
	 * so it can be safely destroyed.
	 */
	hg_string_free(s, TRUE);

	return HGPOINTER_TO_QUARK (cstr);
}

static hg_bool_t
_hg_vm_quark_iterate_compare(hg_quark_t   q1,
			     hg_quark_t   q2,
			     hg_pointer_t user_data)
{
	return hg_vm_quark_compare_content((hg_vm_t *)user_data,
					   q1, q2);
}

static hg_bool_t
_hg_vm_quark_complex_compare(hg_quark_t   q1,
			     hg_quark_t   q2,
			     hg_pointer_t user_data)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;
	hg_bool_t retval = FALSE;

	switch (hg_quark_get_type(q1)) {
	    case HG_TYPE_STRING:
		    G_STMT_START {
			    hg_string_t *s1, *s2;

			    s1 = _HG_VM_LOCK (vm, q1);
			    s2 = _HG_VM_LOCK (vm, q2);
			    if (s1 == NULL ||
				s2 == NULL) {
				    hg_warning("%s: Unable to obtain the actual object.", __PRETTY_FUNCTION__);
			    } else {
				    retval = hg_string_compare(s1, s2);
			    }
			    if (s1)
				    _HG_VM_UNLOCK (vm, q1);
			    if (s2)
				    _HG_VM_UNLOCK (vm, q2);
		    } G_STMT_END;
		    break;
	    case HG_TYPE_DICT:
	    case HG_TYPE_ARRAY:
	    case HG_TYPE_FILE:
	    case HG_TYPE_SNAPSHOT:
	    case HG_TYPE_STACK:
		    retval = (q1 == q2);
		    break;
	    default:
		    g_warn_if_reached();
		    break;
	}

	return retval;
}

static hg_bool_t
_hg_vm_quark_complex_compare_content(hg_quark_t   q1,
				     hg_quark_t   q2,
				     hg_pointer_t user_data)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;
	hg_bool_t retval;
	hg_object_t *o1, *o2;

	o1 = _HG_VM_LOCK (vm, q1);
	o2 = _HG_VM_LOCK (vm, q2);

	retval = hg_object_compare(o1, o2, _hg_vm_quark_iterate_compare, vm);

	_HG_VM_UNLOCK (vm, q1);
	_HG_VM_UNLOCK (vm, q2);

	return retval;
}

static hg_bool_t
_hg_vm_run_gc(hg_mem_t     *mem,
	      hg_pointer_t  user_data)
{
	hg_vm_t *vm = user_data;
	hg_file_t *f;
	hg_usize_t i;
	GList *l;

	hg_return_val_if_fail (mem != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	/* marking objects */
	/** marking I/O **/
	hg_debug(HG_MSGCAT_GC, "VM: marking file objects");
	for (i = 0; i < HG_FILE_IO_END; i++) {
		if (!hg_vm_quark_gc_mark(vm, vm->qio[i]))
			return FALSE;
	}
	if (vm->lineedit) {
		if (!hg_lineedit_gc_mark(vm->lineedit))
			return FALSE;
	}
	/* marking I/O in scanner */
	f = hg_scanner_get_infile(vm->scanner);
	if (f) {
		if (!hg_mem_gc_mark(f->o.mem, f->o.self))
			return FALSE;
	}

	/** marking in stacks **/
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in stacks");
	for (i = 0; i < HG_VM_STACK_END; i++) {
		hg_object_t *o = (hg_object_t *)vm->stacks[i];

		if (!hg_mem_gc_mark(o->mem,
				    o->self))
			return FALSE;
	}
	for (i = 0; i < vm->stacks_stack->len; i++) {
		hg_object_t *o = (hg_object_t *)g_ptr_array_index(vm->stacks_stack, i);

		if (!hg_mem_gc_mark(o->mem, o->self))
			return FALSE;
	}
	/** marking plugins **/
	for (l = vm->plugin_list; l != NULL; l = g_list_next(l)) {
		hg_plugin_t *p = l->data;

		if (!hg_mem_gc_mark(p->mem, p->self))
			return FALSE;
	}
	/** marking miscellaneous **/
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in vm state");
	if (!hg_mem_gc_mark(vm->mem[HG_VM_MEM_LOCAL], vm->vm_state->self))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in $error");
	if (!hg_vm_quark_gc_mark(vm, vm->qerror))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in systemdict");
	if (!hg_vm_quark_gc_mark(vm, vm->qsystemdict))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in globaldict");
	if (!hg_vm_quark_gc_mark(vm, vm->qglobaldict))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in internaldict");
	if (!hg_vm_quark_gc_mark(vm, vm->qinternaldict))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in gstate");
	if (!hg_vm_quark_gc_mark(vm, vm->qgstate))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "VM: marking objects in device");
	if (vm->device) {
		if (!hg_device_gc_mark(vm->device))
			return FALSE;
	}


	/* sweeping objects */

	return TRUE;
}

static hg_bool_t
_hg_vm_dup_stack(hg_mem_t     *mem,
		 hg_quark_t    qdata,
		 hg_pointer_t  data)
{
	hg_stack_t *s = data;

	return hg_stack_push(s, qdata);
}

static hg_bool_t
_hg_vm_set_user_params(hg_mem_t     *mem,
		       hg_quark_t    qkey,
		       hg_quark_t    qval,
		       hg_pointer_t  data)
{
	hg_vm_t *vm = (hg_vm_t *)data;
	hg_usize_t i;

	if (HG_IS_QNAME (qkey)) {
		for (i = HG_VM_user_BEGIN + 1; i < HG_VM_user_END; i++) {
			if (hg_quark_get_hash(vm->quparams_name[i]) == hg_quark_get_hash(qkey))
				break;
		}
		switch (i) {
		    case HG_VM_user_MaxOpStack:
			    if (HG_IS_QINT (qval)) {
				    vm->user_params.max_op_stack = HG_INT (qval);
				    hg_stack_set_max_depth(vm->stacks[HG_VM_STACK_OSTACK],
							   vm->user_params.max_op_stack);
			    }
			    break;
		    case HG_VM_user_MaxExecStack:
			    if (HG_IS_QINT (qval)) {
				    vm->user_params.max_exec_stack = HG_INT (qval);
				    hg_stack_set_max_depth(vm->stacks[HG_VM_STACK_ESTACK],
							   vm->user_params.max_exec_stack);
			    }
			    break;
		    case HG_VM_user_MaxDictStack:
			    if (HG_IS_QINT (qval)) {
				    vm->user_params.max_dict_stack = HG_INT (qval);
				    hg_stack_set_max_depth(vm->stacks[HG_VM_STACK_DSTACK],
							   vm->user_params.max_dict_stack);
			    }
			    break;
		    case HG_VM_user_MaxGStateStack:
			    if (HG_IS_QINT (qval)) {
				    vm->user_params.max_gstate_stack = HG_INT (qval);
				    hg_stack_set_max_depth(vm->stacks[HG_VM_STACK_GSTATE],
							   vm->user_params.max_gstate_stack);
			    }
			    break;
		    default:
			    break;
		}
	}

	return TRUE;
}

static hg_bool_t
_hg_vm_restore_mark_traverse(hg_mem_t     *mem,
			     hg_quark_t    qdata,
			     hg_pointer_t  data)
{
	hg_uint_t id = hg_quark_get_mem_id(qdata);
	hg_mem_t *m = hg_mem_spool_get(id);
	hg_snapshot_t *snapshot = (hg_snapshot_t *)data;

	if (m == NULL) {
		hg_warning("No memory spool found: %d", id);

		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	hg_snapshot_add_ref(snapshot, qdata);

	return TRUE;
}

static hg_bool_t
_hg_vm_restore_mark(hg_snapshot_t *snapshot,
		    hg_pointer_t   data)
{
	hg_vm_t *vm = (hg_vm_t *)data;
	hg_usize_t i;

	for (i = 0; i <= HG_VM_STACK_DSTACK; i++) {
		hg_stack_foreach(vm->stacks[i],
				 _hg_vm_restore_mark_traverse,
				 snapshot, FALSE);
		if (!HG_ERROR_IS_SUCCESS0 ())
			return FALSE;
	}

	return TRUE;
}

static hg_bool_t
_hg_vm_setup(hg_vm_t           *vm,
	     hg_vm_langlevel_t  lang_level)
{
	hg_dict_t *dict = NULL;
	hg_usize_t i;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (lang_level < HG_LANG_LEVEL_END, FALSE, HG_e_VMerror);

	hg_vm_use_global_mem(vm, TRUE);
	vm->n_nest_scan = 0;

	for (i = 0; i < HG_VM_MEM_END; i++) {
		/* Even if the memory spool expansion wasn't supported in PS1,
		 * following up on that spec here isn't a good idea because
		 * this isn't entire clone of Adobe's and the memory management
		 * should be totally different.
		 * So OOM is likely happening on only hieroglyph.
		 */
		hg_mem_spool_set_resizable(vm->mem[i], TRUE);
	}

	hg_vm_quark_set_acl(vm, &vm->qsystemdict, HG_ACL_READABLE|HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);
	hg_vm_quark_set_acl(vm, &vm->qinternaldict, HG_ACL_READABLE|HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);
	hg_vm_quark_set_acl(vm, &vm->qglobaldict, HG_ACL_READABLE|HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);
	hg_vm_quark_set_acl(vm, &vm->qerror, HG_ACL_READABLE|HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);

	hg_stack_push(vm->stacks[HG_VM_STACK_DSTACK], vm->qsystemdict);
	if (lang_level >= HG_LANG_LEVEL_2) {
		hg_stack_push(vm->stacks[HG_VM_STACK_DSTACK],
			      vm->qglobaldict);
	}
	/* userdict will be created/pushed in PostScript */

	/* initialize $error */
	dict = _HG_VM_LOCK (vm, vm->qerror);
	if (dict) {
		if (!hg_dict_add(dict,
				 HG_QNAME ("newerror"),
				 HG_QBOOL (FALSE),
				 FALSE)) {
			_HG_VM_UNLOCK (vm, vm->qerror);
			goto bail;
		}
		if (!hg_dict_add(dict,
				 HG_QNAME ("errorname"),
				 HG_QNULL,
				 FALSE)) {
			_HG_VM_UNLOCK (vm, vm->qerror);
			goto bail;
		}
		if (!hg_dict_add(dict,
				 HG_QNAME (".stopped"),
				 HG_QBOOL (FALSE),
				 FALSE)) {
			_HG_VM_UNLOCK (vm, vm->qerror);
			goto bail;
		}
		_HG_VM_UNLOCK (vm, vm->qerror);
	} else {
		goto bail;
	}

	/* add built-in items into systemdict */
	dict = _HG_VM_LOCK (vm, vm->qsystemdict);
	if (dict) {
		if (!hg_dict_add(dict,
				 HG_QNAME ("systemdict"),
				 vm->qsystemdict,
				 TRUE)) {
			_HG_VM_UNLOCK (vm, vm->qsystemdict);
			goto bail;
		}
		if (!hg_dict_add(dict,
				 HG_QNAME ("globaldict"),
				 vm->qglobaldict,
				 TRUE)) {
			_HG_VM_UNLOCK (vm, vm->qsystemdict);
			goto bail;
		}
		if (!hg_dict_add(dict,
				 HG_QNAME ("$error"),
				 vm->qerror,
				 TRUE)) {
			_HG_VM_UNLOCK (vm, vm->qsystemdict);
			goto bail;
		}

		/* initialize build-in operators */
		if (!hg_operator_register(vm, dict, lang_level)) {
			_HG_VM_UNLOCK (vm, vm->qsystemdict);
			goto bail;
		}
		_HG_VM_UNLOCK (vm, vm->qsystemdict);
	} else {
		goto bail;
	}

	vm->language_level = lang_level;

	return TRUE;
  bail:
	_hg_vm_finish(vm);

	return FALSE;
}

HG_INLINE_FUNC void
_hg_vm_finish(hg_vm_t *vm)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->hold_lang_level = FALSE;
}

static hg_bool_t
_hg_vm_set_error(hg_vm_t           *vm,
		 hg_quark_t         qdata,
		 hg_error_reason_t  error)
{
	hg_quark_t qerrordict, qnerrordict, qhandler, q;
	hg_usize_t i;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (error < HG_e_END, FALSE, HG_e_VMerror);

	if (vm->has_error) {
		hg_quark_t qerrorname = HG_QNAME ("errorname");
		hg_quark_t qcommand = HG_QNAME ("command");
		hg_quark_t qresult_err, qresult_cmd, qwhere;
		hg_dict_t *derror;
		hg_string_t *where;
		hg_char_t *scommand, *swhere;

		derror = _HG_VM_LOCK (vm, vm->qerror);
		if (derror == NULL)
			goto fatal_error;
		qresult_err = hg_dict_lookup(derror, qerrorname);
		qresult_cmd = hg_dict_lookup(derror, qcommand);
		_HG_VM_UNLOCK (vm, vm->qerror);

		if (qresult_cmd == Qnil) {
			scommand = hg_strdup("-%unknown%-");
		} else {
			hg_string_t *s = NULL;

			q = hg_vm_quark_to_string(vm,
						  qresult_cmd,
						  TRUE,
						  (hg_pointer_t *)&s);
			if (q == Qnil)
				scommand = hg_strdup("-%ENOMEM%-");
			else
				scommand = hg_string_get_cstr(s);
			/* this is an instant object.
			 * surely no reference to the container.
			 * so it can be safely destroyed.
			 */
			hg_string_free(s, TRUE);
		}
		qwhere = hg_vm_quark_to_string(vm,
					       qdata,
					       TRUE,
					       (hg_pointer_t *)&where);
		if (qwhere == Qnil)
			swhere = hg_strdup("-%ENOMEM%-");
		else
			swhere = hg_string_get_cstr(where);
		_hg_vm_quark_free(vm, qwhere);

		if (qresult_err == Qnil) {
			g_printerr("Multiple errors occurred.\n"
				   "  previous error: unknown or this happened at %s prior to set /errorname\n"
				   "  current error: %s at %s\n",
				   scommand,
				   hg_name_lookup(vm->qerror_name[error]),
				   swhere);
		} else {
			g_printerr("Multiple errors occurred.\n"
				   "  previous error: %s at %s\n"
				   "  current error: %s at %s\n",
				   hg_name_lookup(qresult_err),
				   scommand,
				   hg_name_lookup(vm->qerror_name[error]),
				   swhere);
		}
		hg_free(swhere);
		hg_free(scommand);

		hg_operator_invoke(HG_QOPER (HG_enc_private_abort), vm);
	}
	for (i = 0; i < HG_VM_STACK_END; i++) {
		hg_stack_set_validation(vm->stacks[i], FALSE);
	}
	qnerrordict = HG_QNAME ("errordict");
	qerrordict = hg_vm_dstack_lookup(vm, qnerrordict);
	if (qerrordict == Qnil) {
		hg_critical("Unable to lookup errordict");

		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		goto fatal_error;
	}
	qhandler = hg_vm_dict_lookup(vm, qerrordict, vm->qerror_name[error], FALSE);
	if (qhandler == Qnil) {
		hg_critical("Unable to obtain the error handler for %s",
			    hg_name_lookup(vm->qerror_name[error]));
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		goto fatal_error;
	}

	q = hg_vm_quark_copy(vm, qhandler, NULL);
	if (q == Qnil)
		goto fatal_error;
	hg_stack_push(vm->stacks[HG_VM_STACK_ESTACK], q);
	hg_stack_push(vm->stacks[HG_VM_STACK_OSTACK], qdata);
	vm->has_error = TRUE;

	return TRUE;
  fatal_error:
	G_STMT_START {
		const hg_char_t *errname = hg_name_lookup(vm->qerror_name[error]);

		if (errname) {
			hg_critical("Fatal error during recovering from /%s", errname);
		} else {
			hg_critical("Fatal error during recovering from the unknown error");
		}

		return hg_operator_invoke(HG_QOPER (HG_enc_private_abort), vm);
	} G_STMT_END;
}

/*< public >*/
/**
 * hg_vm_init:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_init(void)
{
	if (!__hg_vm_is_initialized) {
		__hg_vm_mem = hg_mem_spool_new(HG_MEM_TYPE_MASTER,
					       HG_VM_MEM_SIZE);
		if (__hg_vm_mem == NULL)
			hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);

		__hg_vm_is_initialized = TRUE;

		/* initialize objects */
		hg_object_init();
		HG_ARRAY_INIT;
		HG_DICT_INIT;
		HG_FILE_INIT;
		HG_SNAPSHOT_INIT;
		HG_STACK_INIT;
		HG_STRING_INIT;
		HG_PATH_INIT;
		HG_GSTATE_INIT;
		hg_name_init();
		if (!hg_encoding_init())
			goto error;
		if (!hg_operator_init())
			goto error;
	}

	hg_error_return (HG_STATUS_SUCCESS, 0);
  error:
	hg_vm_tini();

	hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
}

/**
 * hg_vm_tini:
 *
 * FIXME
 */
void
hg_vm_tini(void)
{
	if (__hg_vm_is_initialized) {
		hg_operator_tini();
		hg_encoding_tini();
		hg_name_tini();
		hg_object_tini();

		if (__hg_vm_mem)
			hg_mem_spool_destroy(__hg_vm_mem);
		__hg_vm_is_initialized = FALSE;
	}
}

/**
 * hg_vm_new:
 *
 * FIXME
 *
 * Returns:
 */
hg_vm_t *
hg_vm_new(void)
{
	hg_quark_t q;
	hg_vm_t *retval;
	hg_usize_t i;
	hg_mem_t *gmem, *lmem;
	hg_file_t *fstdin = NULL, *fstdout = NULL;

	q = hg_mem_alloc(__hg_vm_mem, sizeof (hg_vm_t), (hg_pointer_t *)&retval);
	if (q == Qnil)
		return NULL;

	hg_mem_ref(__hg_vm_mem, q);

	memset(retval, 0, sizeof (hg_vm_t));
	retval->self = q;

	/* initialize */
	/** initialize quarks **/
	for (i = 0; i < HG_FILE_IO_END; i++)
		retval->qio[i] = Qnil;
	for (i = 0; i < HG_e_END; i++)
		retval->qerror_name[i] = Qnil;
	for (i = 0; i < HG_VM_user_END; i++)
		retval->quparams_name[i] = Qnil;
	for (i = 0; i < HG_VM_sys_END; i++)
		retval->qsparams_name[i] = Qnil;
	for (i = 0; i < HG_pdev_END; i++)
		retval->qpdevparams_name[i] = Qnil;
	retval->qerror = Qnil;
	retval->qsystemdict = Qnil;
	retval->qglobaldict = Qnil;
	retval->qinternaldict = Qnil;
	retval->qgstate = Qnil;
	/** memory spool **/
	gmem = retval->mem[HG_VM_MEM_GLOBAL] = hg_mem_spool_new(HG_MEM_TYPE_GLOBAL,
								HG_VM_GLOBAL_MEM_SIZE);
	lmem = retval->mem[HG_VM_MEM_LOCAL] = hg_mem_spool_new(HG_MEM_TYPE_LOCAL,
							       HG_VM_LOCAL_MEM_SIZE);
	if (!gmem || !lmem)
		goto bail;
	retval->mem_id[HG_VM_MEM_GLOBAL] = hg_mem_get_id(gmem);
	retval->mem_id[HG_VM_MEM_LOCAL] = hg_mem_get_id(lmem);
	hg_mem_spool_set_gc_procedure(gmem, _hg_vm_run_gc, retval);
	hg_mem_spool_set_gc_procedure(lmem, _hg_vm_run_gc, retval);
	/** vm_state **/
	q = hg_mem_alloc(lmem, sizeof (hg_vm_state_t),
			 (hg_pointer_t *)&retval->vm_state);
	if (q == Qnil)
		goto bail;
	retval->vm_state->self = q;
	retval->vm_state->current_mem_index = HG_VM_MEM_GLOBAL;
	retval->vm_state->n_save_objects = 0;
	/** user_params **/
	retval->user_params.max_op_stack = 500;
	retval->user_params.max_exec_stack = 250;
	retval->user_params.max_dict_stack = 20;
	retval->user_params.max_gstate_stack = 31;
	/** I/O **/
	q = hg_file_new(gmem, "%stdin",
			HG_FILE_IO_MODE_READ,
			(hg_pointer_t *)&fstdin);
	if (q == Qnil)
		goto bail;
	hg_vm_quark_set_acl(retval, &q,
			    HG_ACL_READABLE|HG_ACL_ACCESSIBLE);
	hg_vm_set_io(retval, HG_FILE_IO_STDIN, q);

	q = hg_file_new(gmem, "%stdout",
			HG_FILE_IO_MODE_WRITE,
			(hg_pointer_t *)&fstdout);
	if (q == Qnil)
		goto bail;
	hg_vm_quark_set_acl(retval, &q,
			    HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);
	hg_vm_set_io(retval, HG_FILE_IO_STDOUT, q);

	q = hg_file_new(gmem, "%stderr",
			HG_FILE_IO_MODE_WRITE,
			NULL);
	if (q == Qnil)
		goto bail;
	hg_vm_quark_set_acl(retval, &q,
			    HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);
	hg_vm_set_io(retval, HG_FILE_IO_STDERR, q);

	retval->lineedit = hg_lineedit_new(gmem,
					   hg_lineedit_get_default_vtable(),
					   fstdin,
					   fstdout);
	if (!retval->lineedit)
		goto bail;
	/** gstate **/
	retval->qgstate = hg_gstate_new(lmem, NULL);
	if (retval->qgstate == Qnil)
		goto bail;
	hg_mem_unref(lmem, retval->qgstate);
	/** dictionaries **/
	retval->qsystemdict = hg_dict_new(gmem, 65535, FALSE, NULL);
	retval->qinternaldict = hg_dict_new(gmem, 65535, FALSE, NULL);
	retval->qglobaldict = hg_dict_new(gmem, 65535, FALSE, NULL);
	retval->qerror = hg_dict_new(lmem, 65535, FALSE, NULL);
	if (retval->qsystemdict == Qnil ||
	    retval->qinternaldict == Qnil ||
	    retval->qglobaldict == Qnil ||
	    retval->qerror == Qnil)
		goto bail;
	hg_mem_unref(gmem, retval->qsystemdict);
	hg_mem_unref(gmem, retval->qinternaldict);
	hg_mem_unref(gmem, retval->qglobaldict);
	hg_mem_unref(gmem, retval->qerror);
	/** randam seed **/
	retval->rand_ = g_rand_new();
	retval->rand_seed = g_rand_int(retval->rand_);
	g_rand_set_seed(retval->rand_, retval->rand_seed);
	g_get_current_time(&retval->initiation_time);

	retval->params = g_hash_table_new_full(g_str_hash, g_str_equal,
					       hg_free, hg_vm_value_free);
	retval->plugin_table = g_hash_table_new_full(g_str_hash, g_str_equal,
						     hg_free, NULL);
	/** scanner **/
	/* XXX: we prefer not to use the global nor the local
	 * memory spool to postpone thinking of how to deal
	 * with the yacc instance on GC.
	 */
	retval->scanner = hg_scanner_new(__hg_vm_mem);
	if (retval->scanner == NULL)
		goto bail;
	/** stacks **/
	retval->stack_spooler = hg_stack_spooler_new(__hg_vm_mem);
	retval->stacks[HG_VM_STACK_OSTACK] = hg_vm_stack_new(retval,
							     retval->user_params.max_op_stack);
	retval->stacks[HG_VM_STACK_ESTACK] = hg_vm_stack_new(retval,
							     retval->user_params.max_exec_stack);
	retval->stacks[HG_VM_STACK_DSTACK] = hg_vm_stack_new(retval,
							     retval->user_params.max_dict_stack);
	retval->stacks[HG_VM_STACK_GSTATE] = hg_stack_new(retval->mem[HG_VM_MEM_LOCAL],
							  retval->user_params.max_gstate_stack,
							  retval);
	if (retval->stacks[HG_VM_STACK_OSTACK] == NULL ||
	    retval->stacks[HG_VM_STACK_ESTACK] == NULL ||
	    retval->stacks[HG_VM_STACK_DSTACK] == NULL ||
	    retval->stacks[HG_VM_STACK_GSTATE] == NULL)
		goto bail;

	retval->stacks_stack = g_ptr_array_new();

	hg_vm_set_default_acl(retval, HG_ACL_READABLE|HG_ACL_WRITABLE|HG_ACL_ACCESSIBLE);

#define DECL_ERROR(_v_,_n_)					\
	(_v_)->qerror_name[HG_e_ ## _n_] = HG_QNAME (#_n_);

	DECL_ERROR (retval, dictfull);
	DECL_ERROR (retval, dictstackoverflow);
	DECL_ERROR (retval, dictstackunderflow);
	DECL_ERROR (retval, execstackoverflow);
	DECL_ERROR (retval, handleerror);
	DECL_ERROR (retval, interrupt);
	DECL_ERROR (retval, invalidaccess);
	DECL_ERROR (retval, invalidexit);
	DECL_ERROR (retval, invalidfileaccess);
	DECL_ERROR (retval, invalidfont);
	DECL_ERROR (retval, invalidrestore);
	DECL_ERROR (retval, ioerror);
	DECL_ERROR (retval, limitcheck);
	DECL_ERROR (retval, nocurrentpoint);
	DECL_ERROR (retval, rangecheck);
	DECL_ERROR (retval, stackoverflow);
	DECL_ERROR (retval, stackunderflow);
	DECL_ERROR (retval, syntaxerror);
	DECL_ERROR (retval, timeout);
	DECL_ERROR (retval, typecheck);
	DECL_ERROR (retval, undefined);
	DECL_ERROR (retval, undefinedfilename);
	DECL_ERROR (retval, undefinedresult);
	DECL_ERROR (retval, unmatchedmark);
	DECL_ERROR (retval, unregistered);
	DECL_ERROR (retval, VMerror);
	DECL_ERROR (retval, configurationerror);
	DECL_ERROR (retval, undefinedresource);

#undef DECL_ERROR

#define DECL_UPARAM(_v_,_n_)						\
	(_v_)->quparams_name[HG_VM_user_ ## _n_] = HG_QNAME (#_n_);

	DECL_UPARAM (retval, MaxOpStack);
	DECL_UPARAM (retval, MaxExecStack);
	DECL_UPARAM (retval, MaxDictStack);
	DECL_UPARAM (retval, MaxGStateStack);

#undef DECL_UPARAM

#define DECL_SPARAM(_v_,_n_)						\
	(_v_)->qsparams_name[HG_VM_sys_ ## _n_] = HG_QNAME (#_n_);

#undef DECL_SPARAM

#define DECL_PDPARAM(_v_,_n_)						\
	(_v_)->qpdevparams_name[HG_pdev_ ## _n_] = HG_QNAME (#_n_);

	DECL_PDPARAM (retval, InputAttributes);
	DECL_PDPARAM (retval, PageSize);
	DECL_PDPARAM (retval, MediaColor);
	DECL_PDPARAM (retval, MediaWeight);
	DECL_PDPARAM (retval, MediaType);
	DECL_PDPARAM (retval, ManualFeed);
	DECL_PDPARAM (retval, Orientation);
	DECL_PDPARAM (retval, AdvanceMedia);
	DECL_PDPARAM (retval, AdvanceDistance);
	DECL_PDPARAM (retval, CutMedia);
	DECL_PDPARAM (retval, HWResolution);
	DECL_PDPARAM (retval, ImagingBBox);
	DECL_PDPARAM (retval, Margins);
	DECL_PDPARAM (retval, MirrorPrint);
	DECL_PDPARAM (retval, NegativePrint);
	DECL_PDPARAM (retval, Duplex);
	DECL_PDPARAM (retval, Tumble);
	DECL_PDPARAM (retval, OutputType);
	DECL_PDPARAM (retval, NumCopies);
	DECL_PDPARAM (retval, Collate);
	DECL_PDPARAM (retval, Jog);
	DECL_PDPARAM (retval, OutputFaceUp);
	DECL_PDPARAM (retval, Separations);
	DECL_PDPARAM (retval, Install);
	DECL_PDPARAM (retval, BeginPage);
	DECL_PDPARAM (retval, EndPage);

#undef DECL_PDPARAM

	_HG_VM_UNLOCK (retval,
		       retval->qio[HG_FILE_IO_STDIN]);
	_HG_VM_UNLOCK (retval,
		       retval->qio[HG_FILE_IO_STDOUT]);

	return retval;
  bail:
	if (fstdin)
		_HG_VM_UNLOCK (retval,
			       retval->qio[HG_FILE_IO_STDIN]);
	if (fstdout)
		_HG_VM_UNLOCK (retval,
			       retval->qio[HG_FILE_IO_STDOUT]);
	hg_vm_destroy(retval);

	return NULL;
}

/**
 * hg_vm_destroy:
 * @vm:
 *
 * FIXME
 */
void
hg_vm_destroy(hg_vm_t *vm)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_usize_t i;

	if (vm->device)
		hg_device_close(vm->device);
	hg_scanner_destroy(vm->scanner);
	hg_lineedit_destroy(vm->lineedit);
	for (i = 0; i < HG_VM_STACK_END; i++) {
		if (vm->stacks[i])
			hg_stack_free(vm->stacks[i]);
	}
	hg_stack_spooler_destroy(vm->stack_spooler);
	for (i = 0; i < HG_FILE_IO_END; i++) {
		if (vm->qio[i] != Qnil)
			_hg_vm_quark_free(vm, vm->qio[i]);
	}
	for (i = 0; i < HG_VM_MEM_END; i++) {
		hg_mem_spool_destroy(vm->mem[i]);
	}
	if (vm->stacks_stack)
		g_ptr_array_free(vm->stacks_stack, TRUE);
	if (vm->params)
		g_hash_table_destroy(vm->params);
	if (vm->plugin_table)
		g_hash_table_destroy(vm->plugin_table);
	if (vm->plugin_list)
		g_list_free(vm->plugin_list);
	if (vm->rand_)
		g_rand_free(vm->rand_);
	hg_mem_free(__hg_vm_mem, vm->self);
}

/**
 * hg_vm_malloc:
 * @vm:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_malloc(hg_vm_t      *vm,
	     hg_usize_t    size,
	     hg_pointer_t *ret)
{
	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	return hg_mem_alloc(hg_vm_get_mem(vm), size, ret);
}

/**
 * hg_vm_realloc:
 * @vm:
 * @qdata:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_realloc(hg_vm_t      *vm,
	      hg_quark_t    qdata,
	      hg_usize_t    size,
	      hg_pointer_t *ret)
{
	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	return hg_mem_realloc(_hg_vm_quark_get_mem(vm, qdata), qdata, size, ret);
}

/**
 * hg_vm_set_default_acl:
 * @vm:
 * @acl:
 *
 * FIXME
 */
void
hg_vm_set_default_acl(hg_vm_t        *vm,
		      hg_quark_acl_t  acl)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->default_acl = acl;
}

/**
 * hg_vm_get_mem:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_vm_get_mem(hg_vm_t *vm)
{
	return _hg_vm_get_mem(vm);
}

/**
 * hg_vm_use_global_mem:
 * @vm:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_use_global_mem(hg_vm_t   *vm,
		     hg_bool_t  flag)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	if (flag)
		vm->vm_state->current_mem_index = HG_VM_MEM_GLOBAL;
	else
		vm->vm_state->current_mem_index = HG_VM_MEM_LOCAL;
}

/**
 * hg_vm_is_global_mem_used:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_is_global_mem_used(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, TRUE, HG_e_VMerror);

	return vm->vm_state->current_mem_index == HG_VM_MEM_GLOBAL;
}

/**
 * hg_vm_get_io:
 * @vm:
 * @type:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_get_io(hg_vm_t      *vm,
	     hg_file_io_t  type)
{
	hg_quark_t ret;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (type < HG_FILE_IO_END, Qnil, HG_e_invalidfileaccess);

	if (type == HG_FILE_IO_LINEEDIT) {
		ret = hg_file_new_with_vtable(_hg_vm_get_mem(vm),
					      "%lineedit",
					      HG_FILE_IO_MODE_READ,
					      hg_file_get_lineedit_vtable(),
					      vm->lineedit,
					      NULL);
		if (ret != Qnil)
			hg_vm_quark_set_acl(vm, &ret, HG_ACL_READABLE|HG_ACL_ACCESSIBLE);
	} else if (type == HG_FILE_IO_STATEMENTEDIT) {
		ret = hg_file_new_with_vtable(_hg_vm_get_mem(vm),
					      "%statementedit",
					      HG_FILE_IO_MODE_READ,
					      hg_file_get_lineedit_vtable(),
					      vm->lineedit,
					      NULL);
		if (ret != Qnil)
			hg_vm_quark_set_acl(vm, &ret, HG_ACL_READABLE|HG_ACL_ACCESSIBLE);
	} else {
		ret = vm->qio[type];
	}

	return ret;
}

/**
 * hg_vm_set_io:
 * @vm:
 * @type:
 * @file:
 *
 * FIXME
 */
void
hg_vm_set_io(hg_vm_t      *vm,
	     hg_file_io_t  type,
	     hg_quark_t    file)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_return_if_fail (type < HG_FILE_IO_END, HG_e_VMerror);
	hg_return_if_fail (HG_IS_QFILE (file), HG_e_VMerror);

	switch (type) {
	    case HG_FILE_IO_FILE:
		    break;
	    case HG_FILE_IO_STDIN:
	    case HG_FILE_IO_LINEEDIT:
	    case HG_FILE_IO_STATEMENTEDIT:
		    hg_return_if_fail (hg_vm_quark_is_readable(vm, &file), HG_e_invalidaccess);
		    break;
	    case HG_FILE_IO_STDOUT:
	    case HG_FILE_IO_STDERR:
		    hg_return_if_fail (hg_vm_quark_is_writable(vm, &file), HG_e_invalidaccess);
		    break;
	    default:
		    break;
	}

	vm->qio[type] = file;
	hg_mem_unref(_hg_vm_quark_get_mem(vm, file),
		     file);
}

/**
 * hg_vm_get_language_level:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_vm_langlevel_t
hg_vm_get_language_level(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, HG_LANG_LEVEL_END, HG_e_VMerror);

	return vm->language_level;
}

/**
 * hg_vm_set_language_level:
 * @vm:
 * @level:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_set_language_level(hg_vm_t           *vm,
			 hg_vm_langlevel_t  level)
{
	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (level < HG_LANG_LEVEL_END, FALSE, HG_e_VMerror);

	if (!vm->hold_lang_level) {
		if (hg_vm_get_language_level(vm) >= level)
			return FALSE;

		/* re-initialize operators */
		return hg_vm_startjob(vm, level, "", FALSE);
	}

	return FALSE;
}

/**
 * hg_vm_hold_language_level:
 * @vm:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_hold_language_level(hg_vm_t   *vm,
			  hg_bool_t  flag)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->hold_lang_level = (flag == TRUE);
}

/**
 * hg_vm_translate:
 * @vm:
 * @flags:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_translate(hg_vm_t             *vm,
		hg_vm_trans_flags_t  flags)
{
	hg_bool_t retval;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	/* drop a HG_TRANS_FLAG_BLOCK from flags */
	flags &= ~HG_TRANS_FLAG_BLOCK;

	retval = _hg_vm_translate(vm, flags);

	if (!HG_ERROR_IS_SUCCESS0 ()) {
		hg_quark_t q = hg_stack_index(vm->stacks[HG_VM_STACK_ESTACK], 0);

		_hg_vm_set_error(vm, q, HG_ERROR_GET_REASON (hg_errno));
	}

	return retval;
}

/**
 * hg_vm_main_loop:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_main_loop(hg_vm_t *vm)
{
	hg_bool_t retval = TRUE;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	vm->shutdown = FALSE;
	while (!hg_vm_is_finished(vm)) {
		hg_usize_t depth = hg_stack_depth(vm->stacks[HG_VM_STACK_ESTACK]);

		if (depth == 0)
			break;

		hg_vm_translate(vm, HG_TRANS_FLAG_NONE);
		if (vm->device && hg_device_is_pending_draw(vm->device))
			hg_device_draw(vm->device);
	}

	return retval;
}

/**
 * hg_vm_eval_cstring:
 * @vm:
 * @cstring:
 * @clen:
 * @protect_systemdict:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_eval_cstring(hg_vm_t         *vm,
		   const hg_char_t *cstring,
		   hg_size_t        clen,
		   hg_bool_t        protect_systemdict)
{
	hg_quark_t qstring;
	hg_bool_t retval = FALSE;
	hg_char_t *str;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (cstring != NULL, FALSE, HG_e_VMerror);

	if (clen < 0)
		clen = strlen(cstring);

	/* add \n at the end to avoid failing on scanner */
	str = (hg_char_t *)hg_malloc0(sizeof (hg_char_t) * (clen + 2));
	memcpy(str, cstring, clen);
	str[clen] = '\n';
	str[clen + 1] = 0;
	qstring = HG_QSTRING_LEN (_hg_vm_get_mem(vm),
				  str, clen + 1);
	hg_free(str);
	if (qstring == Qnil) {
		goto error;
	}
	hg_vm_quark_set_executable(vm, &qstring, TRUE);
	retval = _hg_vm_eval(vm, qstring, protect_systemdict);

	/* Don't free the string here.
	 * this causes a segfault in the file object
	 * since the file object requires running the finalizer
	 * for closing a file and it still keeps this reference.
	 *
	 * hg_vm_mfree(vm, qstring);
	 */
  error:

	return retval;
}

/**
 * hg_vm_eval_file:
 * @vm:
 * @initfile:
 * @protect_systemdict:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_eval_file(hg_vm_t         *vm,
		const hg_char_t *initfile,
		hg_bool_t        protect_systemdict)
{
	hg_char_t *filename;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (initfile != NULL && initfile[0] != 0, FALSE, HG_e_VMerror);

	filename = hg_find_libfile(initfile);
	if (filename) {
		hg_quark_t qfile;

		qfile = hg_file_new(_hg_vm_get_mem(vm),
				    filename,
				    HG_FILE_IO_MODE_READ,
				    NULL);
		if (qfile == Qnil)
			goto error;
		hg_vm_quark_set_default_acl(vm, &qfile);
		hg_vm_quark_set_executable(vm, &qfile, TRUE);
		retval = _hg_vm_eval(vm, qfile, protect_systemdict);
		/* may better relying on GC
		 * hg_vm_mfree(vm, qfile);
		 */
	  error:;
	} else {
		hg_warning("Unable to find a file `%s'", initfile);
	}
	g_free(filename);

	return retval;
}

/**
 * hg_vm_get_error_code:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_int_t
hg_vm_get_error_code(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, -1, HG_e_VMerror);

	return vm->error_code;
}

/**
 * hg_vm_set_error_code:
 * @vm:
 * @error_code:
 *
 * FIXME
 */
void
hg_vm_set_error_code(hg_vm_t  *vm,
		     hg_int_t  error_code)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->error_code = error_code;
}

/**
 * hg_vm_startjob:
 * @vm:
 * @lang_level:
 * @initializer:
 * @encapsulated:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_startjob(hg_vm_t           *vm,
	       hg_vm_langlevel_t  lang_level,
	       const hg_char_t   *initializer,
	       hg_bool_t          encapsulated)
{
	hg_dict_t *dict;
	hg_bool_t retval;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	/* initialize VM */
	if (encapsulated) {
		/* initialize globaldict */
		dict = _HG_VM_LOCK (vm, vm->qglobaldict);
		if (dict) {
			hg_dict_clear(dict);
			_HG_VM_UNLOCK (vm, vm->qglobaldict);
		} else {
			hg_warning("Unable to initialize PostScript VM");
			return FALSE;
		}
		/* initialize internaldict */
		dict = _HG_VM_LOCK (vm, vm->qinternaldict);
		if (dict) {
			hg_dict_clear(dict);
			_HG_VM_UNLOCK (vm, vm->qinternaldict);
		} else {
			hg_warning("Unable to initialize PostScript VM");
			return FALSE;
		}
		/* initialize $error */
		dict = _HG_VM_LOCK (vm, vm->qerror);
		if (dict) {
			hg_dict_clear(dict);
			_HG_VM_UNLOCK (vm, vm->qerror);
		} else {
			hg_warning("Unable to initialize PostScript VM");
			return FALSE;
		}
		/* initialize systemdict */
		dict = _HG_VM_LOCK (vm, vm->qsystemdict);
		if (dict) {
			hg_dict_clear(dict);
			_HG_VM_UNLOCK (vm, vm->qsystemdict);
		} else {
			hg_warning("Unable to initialize PostScript VM");
			return FALSE;
		}
		/* initialize stacks */
		hg_stack_clear(vm->stacks[HG_VM_STACK_OSTACK]);
		hg_stack_clear(vm->stacks[HG_VM_STACK_ESTACK]);
		hg_stack_clear(vm->stacks[HG_VM_STACK_DSTACK]);
		hg_stack_clear(vm->stacks[HG_VM_STACK_GSTATE]);
	}
	if (!_hg_vm_setup(vm, lang_level)) {
		hg_warning("Unable to initialize PostScript VM");
		return FALSE;
	}

	/* clean up garbages */
	hg_mem_spool_run_gc(vm->mem[HG_VM_MEM_GLOBAL]);
	hg_mem_spool_run_gc(vm->mem[HG_VM_MEM_LOCAL]);

	/* XXX: restore memory */

	if (encapsulated) {
		hg_snapshot_t *gsnap, *lsnap;
		hg_quark_t qg = hg_snapshot_new(vm->mem[HG_VM_MEM_GLOBAL], (hg_pointer_t *)&gsnap);
		hg_quark_t ql = hg_snapshot_new(vm->mem[HG_VM_MEM_LOCAL], (hg_pointer_t *)&lsnap);

		hg_snapshot_save(gsnap);
		hg_snapshot_save(lsnap);
		/* XXX: save memory */

		_HG_VM_UNLOCK (vm, qg);
		_HG_VM_UNLOCK (vm, ql);
	}

	/* change the default memory */
	hg_vm_use_global_mem(vm, FALSE);

	retval = hg_vm_eval_file(vm, "hg_init.ps", FALSE);

	/* make systemdict read-only */
	dict = _HG_VM_LOCK (vm, vm->qsystemdict);
	if (dict == NULL) {
		hg_warning("Unable to obtain systemdict");
		return FALSE;
	}
	hg_vm_quark_set_writable(vm, &vm->qsystemdict, FALSE);
	hg_vm_quark_set_writable(vm, &vm->qinternaldict, FALSE);
	if (!hg_dict_add(dict, HG_QNAME ("systemdict"),
			 vm->qsystemdict,
			 FALSE)) {
		hg_warning("Unable to update the entry for systemdict");
		return FALSE;
	}
	_HG_VM_UNLOCK (vm, vm->qsystemdict);

	if (initializer && initializer[0] == 0) {
		return TRUE;
	} else {
		hg_vm_load_plugins(vm);

		if (!vm->device)
			vm->device = hg_device_null_new(vm->mem[HG_VM_MEM_LOCAL]);
		hg_device_install(vm->device, vm);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
		hg_vm_eval_cstring(vm, "initgraphics", -1, FALSE);
		/* XXX: initialize device */

		if (initializer) {
			hg_char_t *s = hg_strdup_printf("{(%s)(r)file dup type/filetype eq{cvx exec}if}stopped{$error/newerror get{errordict/handleerror get exec 1 .quit}if}if", initializer);

			retval = hg_vm_eval_cstring(vm, s, -1, FALSE);

			hg_free(s);

			return retval;
		}
	}

	return (retval ? hg_vm_eval_cstring(vm, "systemdict/.loadhistory known{(.hghistory).loadhistory}if start systemdict/.savehistory known{(.hghistory).savehistory}if", -1, FALSE) : FALSE);
}

/**
 * hg_vm_shutdown:
 * @vm:
 * @error_code:
 *
 * FIXME
 */
void
hg_vm_shutdown(hg_vm_t  *vm,
	       hg_int_t  error_code)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->error_code = error_code;
	vm->shutdown = TRUE;
}

/**
 * hg_vm_is_finished
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_is_finished(hg_vm_t *vm)
{
	return vm->shutdown;
}

/**
 * hg_vm_get_gstate:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_get_gstate(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	return vm->qgstate;
}

/**
 * hg_vm_set_gstate:
 * @vm:
 * @qgstate:
 *
 * FIXME
 */
void
hg_vm_set_gstate(hg_vm_t    *vm,
		 hg_quark_t  qgstate)
{
	hg_uint_t id;
	hg_mem_t *mem = NULL;

	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_return_if_fail (HG_IS_QGSTATE (qgstate), HG_e_VMerror);

	id = hg_quark_get_mem_id(qgstate);
	if (id == vm->mem_id[HG_VM_MEM_GLOBAL]) {
		mem = vm->mem[HG_VM_MEM_GLOBAL];
	} else if (id == vm->mem_id[HG_VM_MEM_LOCAL]) {
		mem = vm->mem[HG_VM_MEM_LOCAL];
	} else {
		g_printerr("%s: gstate allocated with the unknown memory spool: 0x%lx\n",
			   __PRETTY_FUNCTION__,
			   qgstate);
		return;
	}
	vm->qgstate = qgstate;
	hg_mem_unref(mem, qgstate);
}

/**
 * hg_vm_get_user_params:
 * @vm:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_get_user_params(hg_vm_t      *vm,
		      hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_dict_t *d;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	retval = hg_dict_new(_hg_vm_get_mem(vm),
			     sizeof (hg_vm_user_params_t) / sizeof (hg_int_t) + 1,
			     hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
			     (hg_pointer_t *)&d);
	if (retval == Qnil)
		return Qnil;

	hg_vm_quark_set_default_acl(vm, &retval);
	if (!hg_dict_add(d, vm->quparams_name[HG_VM_user_MaxOpStack],
			 HG_QINT (vm->user_params.max_op_stack),
			 FALSE))
		goto error;
	if (!hg_dict_add(d, vm->quparams_name[HG_VM_user_MaxExecStack],
			 HG_QINT (vm->user_params.max_exec_stack),
			 FALSE))
		goto error;
	if (!hg_dict_add(d, vm->quparams_name[HG_VM_user_MaxDictStack],
			 HG_QINT (vm->user_params.max_dict_stack),
			 FALSE))
		goto error;
	if (!hg_dict_add(d, vm->quparams_name[HG_VM_user_MaxGStateStack],
			 HG_QINT (vm->user_params.max_gstate_stack),
			 FALSE))
		goto error;

	if (ret)
		*ret = d;
	else
		hg_mem_unlock_object(d->o.mem, retval);
  error:
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		hg_object_free(d->o.mem, retval);
		retval = Qnil;
	}

	return retval;
}

/**
 * hg_vm_set_user_params:
 * @vm:
 * @qdict:
 *
 * FIXME
 */
hg_bool_t
hg_vm_set_user_params(hg_vm_t    *vm,
		      hg_quark_t  qdict)
{
	hg_bool_t retval = TRUE;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	if (!HG_IS_QDICT (qdict)) {
		hg_debug(HG_MSGCAT_VM, "The user params isn't a dict.");

		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (qdict != Qnil) {
		hg_dict_t *d = _HG_VM_LOCK (vm, qdict);

		if (d == NULL)
			return FALSE;
		hg_dict_foreach(d, _hg_vm_set_user_params, vm);
		if (!HG_ERROR_IS_SUCCESS0 ())
			retval = FALSE;

		HG_VM_UNLOCK (vm, qdict);
	}

	return retval;
}

/**
 * hg_vm_set_rand_seed:
 * @vm:
 * @seed:
 *
 * FIXME
 */
void
hg_vm_set_rand_seed(hg_vm_t   *vm,
		    hg_uint_t  seed)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->rand_seed = seed;
	g_rand_set_seed(vm->rand_, seed);
}

/**
 * hg_vm_get_rand_seed:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_vm_get_rand_seed(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, 0, HG_e_VMerror);

	return vm->rand_seed;
}

/**
 * hg_vm_rand_int:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_vm_rand_int(hg_vm_t *vm)
{
	hg_int_t r;

	hg_return_val_if_fail (vm != NULL, 0, HG_e_VMerror);

	r = g_rand_int(vm->rand_);

	return r & 0x7fffffff;
}

/**
 * hg_vm_get_current_time:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_vm_get_current_time(hg_vm_t *vm)
{
	GTimeVal t;
	hg_uint_t retval, i;

	hg_return_val_if_fail (vm != NULL, 0, HG_e_VMerror);

	g_get_current_time(&t);
	if (t.tv_sec == vm->initiation_time.tv_sec) {
		retval = (t.tv_usec - vm->initiation_time.tv_usec) / 1000;
	} else {
		i = t.tv_sec - vm->initiation_time.tv_sec;
		if (t.tv_usec > vm->initiation_time.tv_usec) {
			retval = i * 1000 + (t.tv_usec - vm->initiation_time.tv_usec) / 1000;
		} else {
			i--;
			retval = i * 1000 + ((1 * 1000000 - vm->initiation_time.tv_usec) + t.tv_usec) / 1000;
		}
	}

	return retval;
}

/**
 * hg_vm_has_error:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_has_error(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, TRUE, HG_e_VMerror);

	return vm->has_error;
}

/**
 * hg_vm_clear_error:
 * @vm:
 *
 * FIXME
 */
void
hg_vm_clear_error(hg_vm_t *vm)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	vm->has_error = FALSE;
}

/**
 * hg_vm_reset_error:
 * @vm:
 *
 * FIXME
 */
void
hg_vm_reset_error(hg_vm_t *vm)
{
	hg_dict_t *dict_error;
	hg_usize_t i;

	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	dict_error = _HG_VM_LOCK (vm, vm->qerror);
	if (dict_error == NULL) {
		hg_critical("Unable to obtain $error dict");
		return;
	}
	if (!hg_dict_add(dict_error,
			 HG_QNAME ("newerror"),
			 HG_QBOOL (FALSE),
			 FALSE))
		goto error;
	if (!hg_dict_add(dict_error,
			 HG_QNAME ("errorname"),
			 HG_QNULL,
			 FALSE))
		goto error;
	if (!hg_dict_add(dict_error,
			 HG_QNAME (".stopped"),
			 HG_QBOOL (FALSE),
			 FALSE))
		goto error;

	hg_vm_clear_error(vm);
	for (i = 0; i < HG_VM_STACK_END; i++) {
		hg_stack_set_validation(vm->stacks[i], TRUE);
	}

	return;
  error:
	hg_critical("Unable to reset entries in $error dict");
}

/**
 * hg_vm_reserved_spool_dump:
 * @vm:
 * @mem:
 * @ofile:
 *
 * FIXME
 */
void
hg_vm_reserved_spool_dump(hg_vm_t   *vm,
			  hg_mem_t  *mem,
			  hg_file_t *ofile)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_return_if_fail (mem != NULL, HG_e_VMerror);
	hg_return_if_fail (ofile != NULL, HG_e_VMerror);

	/* to avoid unusable result when OOM */
	hg_mem_spool_set_resizable(_hg_vm_get_mem(vm), TRUE);

	hg_file_append_printf(ofile, "Referenced blocks in %s\n",
			      mem == vm->mem[HG_VM_MEM_GLOBAL] ? "global" :
			      mem == vm->mem[HG_VM_MEM_LOCAL] ? "local" :
			      "unknown");
	hg_file_append_printf(ofile, "       value      \n");
	hg_file_append_printf(ofile, "----------========\n");
	hg_mem_foreach(mem, HG_BLK_ITER_REFERENCED_ONLY,
		       _hg_vm_rs_real_dump, ofile);
}

/**
 * hg_vm_add_plugin:
 * @vm:
 * @name:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_add_plugin(hg_vm_t         *vm,
		 const hg_char_t *name)
{
	hg_plugin_t *plugin;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, FALSE, HG_e_VMerror);

	if (g_hash_table_lookup(vm->plugin_table, name) != NULL) {
		hg_warning("%s plugin is already loaded", name);
		return TRUE;
	}
	plugin = hg_plugin_open(vm->mem[HG_VM_MEM_LOCAL],
				name,
				HG_PLUGIN_EXTENSION);
	if (plugin) {
		GList *l = g_list_alloc();

		l->data = plugin;
		g_hash_table_insert(vm->plugin_table,
				    hg_strdup(name), l);
		vm->plugin_list = g_list_concat(vm->plugin_list, l);
	}

	return plugin != NULL;
}

/**
 * hg_vm_load_plugins:
 * @vm:
 *
 * FIXME
 */
void
hg_vm_load_plugins(hg_vm_t *vm)
{
	GList *l;

	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	for (l = vm->plugin_list; l != NULL; l = g_list_next(l)) {
		hg_plugin_t *p = l->data;

		hg_plugin_load(p, vm);
	}
}

/**
 * hg_vm_remove_plugin:
 * @vm:
 * @name:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_remove_plugin(hg_vm_t         *vm,
		    const hg_char_t *name)
{
	GList *l;
	hg_bool_t retval;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, FALSE, HG_e_VMerror);

	if ((l = g_hash_table_lookup(vm->plugin_table, name)) == NULL) {
		hg_warning("No such plugins loaded: %s", name);
		return FALSE;
	}

	retval = hg_plugin_unload(l->data, vm);
	if (retval) {
		g_hash_table_remove(vm->plugin_table, name);
		vm->plugin_list = g_list_delete_link(vm->plugin_list, l);
	}

	return retval;
}

/**
 * hg_vm_unload_plugins:
 * @vm:
 *
 * FIXME
 */
void
hg_vm_unload_plugins(hg_vm_t *vm)
{
	GList *lk, *llk;

	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	lk = g_hash_table_get_keys(vm->plugin_table);
	for (llk = lk; llk != NULL; llk = g_list_next(llk)) {
		hg_vm_remove_plugin(vm, llk->data);
	}
	g_list_free(lk);
	g_list_free(vm->plugin_list);
	vm->plugin_list = NULL;
}

/**
 * hg_vm_set_device:
 * @vm:
 * @name:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_set_device(hg_vm_t         *vm,
		 const hg_char_t *name)
{
	hg_device_t *dev;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, FALSE, HG_e_VMerror);

	dev = hg_device_open(vm->mem[HG_VM_MEM_LOCAL], name);

	if (vm->device)
		hg_device_close(vm->device);
	if (!dev) {
		vm->device = hg_device_null_new(vm->mem[HG_VM_MEM_LOCAL]);
	} else {
		vm->device = dev;
	}

	return dev != NULL;
}

/**
 * hg_vm_add_param:
 * @vm:
 * @name:
 * @value:
 *
 * FIXME
 */
void
hg_vm_add_param(hg_vm_t         *vm,
		const hg_char_t *name,
		hg_vm_value_t   *value)
{
	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_return_if_fail (name != NULL, HG_e_VMerror);
	hg_return_if_fail (value != NULL, HG_e_VMerror);

	g_hash_table_insert(vm->params, hg_strdup(name), value);
}

/* hg_array_t */

/**
 * hg_vm_array_set:
 * @vm:
 * @qarray:
 * @qdata:
 * @index:
 * @force:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_array_set(hg_vm_t    *vm,
		hg_quark_t  qarray,
		hg_quark_t  qdata,
		hg_usize_t  index,
		hg_bool_t   force)
{
	hg_array_t *a;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	if (!HG_IS_QARRAY (qarray)) {
		hg_debug(HG_MSGCAT_ARRAY, "Not an array");

		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!force &&
	    !hg_vm_quark_is_writable(vm, &qarray)) {
		hg_debug(HG_MSGCAT_ARRAY, "No writable permission to access the array");

		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	a = _HG_VM_LOCK (vm, qarray);
	if (!a)
		return FALSE;

	retval = hg_array_set(a, qdata, index, force);

	_HG_VM_UNLOCK (vm, qarray);

	return retval;
}

/**
 * hg_vm_array_get:
 * @vm:
 * @qarray:
 * @index:
 * @force:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_array_get(hg_vm_t    *vm,
		hg_quark_t  qarray,
		hg_usize_t  index,
		hg_bool_t   force)
{
	hg_quark_t retval = Qnil;
	hg_array_t *a;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	if (!HG_IS_QARRAY (qarray)) {
		hg_debug(HG_MSGCAT_ARRAY, "Not an array");

		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!force &&
	    !hg_vm_quark_is_readable(vm, &qarray)) {
		hg_debug(HG_MSGCAT_ARRAY, "No readable permission to access the array");

		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	a = _HG_VM_LOCK (vm, qarray);
	if (!a)
		return Qnil;

	retval = hg_array_get(a, index);

	_HG_VM_UNLOCK (vm, qarray);

	return retval;
}

/* hg_dict_t */

/**
 * hg_vm_dstack_lookup:
 * @vm:
 * @qname:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_dstack_lookup(hg_vm_t    *vm,
		    hg_quark_t  qname)
{
	hg_quark_t retval = Qnil, quark;
	hg_stack_t *dstack;
	hg_vm_dict_lookup_data_t ldata;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	if (HG_IS_QSTRING (qname)) {
		hg_char_t *str;
		hg_string_t *s;

		s = _HG_VM_LOCK (vm, qname);
		if (s == NULL)
			return Qnil;
		str = hg_string_get_cstr(s);
		quark = hg_name_new_with_string(str, -1);

		hg_free(str);
		_HG_VM_UNLOCK (vm, qname);
	} else if (HG_IS_QEVALNAME (qname)) {
		quark = hg_quark_new(HG_TYPE_NAME, qname);
	} else {
		quark = qname;
	}
	dstack = vm->stacks[HG_VM_STACK_DSTACK];
	ldata.vm = vm;
	ldata.result = Qnil;
	ldata.qname = quark;
	ldata.check_perms = TRUE;
	hg_stack_foreach(dstack, _hg_vm_real_dict_lookup, &ldata, FALSE);
	retval = ldata.result;

	return retval;
}

/**
 * hg_vm_dstack_remove:
 * @vm:
 * @qname:
 * @remove_all:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_dstack_remove(hg_vm_t    *vm,
		    hg_quark_t  qname,
		    hg_bool_t   remove_all)
{
	hg_stack_t *dstack;
	hg_vm_dict_remove_data_t ldata;
	hg_bool_t retval = FALSE;
	hg_quark_t quark;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	if (HG_IS_QSTRING (qname)) {
		hg_char_t *str;
		hg_string_t *s;

		s = _HG_VM_LOCK (vm, qname);
		if (s == NULL)
			return FALSE;
		str = hg_string_get_cstr(s);
		quark = hg_name_new_with_string(str, -1);

		hg_free(str);
		_HG_VM_UNLOCK (vm, qname);
	} else {
		quark = qname;
	}
	dstack = vm->stacks[HG_VM_STACK_DSTACK];
	ldata.vm = vm;
	ldata.result = FALSE;
	ldata.qname = quark;
	ldata.remove_all = remove_all;
	hg_stack_foreach(dstack, _hg_vm_real_dict_remove, &ldata, FALSE);

	if (HG_ERROR_IS_SUCCESS0 ())
		retval = ldata.result;

	return retval;
}

/**
 * hg_vm_dstack_get_dict:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_dstack_get_dict(hg_vm_t *vm)
{
	hg_quark_t qdict;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	qdict = hg_stack_index(vm->stacks[HG_VM_STACK_DSTACK], 0);

	return qdict;
}

/**
 * hg_vm_dict_add:
 * @vm:
 * @qdict:
 * @qkey:
 * @qval:
 * @force:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_dict_add(hg_vm_t    *vm,
	       hg_quark_t  qdict,
	       hg_quark_t  qkey,
	       hg_quark_t  qval,
	       hg_bool_t   force)
{
	hg_dict_t *d;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	if (!HG_IS_QDICT (qdict)) {
		hg_debug(HG_MSGCAT_DICT, "Not a dict");

		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!force &&
	    !hg_vm_quark_is_writable(vm, &qdict)) {
		hg_debug(HG_MSGCAT_DICT, "No writable permission to access the dict");

		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	d = _HG_VM_LOCK (vm, qdict);
	if (!d)
		return FALSE;

	retval = hg_dict_add(d, qkey, qval, force);

	_HG_VM_UNLOCK (vm, qdict);

	return retval;
}

/**
 * hg_vm_dict_remove:
 * @vm:
 * @qdict:
 * @qkey:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_dict_remove(hg_vm_t    *vm,
		  hg_quark_t  qdict,
		  hg_quark_t  qkey)
{
	hg_dict_t *d;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	if (!HG_IS_QDICT (qdict)) {
		hg_debug(HG_MSGCAT_DICT, "Not a dict");

		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &qdict)) {
		hg_debug(HG_MSGCAT_DICT, "No writable permission to access the dict");

		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	d = _HG_VM_LOCK (vm, qdict);
	if (!d)
		return FALSE;

	retval = hg_dict_remove(d, qkey);

	_HG_VM_UNLOCK (vm, qdict);

	return retval;
}

/**
 * hg_vm_dict_lookup:
 * @vm:
 * @qdict:
 * @qkey:
 * @check_perms:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_dict_lookup(hg_vm_t    *vm,
		  hg_quark_t  qdict,
		  hg_quark_t  qkey,
		  hg_bool_t   check_perms)
{
	hg_dict_t *d;
	hg_quark_t retval = Qnil;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	if (!HG_IS_QDICT (qdict)) {
		hg_debug(HG_MSGCAT_DICT, "Not a dict");

		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (check_perms &&
	    !hg_vm_quark_is_readable(vm, &qdict)) {
		hg_debug(HG_MSGCAT_DICT, "No readable permission to access the dict");

		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	d = _HG_VM_LOCK (vm, qdict);
	if (!d)
		return Qnil;

	retval = hg_dict_lookup(d, qkey);

	_HG_VM_UNLOCK (vm, qdict);

	return retval;
}

/* hg_quark_t */

/**
 * hg_vm_quark_get_mem:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_vm_quark_get_mem(hg_vm_t    *vm,
		    hg_quark_t  qdata)
{
	return _hg_vm_quark_get_mem(vm, qdata);
}

/**
 * hg_vm_quark_lock_object:
 * @vm:
 * @qdata:
 * @pretty_function:
 *
 * FIXME
 *
 * Returns:
 */
hg_pointer_t
hg_vm_quark_lock_object(hg_vm_t         *vm,
			hg_quark_t       qdata,
			const hg_char_t *pretty_function)
{
	return _hg_vm_quark_lock_object(vm, qdata, pretty_function);
}

/**
 * hg_vm_quark_unlock_object:
 * @vm:
 * @qdata:
 *
 * FIXME
 */
void
hg_vm_quark_unlock_object(hg_vm_t    *vm,
			  hg_quark_t  qdata)
{
	_hg_vm_quark_unlock_object(vm, qdata);
}

/**
 * hg_vm_quark_gc_mark:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_gc_mark(hg_vm_t    *vm,
		    hg_quark_t  qdata)
{
	hg_object_t *o;
	hg_bool_t retval;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	if (qdata == Qnil)
		return TRUE;

	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata))
		return TRUE;

	o = _HG_VM_LOCK (vm, qdata);
	if (o == NULL)
		return FALSE;

	retval = hg_mem_gc_mark(o->mem, o->self);

	_HG_VM_UNLOCK (vm, qdata);

	return retval;
}

/**
 * hg_vm_quark_copy:
 * @vm:
 * @qdata:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_quark_copy(hg_vm_t      *vm,
		 hg_quark_t    qdata,
		 hg_pointer_t *ret)
{
	hg_object_t *o;
	hg_quark_t retval = Qnil;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (qdata != Qnil, Qnil, HG_e_VMerror);

	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata))
		return qdata;

	o = _HG_VM_LOCK (vm, qdata);
	if (o) {
		retval = hg_object_copy(o, _hg_vm_quark_iterate_copy, vm, ret);

		_HG_VM_UNLOCK (vm, qdata);
	}
	if (retval != Qnil) {
		hg_quark_acl_t acl = 0;

		if (hg_vm_quark_is_readable(vm, &qdata))
			acl |= HG_ACL_READABLE;
		if (hg_vm_quark_is_writable(vm, &qdata))
			acl |= HG_ACL_WRITABLE;
		if (hg_vm_quark_is_executable(vm, &qdata))
			acl |= HG_ACL_EXECUTABLE;
		if (hg_vm_quark_is_accessible(vm, &qdata))
			acl |= HG_ACL_ACCESSIBLE;
		hg_vm_quark_set_acl(vm, &retval, acl);
	}

	return retval;
}

/**
 * hg_vm_quark_to_string:
 * @vm:
 * @qdata:
 * @ps_like_syntax:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_quark_to_string(hg_vm_t      *vm,
		      hg_quark_t    qdata,
		      hg_bool_t     ps_like_syntax,
		      hg_pointer_t *ret)
{
	hg_object_t *o;
	hg_quark_t retval = Qnil;
	hg_string_t *s;
	const hg_char_t const *types[] = {
		"-null-",
		"-integer-",
		"-real-",
		"-name-",
		"-bool-",
		"-string-",
		"-ename-",
		"-dict-",
		"-oper-",
		"-array-",
		"-mark-",
		"-file-",
		"-save-",
		"-stack-",
		"-dictnode-",
		"-path-",
		"-gstate-",
		NULL
	};
	hg_char_t *cstr;

	hg_return_val_if_fail (vm != NULL, Qnil, HG_e_VMerror);

	retval = hg_string_new(_hg_vm_get_mem(vm),
			       65535, (hg_pointer_t *)&s);
	if (retval == Qnil)
		return Qnil;

	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata)) {
		switch (hg_quark_get_type(qdata)) {
		    case HG_TYPE_NULL:
		    case HG_TYPE_MARK:
			    hg_string_append(s, types[hg_quark_get_type(qdata)], -1);
			    break;
		    case HG_TYPE_INT:
			    hg_string_append_printf(s, "%d", HG_INT (qdata));
			    break;
		    case HG_TYPE_REAL:
			    hg_string_append_printf(s, "%f", HG_REAL (qdata));
			    break;
		    case HG_TYPE_EVAL_NAME:
			    if (ps_like_syntax)
				    hg_string_append_printf(s, "//%s", HG_NAME (qdata));
			    else
				    hg_string_append(s, HG_NAME (qdata), -1);
			    break;
		    case HG_TYPE_NAME:
			    if (ps_like_syntax) {
				    if (hg_vm_quark_is_executable(vm, &qdata))
					    hg_string_append_printf(s, "%s", HG_NAME (qdata));
				    else
					    hg_string_append_printf(s, "/%s", HG_NAME (qdata));
			    } else {
				    hg_string_append(s, HG_NAME (qdata), -1);
			    }
			    break;
		    case HG_TYPE_BOOL:
			    hg_string_append(s, HG_BOOL (qdata) ? "true" : "false", -1);
			    break;
		    case HG_TYPE_OPER:
			    hg_string_append_printf(s, "%s",
						    hg_operator_get_name(qdata));
			    break;
		    default:
			    hg_critical("Unknown simple object type: %d",
					hg_quark_get_type(qdata));

			    hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_VMerror);
		}
	} else {
		if (!hg_vm_quark_is_readable(vm, &qdata)) {
			hg_string_append(s, types[hg_quark_get_type(qdata)], -1);
		} else {
			switch (hg_quark_get_type(qdata)) {
			    case HG_TYPE_STRING:
				    if (!ps_like_syntax) {
					    hg_string_t *ss = _HG_VM_LOCK (vm, qdata);

					    if (ss) {
						    cstr = hg_string_get_cstr(ss);
						    _HG_VM_UNLOCK (vm, qdata);

						    hg_string_append(s, cstr, -1);
						    hg_free(cstr);
						    break;
					    }
				    }
			    case HG_TYPE_DICT:
			    case HG_TYPE_ARRAY:
			    case HG_TYPE_FILE:
			    case HG_TYPE_SNAPSHOT:
			    case HG_TYPE_STACK:
				    o = _HG_VM_LOCK (vm, qdata);
				    if (o) {
					    cstr = hg_object_to_cstr(o, _hg_vm_quark_iterate_to_cstr, vm);
					    if (cstr && HG_IS_QARRAY (qdata) && hg_vm_quark_is_executable(vm, &qdata)) {
						    if (cstr[0] == '[') {
							    cstr[0] = '{';
							    cstr[strlen(cstr)-1] = '}';
						    }
					    }
					    hg_string_append(s, cstr, -1);
					    hg_free(cstr);

					    _HG_VM_UNLOCK (vm, qdata);
				    }
				    break;
			    default:
				    hg_critical("Unknown complex object type: %d",
						hg_quark_get_type(qdata));

				    hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_VMerror);
			}
		}
	}
	if (retval != Qnil) {
		hg_string_fix_string_size(s);
		if (ret)
			*ret = s;
		else
			_HG_VM_UNLOCK (vm, retval);
	}
	if (!HG_ERROR_IS_SUCCESS0 ())
		retval = Qnil;

	return retval;
}

/**
 * hg_vm_quark_compare:
 * @vm:
 * @qdata1:
 * @qdata2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_compare(hg_vm_t    *vm,
		    hg_quark_t  qdata1,
		    hg_quark_t  qdata2)
{
	return hg_quark_compare(qdata1, qdata2, _hg_vm_quark_complex_compare, vm);
}

/**
 * hg_vm_quark_compare_content:
 * @vm:
 * @qdata1:
 * @qdata2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_compare_content(hg_vm_t    *vm,
			    hg_quark_t  qdata1,
			    hg_quark_t  qdata2)
{
	return hg_quark_compare(qdata1, qdata2, _hg_vm_quark_complex_compare_content, vm);
}

/**
 * hg_vm_quark_set_default_acl:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
void
hg_vm_quark_set_default_acl(hg_vm_t    *vm,
			    hg_quark_t *qdata)
{
	hg_quark_acl_t acl = 0;

	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_return_if_fail (qdata != NULL, HG_e_VMerror);

	if (*qdata == Qnil)
		return;
	/* do not reset an exec bit here */
	if (hg_quark_is_simple_object(*qdata)) {
		acl = HG_ACL_READABLE|HG_ACL_ACCESSIBLE;

		if (hg_quark_is_executable(*qdata))
			acl |= HG_ACL_EXECUTABLE;
		hg_quark_set_acl(qdata, acl);
	} else if (HG_IS_QOPER (*qdata)) {
		hg_quark_set_acl(qdata, HG_ACL_EXECUTABLE|HG_ACL_ACCESSIBLE);
	} else {
		hg_vm_quark_set_acl(vm, qdata, vm->default_acl);
	}
}

/**
 * hg_vm_quark_set_acl:
 * @vm:
 * @qdata:
 * @readable:
 * @writable:
 * @executable:
 * @accessible:
 *
 * FIXME
 */
void
hg_vm_quark_set_acl(hg_vm_t        *vm,
		    hg_quark_t     *qdata,
		    hg_quark_acl_t  acl)
{
	hg_quark_set_acl(qdata, acl);
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);

		if (o) {
			hg_object_set_acl(o,
					  acl & HG_ACL_READABLE ? 1 : -1,
					  acl & HG_ACL_WRITABLE ? 1 : -1,
					  acl & HG_ACL_EXECUTABLE ? 1 : -1,
					  acl & HG_ACL_ACCESSIBLE ? 1 : -1);
			_HG_VM_UNLOCK (vm, *qdata);
		}
	}
}

/**
 * hg_vm_quark_set_readable:
 * @vm:
 * @qdata:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_quark_set_readable(hg_vm_t    *vm,
			 hg_quark_t *qdata,
			 hg_bool_t   flag)
{
	hg_quark_set_readable(qdata, flag);
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);

		if (o) {
			hg_object_set_acl(o, flag ? 1 : -1, 0, 0, 0);

			_HG_VM_UNLOCK (vm, *qdata);
		}
	}
}

/**
 * hg_vm_quark_is_readable:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_is_readable(hg_vm_t    *vm,
			hg_quark_t *qdata)
{
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);

		if (o) {
			hg_quark_acl_t acl = hg_object_get_acl(o);

			if (acl != -1) {
				hg_quark_set_acl(qdata, acl);
			}

			_HG_VM_UNLOCK (vm, *qdata);
		}
	}

	return hg_quark_is_readable(*qdata);
}

/**
 * hg_vm_quark_set_writable:
 * @vm:
 * @qdata:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_quark_set_writable(hg_vm_t    *vm,
			 hg_quark_t *qdata,
			 hg_bool_t   flag)
{
	hg_quark_set_writable(qdata, flag);
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);

		if (o) {
			hg_object_set_acl(o, 0, flag ? 1 : -1, 0, 0);

			_HG_VM_UNLOCK (vm, *qdata);
		}
	}
}

/**
 * hg_vm_quark_is_writable:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_is_writable(hg_vm_t    *vm,
			hg_quark_t *qdata)
{
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);
		hg_quark_acl_t acl = hg_object_get_acl(o);

		if (acl != -1) {
			hg_quark_set_acl(qdata, acl);
		}

		_HG_VM_UNLOCK (vm, *qdata);
	}

	return hg_quark_is_writable(*qdata);
}

/**
 * hg_vm_quark_set_executable:
 * @vm:
 * @qdata:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_quark_set_executable(hg_vm_t    *vm,
			   hg_quark_t *qdata,
			   hg_bool_t   flag)
{
	hg_quark_set_executable(qdata, flag);
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);

		if (o) {
			hg_object_set_acl(o, 0, 0, flag ? 1 : -1, 0);

			_HG_VM_UNLOCK (vm, *qdata);
		}
	}
}

/**
 * hg_vm_quark_is_executable:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_is_executable(hg_vm_t    *vm,
			  hg_quark_t *qdata)
{
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);
		hg_quark_acl_t acl = hg_object_get_acl(o);
		if (acl != -1) {
			hg_quark_set_acl(qdata, acl);
		}

		_HG_VM_UNLOCK (vm, *qdata);
	}

	return hg_quark_is_executable(*qdata);
}

/**
 * hg_vm_quark_set_accessible:
 * @vm:
 * @qdata:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_quark_set_accessible(hg_vm_t    *vm,
			   hg_quark_t *qdata,
			   hg_bool_t   flag)
{
	hg_quark_set_accessible(qdata, flag);
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);

		if (o) {
			hg_object_set_acl(o, 0, 0, 0, flag ? 1 : -1);

			_HG_VM_UNLOCK (vm, *qdata);
		}
	}
}

/**
 * hg_vm_quark_is_accessible:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_quark_is_accessible(hg_vm_t    *vm,
			  hg_quark_t *qdata)
{
	if (!hg_quark_is_simple_object(*qdata) &&
	    !HG_IS_QOPER (*qdata)) {
		hg_object_t *o = _HG_VM_LOCK (vm, *qdata);
		hg_quark_acl_t acl = hg_object_get_acl(o);

		if (acl != -1) {
			hg_quark_set_acl(qdata, acl);
		}

		_HG_VM_UNLOCK (vm, *qdata);
	}

	return hg_quark_is_accessible(*qdata);
}

/* hg_snapshot_t */

/**
 * hg_vm_snapshot_save:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_snapshot_save(hg_vm_t *vm)
{
	hg_quark_t q, qgg = hg_vm_get_gstate(vm), qg;
	hg_snapshot_t *sn;
	hg_gstate_t *gstate;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);

	q = hg_snapshot_new(vm->mem[HG_VM_MEM_LOCAL],
			    (hg_pointer_t *)&sn);
	if (q == Qnil) {
		hg_debug(HG_MSGCAT_SNAPSHOT, "Unable to create a snapshot object");
		return FALSE;
	}
	if (!hg_snapshot_save(sn)) {
		hg_debug(HG_MSGCAT_SNAPSHOT, "Unable to take a snapshot.");
		_HG_VM_UNLOCK (vm, q);
		return FALSE;
	}
	_HG_VM_UNLOCK (vm, q);

	qg = hg_vm_quark_copy(vm, qgg, NULL);
	if (qg == Qnil) {
		hg_debug(HG_MSGCAT_SNAPSHOT, "Unable to copy the gstate.");
		return FALSE;
	}

	gstate = _HG_VM_LOCK (vm, qgg);
	if (gstate == NULL)
		return FALSE;
	gstate->is_snapshot = q;
	_HG_VM_UNLOCK (vm, qgg);

	if (!hg_stack_push(vm->stacks[HG_VM_STACK_GSTATE], qgg)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
	}
	hg_vm_set_gstate(vm, qg);

	if (!hg_stack_push(vm->stacks[HG_VM_STACK_OSTACK], q)) {
		return FALSE;
	}
	vm->vm_state->n_save_objects++;

	return TRUE;
}

/**
 * hg_vm_snapshot_restore:
 * @vm:
 * @qsnapshot:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_vm_snapshot_restore(hg_vm_t    *vm,
		       hg_quark_t  qsnapshot)
{
	hg_quark_t q = Qnil, qq = Qnil;
	hg_snapshot_t *sn;
	hg_gstate_t *gstate;

	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (HG_IS_QSNAPSHOT (qsnapshot), FALSE, HG_e_typecheck);

	/* drop the gstate in the stack */
	while (hg_stack_depth(vm->stacks[HG_VM_STACK_GSTATE]) > 0 &&
	       qq != qsnapshot) {
		q = hg_stack_index(vm->stacks[HG_VM_STACK_GSTATE], 0);
		gstate = _HG_VM_LOCK (vm, q);
		if (gstate == NULL) {
			return FALSE;
		}
		hg_vm_set_gstate(vm, q);
		qq = gstate->is_snapshot;
		hg_stack_drop(vm->stacks[HG_VM_STACK_GSTATE]);

		_HG_VM_UNLOCK (vm, q);
	}

	sn = _HG_VM_LOCK (vm, qsnapshot);
	if (sn == NULL) {
		return FALSE;
	}
	if (!hg_snapshot_restore(sn,
				 _hg_vm_restore_mark,
				 vm)) {
		_HG_VM_UNLOCK (vm, qsnapshot);

		return FALSE;
	}

	return TRUE;
}

/* hg_stack_t */

/**
 * hg_vm_stack_new:
 * @vm:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_stack_t *
hg_vm_stack_new(hg_vm_t    *vm,
		hg_usize_t  size)
{
	hg_stack_t *retval = hg_stack_new(__hg_vm_mem, size, vm);

	hg_stack_set_spooler(retval, vm->stack_spooler);

	return retval;
}

/**
 * hg_vm_stack_dump:
 * @stack:
 * @ofile:
 *
 * FIXME
 */
void
hg_vm_stack_dump(hg_vm_t    *vm,
		 hg_stack_t *stack,
		 hg_file_t  *output)
{
	hg_vm_stack_dump_data_t data;
	hg_mem_t *m;

	hg_return_if_fail (vm != NULL, HG_e_VMerror);
	hg_return_if_fail (stack != NULL, HG_e_VMerror);
	hg_return_if_fail (output != NULL, HG_e_VMerror);

	m = _hg_vm_get_mem(vm);
	/* to avoid unusable result when OOM */
	hg_mem_spool_set_resizable(m, TRUE);

	data.vm = vm;
	data.stack = stack;
	data.ofile = output;

	hg_file_append_printf(output, "       value      |    type    |M|attr|content\n");
	hg_file_append_printf(output, "----------========+------------+-+----+---------------------------------\n");
	hg_stack_foreach(stack, _hg_vm_stack_real_dump, &data, FALSE);
}

/* hg_string_t */

/**
 * hg_vm_string_get_cstr:
 * @vm:
 * @qstring:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_char_t *
hg_vm_string_get_cstr(hg_vm_t    *vm,
		      hg_quark_t  qstring)
{
	hg_char_t *retval = NULL;
	hg_string_t *s;

	hg_return_val_if_fail (vm != NULL, NULL, HG_e_VMerror);

	if (!HG_IS_QSTRING (qstring)) {
		hg_debug(HG_MSGCAT_STRING, "Not a string");

		hg_error_return_val (NULL, HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &qstring)) {
		hg_debug(HG_MSGCAT_STRING, "No readable permission to access the string");

		hg_error_return_val (NULL, HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	s = _HG_VM_LOCK (vm, qstring);
	if (!s)
		return NULL;
	retval = hg_string_get_cstr(s);

	_HG_VM_UNLOCK (vm, qstring);

	return retval;
}

/**
 * hg_vm_string_length:
 * @vm:
 * @qstring:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_vm_string_length(hg_vm_t    *vm,
		    hg_quark_t  qstring)
{
	hg_uint_t retval = 0;
	hg_string_t *s;

	hg_return_val_if_fail (vm != NULL, 0, HG_e_VMerror);

	if (!HG_IS_QSTRING (qstring)) {
		hg_debug(HG_MSGCAT_STRING, "Not a string");

		hg_error_return_val (0, HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &qstring)) {
		hg_debug(HG_MSGCAT_STRING, "No readable permission to access the string");

		hg_error_return_val (0, HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	s = _HG_VM_LOCK (vm, qstring);
	if (!s)
		return 0;
	retval = hg_string_length(s);

	_HG_VM_UNLOCK (vm, qstring);

	return retval;
}

/* hg_vm_value_t */

/**
 * hg_vm_value_free:
 * @data:
 *
 * FIXME
 */
void
hg_vm_value_free(hg_pointer_t data)
{
	hg_vm_value_t *v = (hg_vm_value_t *)data;

	if (v->type == HG_TYPE_STRING)
		hg_free(v->u.string);

	hg_free(v);
}

/**
 * hg_vm_value_boolean_new:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
hg_vm_value_t *
hg_vm_value_boolean_new(hg_bool_t value)
{
	hg_vm_value_t *retval;

	retval = (hg_vm_value_t *)hg_malloc(sizeof (hg_vm_value_t));
	if (retval) {
		retval->type = HG_TYPE_BOOL;
		retval->u.bool = value;
	}

	return retval;
}

/**
 * hg_vm_value_integer_new:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
hg_vm_value_t *
hg_vm_value_integer_new(hg_int_t value)
{
	hg_vm_value_t *retval;

	retval = (hg_vm_value_t *)hg_malloc(sizeof (hg_vm_value_t));
	if (retval) {
		retval->type = HG_TYPE_INT;
		retval->u.integer = value;
	}

	return retval;
}

/**
 * hg_vm_value_real_new:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
hg_vm_value_t *
hg_vm_value_real_new(hg_real_t value)
{
	hg_vm_value_t *retval;

	retval = (hg_vm_value_t *)hg_malloc(sizeof (hg_vm_value_t));
	if (retval) {
		retval->type = HG_TYPE_REAL;
		retval->u.real = value;
	}

	return retval;
}

/**
 * hg_vm_value_string_new:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
hg_vm_value_t *
hg_vm_value_string_new(const hg_char_t *value)
{
	hg_vm_value_t *retval;

	retval = (hg_vm_value_t *)hg_malloc(sizeof (hg_vm_value_t));
	if (retval) {
		retval->type = HG_TYPE_STRING;
		retval->u.string = hg_strdup(value);
	}

	return retval;
}
