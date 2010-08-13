/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgvm.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "hgbool.h"
#include "hgdict.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgmem.h"
#include "hgname.h"
#include "hgnull.h"
#include "hgoperator.h"
#include "hgreal.h"
#include "hgscanner.h"
#include "hgvm.h"

#define HG_VM_MEM_SIZE		128000
#define HG_VM_GLOBAL_MEM_SIZE	24000000
#define HG_VM_LOCAL_MEM_SIZE	1000000
#define _HG_VM_LOCK(_v_,_q_,_e_)					\
	(_hg_vm_real_lock_object((_v_),(_q_),__PRETTY_FUNCTION__,(_e_)))
#define _HG_VM_UNLOCK(_v_,_q_)				\
	(_hg_vm_real_unlock_object((_v_),(_q_)))


typedef struct _hg_vm_dict_lookup_data_t {
	hg_vm_t    *vm;
	hg_quark_t  qname;
	hg_quark_t  result;
} hg_vm_dict_lookup_data_t;
typedef struct _hg_vm_dict_remove_data_t {
	hg_vm_t    *vm;
	hg_quark_t  qname;
	gboolean    remove_all;
	gboolean    result;
} hg_vm_dict_remove_data_t;
typedef struct _hg_vm_stack_dump_data_t {
	hg_vm_t    *vm;
	hg_stack_t *stack;
	hg_file_t  *ofile;
} hg_vm_stack_dump_data_t;


static hg_quark_t hg_vm_step_in_exec_array(hg_vm_t    *vm,
					   hg_quark_t  qparent);

hg_mem_t *__hg_vm_mem = NULL;
gboolean __hg_vm_is_initialized = FALSE;

/*< private >*/
G_INLINE_FUNC hg_mem_t *
_hg_vm_get_mem(hg_vm_t    *vm,
	       hg_quark_t  quark)
{
	hg_mem_t *retval = NULL;
	gint id = _hg_quark_type_bit_get_bits(quark,
					      HG_QUARK_TYPE_BIT_MEM_ID,
					      HG_QUARK_TYPE_BIT_MEM_ID_END);

	if (vm->mem_id[HG_VM_MEM_GLOBAL] == id) {
		retval = vm->mem[HG_VM_MEM_GLOBAL];
	} else if (vm->mem_id[HG_VM_MEM_LOCAL] == id) {
		retval = vm->mem[HG_VM_MEM_LOCAL];
	} else {
		g_warning("Unknown memory spooler id: %d, quark: %lx", id, quark);
	}

	return retval;
}

G_INLINE_FUNC gpointer
_hg_vm_real_lock_object(hg_vm_t      *vm,
			hg_quark_t    qdata,
			const gchar  *pretty_function,
			GError      **error)
{
	return hg_mem_lock_object_with_gerror(_hg_vm_get_mem(vm, qdata),
					      qdata,
					      pretty_function,
					      error);
}

G_INLINE_FUNC void
_hg_vm_real_unlock_object(hg_vm_t    *vm,
			  hg_quark_t  qdata)
{
	hg_mem_unlock_object(_hg_vm_get_mem(vm, qdata),
			     qdata);
}

static gboolean
_hg_vm_real_dict_lookup(hg_mem_t    *mem,
			hg_quark_t   q,
			gpointer     data,
			GError     **error)
{
	hg_vm_dict_lookup_data_t *x = data;
	hg_dict_t *dict;
	gboolean retval = TRUE;

	dict = _HG_VM_LOCK (x->vm, q, error);
	if (dict == NULL)
		return FALSE;

	if ((x->result = hg_dict_lookup(dict, x->qname, error)) != Qnil) {
		retval = FALSE;
	}

	_HG_VM_UNLOCK (x->vm, q);

	return retval;
}

static gboolean
_hg_vm_real_dict_remove(hg_mem_t    *mem,
			hg_quark_t   q,
			gpointer     data,
			GError     **error)
{
	hg_vm_dict_remove_data_t *x = data;
	hg_dict_t *dict;

	dict = _HG_VM_LOCK (x->vm, q, error);
	if (dict == NULL)
		return FALSE;

	x->result |= hg_dict_remove(dict, x->qname, error);

	_HG_VM_UNLOCK (x->vm, q);

	return !x->result || x->remove_all;
}

G_INLINE_FUNC gchar *
_hg_vm_find_file(const gchar *initfile)
{
	const gchar *env = g_getenv("HIEROGLYPH_LIB_PATH");
	gchar **paths, *filename = NULL, *basename = g_path_get_basename(initfile);
	gint i = 0;

	if (env) {
		paths = g_strsplit(env, ":", 0);
		if (paths) {
			for (i = 0; paths[i] != NULL; i++) {
				filename = g_build_filename(paths[i], basename, NULL);
				if (g_file_test(filename, G_FILE_TEST_EXISTS))
					break;
				g_free(filename);
				filename = NULL;
			}
			g_strfreev(paths);
		}
	}
	if (!filename) {
		filename = g_build_filename(HIEROGLYPH_LIBDIR, basename, NULL);
		if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
			g_free(filename);
			filename = NULL;
		}
	}
	if (basename)
		g_free(basename);

	return filename;
}

static gboolean
_hg_vm_stack_real_dump(hg_mem_t    *mem,
		       hg_quark_t   qdata,
		       gpointer     data,
		       GError     **error)
{
	hg_vm_stack_dump_data_t *ddata = data;
	hg_quark_t q;
	hg_string_t *s = NULL;
	gchar *cstr = NULL;

	q = hg_vm_quark_to_string(ddata->vm, qdata, (gpointer *)&s, error);
	if (q != Qnil)
		cstr = hg_string_get_cstr(s);

	hg_file_append_printf(ddata->ofile, "0x%016lx|%-12s| %c%c%c|%s\n",
			      qdata,
			      hg_quark_get_type_name(qdata),
			      (hg_quark_is_readable(qdata) ? 'r' : '-'),
			      (hg_quark_is_writable(qdata) ? 'w' : '-'),
			      (hg_quark_is_executable(qdata) ? 'x' : '-'),
			      q == Qnil ? "..." : cstr);

	g_free(cstr);
	/* this is an instant object.
	 * surely no reference to the container.
	 * so it can be safely destroyed.
	 */
	hg_string_free(s, TRUE);

	return TRUE;
}

static gboolean
hg_vm_stepi_in_exec_array(hg_vm_t    *vm,
			  hg_quark_t  qparent)
{
	hg_stack_t *ostack, *estack;
	hg_quark_t qexecobj, qresult;
	hg_file_t *file;
	GError *err = NULL;
	gboolean retval = TRUE;
	const gchar *name;

	ostack = vm->stacks[HG_VM_STACK_OSTACK];
	estack = vm->stacks[HG_VM_STACK_ESTACK];

	qexecobj = hg_stack_index(estack, 0, &err);
	if (err) {
		hg_vm_set_error(vm, qparent, HG_VM_e_stackunderflow);
		goto finalize;
	}

	switch (hg_quark_get_type(qexecobj)) {
	    case HG_TYPE_EVAL_NAME:
		    if (hg_quark_is_executable(qexecobj)) {
			    qresult = hg_vm_dict_lookup(vm, qexecobj);
			    if (qresult == Qnil) {
				    hg_vm_set_error(vm, qparent,
						    HG_VM_e_undefined);
				    return FALSE;
			    }
			    qexecobj = qresult;
		    } else {
			    g_warning("Immediately evaluated name object somehow doesn't have a exec bit turned on: %s", hg_name_lookup(vm->name, qexecobj));
		    }
	    case HG_TYPE_NULL:
	    case HG_TYPE_INT:
	    case HG_TYPE_REAL:
	    case HG_TYPE_BOOL:
	    case HG_TYPE_DICT:
	    case HG_TYPE_MARK:
	    case HG_TYPE_NAME:
	    case HG_TYPE_ARRAY:
	    case HG_TYPE_STRING:
	    case HG_TYPE_OPER:
	    push_stack:
		    if (!hg_stack_push(ostack, qexecobj)) {
			    hg_vm_set_error(vm, qparent,
					    HG_VM_e_stackoverflow);
			    return FALSE;
		    }
		    hg_stack_drop(estack, &err);
		    break;
	    case HG_TYPE_FILE:
		    if (!hg_quark_is_executable(qexecobj)) {
			    goto push_stack;
		    } else {
			    file = _HG_VM_LOCK (vm, qexecobj, &err);
			    if (file == NULL)
				    break;
			    hg_scanner_attach_file(vm->scanner, file);
			    if (!hg_scanner_scan(vm->scanner,
						 hg_vm_get_mem(vm),
						 &err)) {
				    hg_vm_set_error(vm, qparent,
						    HG_VM_e_syntaxerror);
				    retval = FALSE;
				    break;
			    }
			    qresult = hg_scanner_get_token(vm->scanner);
			    hg_vm_quark_set_attributes(vm, &qresult);
#if defined (HG_DEBUG) && defined (HG_VM_DEBUG)
			    G_STMT_START {
				    hg_quark_t qs;
				    hg_string_t *s;

				    qs = hg_vm_quark_to_string(vm, qresult, (gpointer *)&s, &err);
				    if (qs == Qnil) {
					    if (err) {
						    g_print("WW: Unable to look up the scanned object: %lx: %s\n", qresult, err->message);
						    g_clear_error(&err);
					    } else {
						    g_print("WW: Unable to look up the scanned object: %lx\n", qresult);
					    }
				    } else {
					    gchar *cstr = hg_string_get_cstr(s);

					    g_print("I(%d): scanning... %s [%s]\n",
						    vm->n_nest_scan, cstr,
						    hg_quark_get_type_name(qresult));
					    g_free(cstr);
					    /* this is an instant object.
					     * surely no reference to the container.
					     * so it can be safely destroyed.
					     */
					    hg_string_free(s, TRUE);
				    }
			    } G_STMT_END;
#endif
			    /* exception for processing the executable array */
			    if (HG_IS_QNAME (qresult) &&
				(name = hg_name_lookup(vm->name, qresult)) != NULL) {
				    if (!strcmp(name, "{")) {
					    qresult = hg_vm_step_in_exec_array(vm, qparent);
					    if (qresult == Qnil)
						    return FALSE;
				    } else if (!strcmp(name, "}")) {
					    gssize i, idx = 0;
					    hg_quark_t q;
					    hg_array_t *a;

					    while (1) {
						    q = hg_stack_index(ostack, idx, &err);
						    if (err) {
							    hg_vm_set_error(vm, qparent,
									    HG_VM_e_unmatchedmark);
							    goto finalize;
						    }
						    if (HG_IS_QMARK (q))
							    break;
						    idx++;
					    }
					    qresult = hg_array_new(hg_vm_get_mem(vm),
								   idx,
								   (gpointer *)&a);
					    if (qresult == Qnil) {
						    hg_vm_set_error(vm, qparent,
								    HG_VM_e_VMerror);
						    return FALSE;
					    }
					    hg_vm_quark_set_attributes(vm, &qresult);
					    hg_quark_set_executable(&qresult, TRUE);
					    for (i = idx - 1; i >= 0; i--) {
						    q = hg_stack_index(ostack, i, &err);
						    if (err)
							    goto finalize;
						    hg_array_set(a, q, idx - i - 1, &err);
						    if (err)
							    goto finalize;
					    }
					    _HG_VM_UNLOCK (vm, qresult);

					    for (i = 0; i <= idx; i++) {
						    hg_stack_drop(ostack, &err);
						    if (err) {
							    hg_vm_set_error(vm, qparent,
									    HG_VM_e_stackunderflow);
							    goto finalize;
						    }
					    }
					    if (!hg_stack_push(ostack, qresult))
						    hg_vm_set_error(vm, qparent,
								    HG_VM_e_stackoverflow);

					    return FALSE;
				    }
			    }
			    hg_stack_push(estack, qresult);
		    }
		    break;
	    default:
		    g_warning("Unknown object type: %d\n", hg_quark_get_type(qexecobj));
		    return FALSE;
	}

  finalize:
	if (err) {
		g_warning("%s (code: %d)",
			  err->message,
			  err->code);
		if (!hg_vm_has_error(vm))
			hg_vm_set_error(vm, qexecobj, HG_VM_e_VMerror);
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

static hg_quark_t
hg_vm_step_in_exec_array(hg_vm_t    *vm,
			 hg_quark_t  qparent)
{
	hg_stack_t *old_ostack;
	hg_quark_t retval = Qnil;

	vm->n_nest_scan++;

	old_ostack = vm->stacks[HG_VM_STACK_OSTACK];
	vm->stacks[HG_VM_STACK_OSTACK] = hg_stack_new(__hg_vm_mem,
						      65535, vm);
	if (vm->stacks[HG_VM_STACK_OSTACK] == NULL) {
		/* unable to dup the correct stacks in this case */
		vm->stacks[HG_VM_STACK_OSTACK] = old_ostack;
		hg_vm_set_error(vm, qparent, HG_VM_e_VMerror);

		return FALSE;
	}

	hg_stack_push(vm->stacks[HG_VM_STACK_OSTACK],
		      HG_QMARK);

	while (hg_vm_stepi_in_exec_array(vm, qparent));
	if (!hg_vm_has_error(vm)) {
		retval = hg_stack_index(vm->stacks[HG_VM_STACK_OSTACK],
					0, NULL);
	}

	hg_stack_free(vm->stacks[HG_VM_STACK_OSTACK]);
	vm->stacks[HG_VM_STACK_OSTACK] = old_ostack;

	vm->n_nest_scan--;

	return retval;
}

static gboolean
_hg_vm_quark_iterate_gc_mark(hg_quark_t   qdata,
			     gpointer     user_data,
			     GError     **error)
{
	return hg_vm_quark_gc_mark((hg_vm_t *)user_data, qdata, error);
}

static hg_quark_t
_hg_vm_quark_iterate_copy(hg_quark_t   qdata,
			  gpointer     user_data,
			  gpointer    *ret,
			  GError     **error)
{
	return hg_vm_quark_copy((hg_vm_t *)user_data, qdata, ret, error);
}

static hg_quark_t
_hg_vm_quark_iterate_to_cstr(hg_quark_t   qdata,
			     gpointer     user_data,
			     gpointer    *ret,
			     GError     **error)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;
	hg_string_t *s = NULL;
	hg_quark_t q;
	gchar *cstr = NULL;

	q = hg_vm_quark_to_string(vm, qdata, (gpointer *)&s, error);
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

static gboolean
_hg_vm_quark_complex_compare(hg_quark_t q1,
			     hg_quark_t q2,
			     gpointer   user_data)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;
	gboolean retval = FALSE;

	switch (hg_quark_get_type(q1)) {
	    case HG_TYPE_STRING:
		    G_STMT_START {
			    hg_string_t *s1, *s2;

			    s1 = _HG_VM_LOCK (vm, q1, NULL);
			    s2 = _HG_VM_LOCK (vm, q2, NULL);
			    if (s1 == NULL ||
				s2 == NULL) {
				    g_warning("%s: Unable to obtain the actual object.", __PRETTY_FUNCTION__);
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
	    case HG_TYPE_SAVE:
	    case HG_TYPE_STACK:
		    retval = (q1 == q2);
		    break;
	    default:
		    g_warn_if_reached();
		    break;
	}

	return retval;
}

static gboolean
_hg_vm_rs_gc(hg_mem_t    *mem,
	     hg_quark_t   qdata,
	     gpointer     data,
	     GError     **error)
{
	hg_vm_t *vm = data;
	GError *err = NULL;
	gboolean retval;

	retval = hg_vm_quark_gc_mark(vm, qdata, &err);
	if (!retval && err == NULL) {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "GC failed");
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

#if 0
static gboolean
_hg_vm_run_gc_in_stack(hg_mem_t    *mem,
		       hg_quark_t   qdata,
		       gpointer     data,
		       GError     **error)
{
	hg_vm_t *vm = data;
	gboolean retval;

	retval = hg_vm_quark_gc_mark(vm, qdata, error);
	if (!retval && error && *error == NULL) {
		g_set_error(error, HG_ERROR, ENOMEM,
			    "GC failed");
	}

	return retval;
}
#endif

static gboolean
_hg_vm_run_gc(hg_mem_t *mem,
	      gpointer  user_data)
{
	hg_vm_t *vm = user_data;
	gsize i;
	GError *err = NULL;

	hg_return_val_if_fail (mem != NULL, FALSE);
	hg_return_val_if_fail (vm != NULL, FALSE);

	/* marking objects */
	/** marking I/O **/
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: marking file objects\n");
#endif
	for (i = 0; i < HG_FILE_IO_END; i++) {
		if (!hg_vm_quark_gc_mark(vm, vm->qio[i], &err))
			goto error;
	}
	/** marking in stacks **/
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: marking objects in stacks\n");
#endif
	for (i = 0; i < HG_VM_STACK_END; i++) {
		hg_stack_t *s = vm->stacks[i];

		if (!hg_object_gc_mark((hg_object_t *)s,
				       _hg_vm_quark_iterate_gc_mark,
				       vm, &err))
			goto error;
	}
	/** marking miscellaneous **/
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: marking objects in $error\n");
#endif
	if (!hg_vm_quark_gc_mark(vm, vm->qerror, &err))
		goto error;
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: marking objects in systemdict\n");
#endif
	if (!hg_vm_quark_gc_mark(vm, vm->qsystemdict, &err))
		goto error;
#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: marking objects in globaldict\n");
#endif
	if (!hg_vm_quark_gc_mark(vm, vm->qglobaldict, &err))
		goto error;
	/** XXX: marking scanner */

	/* sweeping objects */

	return TRUE;
  error:
	return FALSE;
}

/*< public >*/
/**
 * hg_vm_init:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_init(void)
{
	if (!__hg_vm_is_initialized) {
		__hg_vm_mem = hg_mem_new(HG_VM_MEM_SIZE);
		if (__hg_vm_mem == NULL)
			return FALSE;

		__hg_vm_is_initialized = TRUE;

		/* initialize objects */
		hg_object_init();
		HG_ARRAY_INIT;
		HG_DICT_INIT;
		HG_FILE_INIT;
		HG_STACK_INIT;
		HG_STRING_INIT;
		if (!hg_encoding_init())
			goto error;
		if (!hg_operator_init())
			goto error;
	}

	return TRUE;
  error:
	hg_vm_tini();

	return FALSE;
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
		hg_object_tini();

		if (__hg_vm_mem)
			hg_mem_destroy(__hg_vm_mem);
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

	q = hg_mem_alloc(__hg_vm_mem, sizeof (hg_vm_t), (gpointer *)&retval);
	if (q == Qnil)
		return NULL;

	memset(retval, 0, sizeof (hg_vm_t));
	retval->self = q;
	retval->name = hg_name_init();
	retval->current_mem_index = HG_VM_MEM_GLOBAL;
	retval->mem[HG_VM_MEM_GLOBAL] = hg_mem_new(HG_VM_GLOBAL_MEM_SIZE);
	retval->mem[HG_VM_MEM_LOCAL] = hg_mem_new(HG_VM_LOCAL_MEM_SIZE);
	if (retval->mem[HG_VM_MEM_GLOBAL] == NULL ||
	    retval->mem[HG_VM_MEM_LOCAL] == NULL)
		goto error;
	retval->mem_id[HG_VM_MEM_GLOBAL] = hg_mem_get_id(retval->mem[HG_VM_MEM_GLOBAL]);
	retval->mem_id[HG_VM_MEM_LOCAL] = hg_mem_get_id(retval->mem[HG_VM_MEM_LOCAL]);
	hg_mem_set_garbage_collector(retval->mem[HG_VM_MEM_GLOBAL], _hg_vm_run_gc, retval);
	hg_mem_set_garbage_collector(retval->mem[HG_VM_MEM_LOCAL], _hg_vm_run_gc, retval);
	hg_mem_reserved_spool_set_garbage_collector(retval->mem[HG_VM_MEM_GLOBAL], _hg_vm_rs_gc, retval);
	hg_mem_reserved_spool_set_garbage_collector(retval->mem[HG_VM_MEM_LOCAL], _hg_vm_rs_gc, retval);

	/* XXX: we prefer not to use the global nor the local
	 * memory spool to postpone thinking of how to deal
	 * with the yacc instance on GC.
	 */
	retval->scanner = hg_scanner_new(__hg_vm_mem,
					 retval->name);
	if (retval->scanner == NULL)
		goto error;
	retval->stacks[HG_VM_STACK_OSTACK] = hg_stack_new(__hg_vm_mem,
							  65535, retval);
	retval->stacks[HG_VM_STACK_ESTACK] = hg_stack_new(__hg_vm_mem,
							  65535, retval);
	retval->stacks[HG_VM_STACK_DSTACK] = hg_stack_new(__hg_vm_mem,
							  65535, retval);
	if (retval->stacks[HG_VM_STACK_OSTACK] == NULL ||
	    retval->stacks[HG_VM_STACK_ESTACK] == NULL ||
	    retval->stacks[HG_VM_STACK_DSTACK] == NULL)
		goto error;

	hg_vm_set_default_attributes(retval, HG_VM_ACCESS_READABLE|HG_VM_ACCESS_WRITABLE);

#define DECL_ERROR(_v_,_n_)						\
	(_v_)->qerror_name[HG_VM_e_ ## _n_] = HG_QNAME ((_v_)->name, #_n_);

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

	return retval;
  error:
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
	hg_return_if_fail (vm != NULL);
	gsize i;

	hg_scanner_destroy(vm->scanner);
	for (i = 0; i < HG_VM_STACK_END; i++) {
		if (vm->stacks[i])
			hg_stack_free(vm->stacks[i]);
	}
	for (i = 0; i < HG_FILE_IO_END; i++) {
		if (vm->qio[i] != Qnil)
			hg_vm_mfree(vm, vm->qio[i]);
	}
	for (i = 0; i < HG_VM_MEM_END; i++) {
		hg_mem_destroy(vm->mem[i]);
	}
	if (vm->name)
		hg_name_tini(vm->name);
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
hg_vm_malloc(hg_vm_t  *vm,
	     gsize     size,
	     gpointer *ret)
{
	hg_return_val_if_fail (vm != NULL, Qnil);

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
hg_vm_realloc(hg_vm_t    *vm,
	      hg_quark_t  qdata,
	      gsize       size,
	      gpointer   *ret)
{
	hg_return_val_if_fail (vm != NULL, Qnil);

	return hg_mem_realloc(_hg_vm_get_mem(vm, qdata), qdata, size, ret);
}

/**
 * hg_vm_mfree:
 * @vm:
 * @qdata:
 *
 * FIXME
 */
void
hg_vm_mfree(hg_vm_t    *vm,
	    hg_quark_t  qdata)
{
	hg_return_if_fail (vm != NULL);

	if (qdata == Qnil)
		return;

	if (!hg_quark_is_simple_object(qdata) &&
	    !HG_IS_QOPER (qdata)) {
		hg_object_free(_hg_vm_get_mem(vm, qdata),
			       qdata);
	} else {
		hg_mem_free(_hg_vm_get_mem(vm, qdata), qdata);
	}
}

/**
 * hg_vm_lock_object:
 * @vm:
 * @qdata:
 * @pretty_function:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gpointer
hg_vm_lock_object(hg_vm_t      *vm,
		  hg_quark_t    qdata,
		  const gchar  *pretty_function,
		  GError      **error)
{
	hg_return_val_if_fail (vm != NULL, NULL);

	return _hg_vm_real_lock_object(vm, qdata, pretty_function, error);
}

/**
 * hg_vm_unlock_object:
 * @vm:
 * @qdata:
 *
 * FIXME
 */
void
hg_vm_unlock_object(hg_vm_t    *vm,
		    hg_quark_t  qdata)
{
	hg_return_if_fail (vm != NULL);

	_hg_vm_real_unlock_object(vm, qdata);
}

/**
 * hg_vm_quark_gc_mark:
 * @vm:
 * @qdata:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_quark_gc_mark(hg_vm_t     *vm,
		    hg_quark_t   qdata,
		    GError     **error)
{
	hg_object_t *o;
	GError *err = NULL;
	gboolean retval = FALSE;

	hg_return_val_with_gerror_if_fail (vm != NULL, FALSE, error);

	if (qdata == Qnil)
		return TRUE;

	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata))
		return TRUE;

	o = _HG_VM_LOCK (vm, qdata, &err);
	if (o) {
		retval = hg_object_gc_mark(o, _hg_vm_quark_iterate_gc_mark, vm, &err);

		_HG_VM_UNLOCK (vm, qdata);
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

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
hg_vm_quark_copy(hg_vm_t     *vm,
		 hg_quark_t   qdata,
		 gpointer    *ret,
		 GError     **error)
{
	hg_object_t *o;
	hg_quark_t retval = Qnil;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (vm != NULL, Qnil, error);

	if (qdata == Qnil)
		return Qnil;

	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata))
		return qdata;

	o = _HG_VM_LOCK (vm, qdata, &err);
	if (o) {
		retval = hg_object_copy(o, _hg_vm_quark_iterate_copy, vm, ret, &err);

		_HG_VM_UNLOCK (vm, qdata);
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
	} else {
		hg_quark_set_access_bits(&retval,
					 hg_quark_is_readable(qdata),
					 hg_quark_is_writable(qdata),
					 hg_quark_is_executable(qdata));
	}

	return retval;
}

/**
 * hg_vm_quark_to_string:
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
hg_vm_quark_to_string(hg_vm_t     *vm,
		      hg_quark_t   qdata,
		      gpointer    *ret,
		      GError     **error)
{
	hg_object_t *o;
	hg_quark_t retval = Qnil;
	hg_string_t *s;
	GError *err = NULL;
	const gchar const *types[] = {
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
		NULL
	};
	gchar *cstr;

	hg_return_val_if_fail (vm != NULL, Qnil);

	retval = hg_string_new(hg_vm_get_mem(vm),
			       65535, (gpointer *)&s);
	if (retval == Qnil) {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "Out of memory.");
		goto error;
	}
	if (hg_quark_is_simple_object(qdata) ||
	    HG_IS_QOPER (qdata)) {
		switch (hg_quark_get_type(qdata)) {
		    case HG_TYPE_NULL:
		    case HG_TYPE_MARK:
			    hg_string_append(s, types[hg_quark_get_type(qdata)], -1, &err);
			    break;
		    case HG_TYPE_INT:
			    hg_string_append_printf(s, "%d", HG_INT (qdata));
			    break;
		    case HG_TYPE_REAL:
			    hg_string_append_printf(s, "%f", HG_REAL (qdata));
			    break;
		    case HG_TYPE_EVAL_NAME:
			    hg_string_append_printf(s, "//%s", HG_NAME (vm->name, qdata));
			    break;
		    case HG_TYPE_NAME:
			    if (hg_quark_is_executable(qdata))
				    hg_string_append_printf(s, "%s", HG_NAME (vm->name, qdata));
			    else
				    hg_string_append_printf(s, "/%s", HG_NAME (vm->name, qdata));
			    break;
		    case HG_TYPE_BOOL:
			    hg_string_append(s, HG_BOOL (qdata) ? "true" : "false", -1, &err);
			    break;
		    case HG_TYPE_OPER:
			    hg_string_append_printf(s, "%s",
						    hg_operator_get_name(qdata));
			    break;
		    default:
			    g_set_error(&err, HG_ERROR, EINVAL,
					"Unknown simple object type: %d",
					hg_quark_get_type(qdata));
			    goto error;
		}
	} else {
		if (!hg_quark_is_readable(qdata)) {
			hg_string_append(s, types[hg_quark_get_type(qdata)], -1, &err);
		} else {
			switch (hg_quark_get_type(qdata)) {
			    case HG_TYPE_STRING:
			    case HG_TYPE_DICT:
			    case HG_TYPE_ARRAY:
			    case HG_TYPE_FILE:
			    case HG_TYPE_SAVE:
			    case HG_TYPE_STACK:
				    o = _HG_VM_LOCK (vm, qdata, &err);
				    if (o) {
					    cstr = hg_object_to_cstr(o, _hg_vm_quark_iterate_to_cstr, vm, &err);
					    if (cstr && HG_IS_QARRAY (qdata) && hg_quark_is_executable(qdata)) {
						    if (cstr[0] == '[') {
							    cstr[0] = '{';
							    cstr[strlen(cstr)-1] = '}';
						    }
					    }
					    hg_string_append(s, cstr, -1, &err);

					    _HG_VM_UNLOCK (vm, qdata);
				    }
				    break;
			    default:
				    g_set_error(&err, HG_ERROR, EINVAL,
						"Unknown complex object type: %d",
						hg_quark_get_type(qdata));
				    goto error;
			}
		}
	}
  error:
	if (retval != Qnil) {
		if (!hg_string_fix_string_size(s)) {
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "Out of memory");
		}
		if (ret)
			*ret = s;
		else
			_HG_VM_UNLOCK (vm, retval);
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
		retval = Qnil;
	}

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
gboolean
hg_vm_quark_compare(hg_vm_t    *vm,
		    hg_quark_t  qdata1,
		    hg_quark_t  qdata2)
{
	return hg_quark_compare(qdata1, qdata2, _hg_vm_quark_complex_compare, vm);
}

/**
 * hg_vm_quark_set_attributes:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
void
hg_vm_quark_set_attributes(hg_vm_t    *vm,
			   hg_quark_t *qdata)
{
	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (qdata != NULL);

	if (*qdata == Qnil)
		return;
	/* do not reset an exec bit here */
	if (hg_quark_is_simple_object(*qdata)) {
		hg_quark_set_access_bits(qdata,
					 (vm->qattributes & HG_VM_ATTRIBUTE1 (HG_VM_ACCESS_READABLE)),
					 FALSE,
					 hg_quark_is_executable(*qdata));
	} else if (HG_IS_QOPER (*qdata)) {
		hg_quark_set_access_bits(qdata,
					 FALSE,
					 FALSE,
					 hg_quark_is_executable(*qdata));
	} else {
		_hg_quark_type_bit_set_bits(qdata,
					    HG_QUARK_TYPE_BIT_ACCESS1,
					    HG_QUARK_TYPE_BIT_ACCESS_END,
					    vm->qattributes);
	}
}

/**
 * hg_vm_set_default_attributes:
 * @vm:
 * @qattributes:
 *
 * FIXME
 */
void
hg_vm_set_default_attributes(hg_vm_t    *vm,
			     guint       qattributes)
{
	hg_return_if_fail (vm != NULL);

	vm->qattributes = HG_VM_ATTRIBUTE1 (qattributes);
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
	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (vm->current_mem_index < HG_VM_MEM_END, NULL);

	return vm->mem[vm->current_mem_index];
}

/**
 * hg_vm_get_mem_from_quark:
 * @vm:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_vm_get_mem_from_quark(hg_vm_t    *vm,
			 hg_quark_t  qdata)
{
	return _hg_vm_get_mem(vm, qdata);
}

/**
 * hg_vm_use_global_mem:
 * @vm:
 * @flag:
 *
 * FIXME
 */
void
hg_vm_use_global_mem(hg_vm_t  *vm,
		     gboolean  flag)
{
	hg_return_if_fail (vm != NULL);

	if (flag)
		vm->current_mem_index = HG_VM_MEM_GLOBAL;
	else
		vm->current_mem_index = HG_VM_MEM_LOCAL;
}

/**
 * hg_vm_is_global_mem_used:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_is_global_mem_used(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, TRUE);

	return vm->current_mem_index == HG_VM_MEM_GLOBAL;
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
	hg_return_val_if_fail (vm != NULL, Qnil);
	hg_return_val_if_fail (type < HG_FILE_IO_END, Qnil);

	if (type == HG_FILE_IO_LINEEDIT) {
		return hg_file_new_with_vtable(hg_vm_get_mem(vm),
					       "%lineedit",
					       HG_FILE_IO_MODE_READ,
					       hg_file_get_lineedit_vtable(),
					       vm->lineedit,
					       NULL,
					       NULL);
	} else if (type == HG_FILE_IO_STATEMENTEDIT) {
		return hg_file_new_with_vtable(hg_vm_get_mem(vm),
					       "%statementedit",
					       HG_FILE_IO_MODE_READ,
					       hg_file_get_lineedit_vtable(),
					       vm->lineedit,
					       NULL,
					       NULL);
	}

	return vm->qio[type];
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
	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (type < HG_FILE_IO_END);
	hg_return_if_fail (HG_IS_QFILE (file));

	vm->qio[type] = file;
}

/**
 * hg_vm_setup:
 * @vm:
 * @lang_level:
 * @stdin:
 * @stdout:
 * @stderr:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_setup(hg_vm_t           *vm,
	    hg_vm_langlevel_t  lang_level,
	    hg_quark_t         stdin,
	    hg_quark_t         stdout,
	    hg_quark_t         stderr)
{
	hg_quark_t qf;
	hg_dict_t *dict = NULL, *dict_error;
	hg_file_t *fstdin, *fstdout;
	GError *err = NULL;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (lang_level < HG_LANG_LEVEL_END, FALSE);

	if (vm->is_initialized)
		return TRUE;

	vm->is_initialized = TRUE;

	hg_vm_use_global_mem(vm, TRUE);

	vm->n_nest_scan = 0;

	/* initialize I/O */
	if (stdin == Qnil) {
		qf = hg_file_new(hg_vm_get_mem(vm),
				 "%stdin",
				 HG_FILE_IO_MODE_READ,
				 NULL,
				 (gpointer *)&fstdin);
	} else {
		qf = stdin;
	}
	hg_quark_set_access_bits(&qf, TRUE, FALSE, FALSE);
	vm->qio[HG_FILE_IO_STDIN] = qf;
	if (stdout == Qnil) {
		qf  = hg_file_new(hg_vm_get_mem(vm),
				  "%stdout",
				  HG_FILE_IO_MODE_WRITE,
				  NULL,
				  (gpointer *)&fstdout);
	} else {
		qf = stdout;
	}
	hg_quark_set_access_bits(&qf, FALSE, TRUE, FALSE);
	vm->qio[HG_FILE_IO_STDOUT] = qf;
	if (stderr == Qnil) {
		qf = hg_file_new(hg_vm_get_mem(vm),
				 "%stderr",
				 HG_FILE_IO_MODE_WRITE,
				 NULL,
				 NULL);
	} else {
		qf = stderr;
	}
	hg_quark_set_access_bits(&qf, FALSE, TRUE, FALSE);
	vm->qio[HG_FILE_IO_STDERR] = qf;

	if (vm->qio[HG_FILE_IO_STDIN] == Qnil ||
	    vm->qio[HG_FILE_IO_STDOUT] == Qnil ||
	    vm->qio[HG_FILE_IO_STDERR] == Qnil) {
		goto error;
	}

	vm->lineedit = hg_lineedit_new(hg_vm_get_mem(vm),
				       hg_lineedit_get_default_vtable(),
				       fstdin,
				       fstdout);
	if (vm->lineedit == NULL)
		goto error;

	/* initialize stacks */
	hg_stack_clear(vm->stacks[HG_VM_STACK_OSTACK]);
	hg_stack_clear(vm->stacks[HG_VM_STACK_ESTACK]);
	hg_stack_clear(vm->stacks[HG_VM_STACK_DSTACK]);

	/* initialize dictionaries */
	vm->qsystemdict = hg_dict_new(vm->mem[HG_VM_MEM_GLOBAL],
				      65535,
				      NULL);
	vm->qglobaldict = hg_dict_new(vm->mem[HG_VM_MEM_GLOBAL],
				      65535,
				      NULL);
	vm->qerror = hg_dict_new(vm->mem[HG_VM_MEM_LOCAL],
				 65535,
				 (gpointer *)&dict_error);
	if (vm->qsystemdict == Qnil ||
	    vm->qglobaldict == Qnil ||
	    vm->qerror == Qnil)
		goto error;

	hg_quark_set_access_bits(&vm->qsystemdict, TRUE, TRUE, FALSE);
	hg_quark_set_access_bits(&vm->qglobaldict, TRUE, TRUE, FALSE);
	hg_quark_set_access_bits(&vm->qerror, TRUE, TRUE, FALSE);

	hg_stack_push(vm->stacks[HG_VM_STACK_DSTACK], vm->qsystemdict);
	if (lang_level >= HG_LANG_LEVEL_2) {
		hg_stack_push(vm->stacks[HG_VM_STACK_DSTACK],
			      vm->qglobaldict);
	}
	/* userdict will be created/pushed in PostScript */

	/* initialize $error */
	if (!hg_dict_add(dict_error,
			 HG_QNAME (vm->name, "newerror"),
			 HG_QBOOL (FALSE),
			 &err))
		goto error;
	if (!hg_dict_add(dict_error,
			 HG_QNAME (vm->name, "errorname"),
			 HG_QNULL,
			 &err))
		goto error;
	if (!hg_dict_add(dict_error,
			 HG_QNAME (vm->name, ".isstop"),
			 HG_QBOOL (FALSE),
			 &err))
		goto error;

	/* add built-in items into systemdict */
	dict = _HG_VM_LOCK (vm, vm->qsystemdict, NULL);
	if (dict == NULL)
		goto error;

	if (!hg_dict_add(dict,
			 HG_QNAME (vm->name, "systemdict"),
			 vm->qsystemdict,
			 &err))
		goto error;
	if (!hg_dict_add(dict,
			 HG_QNAME (vm->name, "globaldict"),
			 vm->qglobaldict,
			 &err))
		goto error;
	if (!hg_dict_add(dict,
			 HG_QNAME (vm->name, "$error"),
			 vm->qerror,
			 &err))
		goto error;

	/* initialize build-in operators */
	if (!hg_operator_register(dict, vm->name, lang_level))
		goto error;

	_HG_VM_UNLOCK (vm, vm->qsystemdict);

	vm->language_level = lang_level;

	return TRUE;
  error:
	if (dict)
		_HG_VM_UNLOCK (vm, vm->qsystemdict);
	if (err) {
		g_warning("%s (code: %d)",
			  err->message,
			  err->code);
		g_error_free(err);
	}

	hg_vm_finish(vm);

	return FALSE;
}

/**
 * hg_vm_finish:
 * @vm:
 *
 * FIXME
 */
void
hg_vm_finish(hg_vm_t *vm)
{
	gsize i;

	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (vm->is_initialized);

	if (vm->qsystemdict != Qnil) {
		hg_vm_mfree(vm, vm->qsystemdict);
		vm->qsystemdict = Qnil;
	}
	if (vm->qglobaldict != Qnil) {
		hg_vm_mfree(vm, vm->qglobaldict);
		vm->qglobaldict = Qnil;
	}
	for (i = 0; i < HG_VM_STACK_END; i++) {
		if (vm->stacks[i])
			hg_stack_free(vm->stacks[i]);
	}
	/* XXX: finalizing I/O */

	if (vm->lineedit) {
		hg_lineedit_destroy(vm->lineedit);
		vm->lineedit = NULL;
	}

	vm->is_initialized = FALSE;
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
	hg_return_val_if_fail (vm != NULL, HG_LANG_LEVEL_END);

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
gboolean
hg_vm_set_language_level(hg_vm_t           *vm,
			 hg_vm_langlevel_t  level)
{
	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (level < HG_LANG_LEVEL_END, FALSE);

	/* re-initialize operators */
	if (vm->is_initialized)
		hg_vm_finish(vm);

	return hg_vm_setup(vm, level, Qnil, Qnil, Qnil);
}

/**
 * hg_vm_dict_lookup:
 * @vm:
 * @qname:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_dict_lookup(hg_vm_t    *vm,
		  hg_quark_t  qname)
{
	hg_quark_t retval = Qnil, quark;
	hg_stack_t *dstack;
	GError *err = NULL;
	hg_vm_dict_lookup_data_t ldata;

	hg_return_val_if_fail (vm != NULL, Qnil);

	if (HG_IS_QSTRING (qname)) {
		gchar *str;
		hg_string_t *s;

		s = _HG_VM_LOCK (vm, qname, &err);
		if (s == NULL)
			goto error;
		str = hg_string_get_cstr(s);
		quark = hg_name_new_with_string(vm->name, str, -1);

		g_free(str);
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
	hg_stack_foreach(dstack, _hg_vm_real_dict_lookup, &ldata, &err);
	retval = ldata.result;
  error:
	if (err) {
		/* XXX: remove name too to reduce the memory usage */
		g_error_free(err);
	} else {
		retval = ldata.result;
	}

	return retval;
}

/**
 * hg_vm_dict_remove:
 * @vm:
 * @qname:
 * @remove_all:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_dict_remove(hg_vm_t    *vm,
		  hg_quark_t  qname,
		  gboolean    remove_all)
{
	hg_stack_t *dstack;
	GError *err = NULL;
	hg_vm_dict_remove_data_t ldata;
	gboolean retval = FALSE;
	hg_quark_t quark;

	hg_return_val_if_fail (vm != NULL, FALSE);

	if (HG_IS_QSTRING (qname)) {
		gchar *str;
		hg_string_t *s;

		s = _HG_VM_LOCK (vm, qname, &err);
		if (s == NULL)
			goto error;
		str = hg_string_get_cstr(s);
		quark = hg_name_new_with_string(vm->name, str, -1);

		g_free(str);
		_HG_VM_UNLOCK (vm, qname);
	} else {
		quark = qname;
	}
	dstack = vm->stacks[HG_VM_STACK_DSTACK];
	ldata.vm = vm;
	ldata.result = FALSE;
	ldata.qname = qname;
	ldata.remove_all = remove_all;
	hg_stack_foreach(dstack, _hg_vm_real_dict_remove, &ldata, &err);

  error:
	if (err) {
		/* XXX */
		g_error_free(err);
	} else {
		retval = ldata.result;
	}

	return retval;
}

/**
 * hg_vm_get_dict:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_vm_get_dict(hg_vm_t *vm)
{
	hg_quark_t qdict;
	GError *err = NULL;

	hg_return_val_if_fail (vm != NULL, Qnil);

	qdict = hg_stack_index(vm->stacks[HG_VM_STACK_DSTACK], 0, &err);
	if (err) {
		/* XXX */
		g_error_free(err);
	}

	return qdict;
}

/**
 * hg_vm_stepi:
 * @vm:
 * @is_proceeded:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_stepi(hg_vm_t  *vm,
	    gboolean *is_proceeded)
{
	hg_stack_t *estack, *ostack;
	hg_quark_t qexecobj, qresult;
	hg_file_t *file;
	GError *err = NULL;
	gboolean retval = TRUE;
	const gchar *name;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (is_proceeded != NULL, FALSE);

	ostack = vm->stacks[HG_VM_STACK_OSTACK];
	estack = vm->stacks[HG_VM_STACK_ESTACK];

	qexecobj = hg_stack_index(estack, 0, &err);
	*is_proceeded = FALSE;
	if (err) {
		hg_vm_set_error(vm, Qnil, HG_VM_e_stackunderflow);
		return FALSE;
	}

  evaluate:
#if defined (HG_DEBUG) && defined (HG_VM_DEBUG)
	G_STMT_START {
		hg_quark_t qs;
		hg_string_t *s;

		qs = hg_vm_quark_to_string(vm, qexecobj, (gpointer *)&s, &err);
		if (qs == Qnil) {
			if (err) {
				g_print("W: Unable to look up the object being executed: %lx: %s\n", qexecobj, err->message);
				g_clear_error(&err);
			} else {
				g_print("W: Unable to look up the object being executed: %lx\n", qexecobj);
			}
		} else {
			gchar *cstr = hg_string_get_cstr(s);

			g_print("I: executing... %s [%s]\n",
				cstr, hg_quark_get_type_name(qexecobj));
			g_free(cstr);
			/* this is an instant object.
			 * surely no reference to the container.
			 * so it can be safely destroyed.
			 */
			hg_string_free(s, TRUE);
		}
	} G_STMT_END;
#endif
	switch (hg_quark_get_type(qexecobj)) {
	    case HG_TYPE_NULL:
	    case HG_TYPE_INT:
	    case HG_TYPE_REAL:
	    case HG_TYPE_BOOL:
	    case HG_TYPE_DICT:
	    case HG_TYPE_MARK:
	    push_stack:
		    if (!hg_stack_push(ostack, qexecobj)) {
			    hg_vm_set_error(vm, qexecobj, HG_VM_e_stackoverflow);
			    return TRUE;
		    }
		    hg_stack_drop(estack, &err);
		    *is_proceeded = TRUE;
		    break;
	    case HG_TYPE_EVAL_NAME:
		    if (hg_quark_is_executable(qexecobj)) {
			    qresult = hg_vm_dict_lookup(vm, qexecobj);
			    if (qresult == Qnil) {
				    hg_vm_set_error(vm, qexecobj,
						    HG_VM_e_undefined);

				    return TRUE;
			    }
			    qexecobj = qresult;
			    goto evaluate;
		    }
		    g_warning("Immediately evaluated name object somehow doesn't have a exec bit turned on: %s", hg_name_lookup(vm->name, qexecobj));
		    goto push_stack;
	    case HG_TYPE_NAME:
		    /* /foo  ... nope
		     * foo   ... exec bit
		     */
		    if (hg_quark_is_executable(qexecobj)) {
			    hg_quark_t q;

			    qresult = hg_vm_dict_lookup(vm, qexecobj);
			    if (qresult == Qnil) {
				    hg_vm_set_error(vm, qexecobj,
						    HG_VM_e_undefined);

				    return TRUE;
			    }
			    q = hg_vm_quark_copy(vm, qresult, NULL, &err);
			    if (err) {
				    hg_vm_set_error(vm, qexecobj,
						    HG_VM_e_VMerror);
				    return TRUE;
			    }
			    if (!hg_stack_push(estack, q)) {
				    hg_vm_set_error(vm, qexecobj,
						    HG_VM_e_execstackoverflow);

				    return TRUE;
			    }
			    hg_stack_roll(estack, 2, 1, &err);
			    hg_stack_drop(estack, &err);
			    break;
		    }
		    goto push_stack;
	    case HG_TYPE_ARRAY:
		    /* [ ... ] ... nope
		     * { ... } ... exec bit
		     */
		    if (hg_quark_is_executable(qexecobj)) {
			    hg_array_t *a = _HG_VM_LOCK (vm, qexecobj, &err);

			    if (a == NULL) {
				    hg_vm_set_error(vm, qexecobj, HG_VM_e_VMerror);
				    return TRUE;
			    }
			    if (hg_array_length(a) > 0) {
				    qresult = hg_array_get(a, 0, &err);
				    if (err) {
					    hg_vm_set_error(vm, qexecobj, HG_VM_e_VMerror);
					    goto a_error;
				    }
				    if (hg_quark_is_executable(qresult) &&
					(HG_IS_QNAME (qresult) ||
					 HG_IS_QOPER (qresult))) {
					    if (!hg_stack_push(estack, qresult)) {
						    hg_vm_set_error(vm, qexecobj,
								    HG_VM_e_execstackoverflow);
						    goto a_error;
					    }
				    } else {
					    if (!hg_stack_push(ostack, qresult)) {
						    hg_vm_set_error(vm, qexecobj,
								    HG_VM_e_stackoverflow);
						    goto a_error;
					    }
				    }
				    hg_array_remove(a, 0);
			    } else {
				    hg_stack_drop(estack, &err);
			    }
		      a_error:
			    _HG_VM_UNLOCK (vm, qexecobj);
			    break;
		    }
		    goto push_stack;
	    case HG_TYPE_STRING:
		    if (hg_quark_is_executable(qexecobj)) {
			    hg_string_t *s;

			    s = _HG_VM_LOCK (vm, qexecobj, &err);
			    if (s == NULL)
				    break;
			    qresult = hg_file_new_with_string(hg_vm_get_mem(vm),
							      "%sfile",
							      HG_FILE_IO_MODE_READ,
							      s,
							      NULL,
							      &err,
							      NULL);
			    hg_quark_set_executable(&qresult, TRUE);
			    _HG_VM_UNLOCK (vm, qexecobj);
			    hg_stack_drop(estack, &err);
			    if (!hg_stack_push(estack, qresult)) {
				    hg_vm_set_error(vm, qexecobj,
						    HG_VM_e_execstackoverflow);

				    return TRUE;
			    }
			    break;
		    }
		    goto push_stack;
	    case HG_TYPE_OPER:
		    retval = hg_operator_invoke(qexecobj,
						vm,
						&err);
		    if (retval) {
			    hg_stack_drop(estack, &err);
		    }
		    *is_proceeded = TRUE;
		    break;
	    case HG_TYPE_FILE:
		    if (!hg_quark_is_executable(qexecobj)) {
			    goto push_stack;
		    } else {
			    file = _HG_VM_LOCK (vm, qexecobj, &err);
			    if (file == NULL)
				    break;
			    hg_scanner_attach_file(vm->scanner, file);
			    if (!hg_scanner_scan(vm->scanner,
						 hg_vm_get_mem(vm),
						 &err)) {
				    if (!err && hg_file_is_eof(file)) {
#if defined (HG_DEBUG) && defined (HG_VM_DEBUG)
					    g_print("I: EOF detected\n");
#endif
					    hg_stack_drop(estack, &err);
					    *is_proceeded = TRUE;
					    break;
				    }
				    g_print("%s\n", err->message);
				    hg_vm_set_error(vm, qexecobj,
						    HG_VM_e_syntaxerror);

				    return TRUE;
			    }
			    qresult = hg_scanner_get_token(vm->scanner);
			    hg_vm_quark_set_attributes(vm, &qresult);
#if defined (HG_DEBUG) && defined (HG_VM_DEBUG)
			    G_STMT_START {
				    hg_quark_t qs;
				    hg_string_t *s;

				    qs = hg_vm_quark_to_string(vm, qresult, (gpointer *)&s, &err);
				    if (qs == Qnil) {
					    if (err) {
						    g_print("W: Unable to look up the scanned object: %lx: %s\n", qresult, err->message);
						    g_clear_error(&err);
					    } else {
						    g_print("W: Unable to look up the scanned object: %lx\n", qresult);
					    }
				    } else {
					    gchar *cstr = hg_string_get_cstr(s);

					    g_print("I: scanning... %s [%s]\n",
						    cstr, hg_quark_get_type_name(qresult));
					    g_free(cstr);
					    /* this is an instant object.
					     * surely no reference to the container.
					     * so it can be safely destroyed.
					     */
					    hg_string_free(s, TRUE);
				    }
			    } G_STMT_END;
#endif
			    /* exception for processing the executable array */
			    if (HG_IS_QNAME (qresult) &&
				(name = hg_name_lookup(vm->name, qresult)) != NULL) {
				    if (!strcmp(name, "{")) {
					    qresult = hg_vm_step_in_exec_array(vm, qexecobj);
					    if (qresult == Qnil)
						    break;
					    if (!hg_stack_push(ostack, qresult)) {
						    hg_vm_set_error(vm, qexecobj,
								    HG_VM_e_stackoverflow);
					    }
					    return TRUE;
				    } else if (!strcmp(name, "}")) {
					    hg_vm_set_error(vm, qexecobj, HG_VM_e_syntaxerror);
					    return TRUE;
				    }
			    }
			    hg_stack_push(estack, qresult);
		    }
		    break;
	    default:
		    g_warning("Unknown object type: %d\n", hg_quark_get_type(qexecobj));
		    return FALSE;
	}

	if (err) {
		if (!hg_vm_has_error(vm))
			hg_vm_set_error(vm, qexecobj, HG_VM_e_VMerror);
		/* XXX */
		g_warning("%s (code: %d)", err->message, err->code);
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

/**
 * hg_vm_step:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_step(hg_vm_t *vm)
{
	gboolean retval, is_proceeded;

	while ((retval = hg_vm_stepi(vm, &is_proceeded))) {
		if (is_proceeded)
			break;
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
gboolean
hg_vm_main_loop(hg_vm_t *vm)
{
	gboolean retval = TRUE;

	hg_return_val_if_fail (vm != NULL, FALSE);

	vm->shutdown = FALSE;
	while (!vm->shutdown) {
		gsize depth = hg_stack_depth(vm->stacks[HG_VM_STACK_ESTACK]);

		if (depth == 0)
			break;

		if (!hg_vm_step(vm)) {
			g_print("bah\n");
			/* XXX */
		}
	}

	return retval;
}

/**
 * hg_vm_find_libfile:
 * @vm:
 * @filename:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gchar *
hg_vm_find_libfile(hg_vm_t     *vm G_GNUC_UNUSED,
		   const gchar *filename)
{
	hg_return_val_if_fail (filename != NULL, NULL);

	return _hg_vm_find_file(filename);
}

/**
 * hg_vm_eval:
 * @vm:
 * @qeval:
 * @ostack:
 * @estack:
 * @dstack:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_eval(hg_vm_t     *vm,
	   hg_quark_t   qeval,
	   hg_stack_t  *ostack,
	   hg_stack_t  *estack,
	   hg_stack_t  *dstack,
	   GError     **error)
{
	hg_stack_t *old_ostack = NULL, *old_estack = NULL, *old_dstack = NULL;
	gboolean retval;

	hg_return_val_with_gerror_if_fail (vm != NULL, FALSE, error);

	if (hg_quark_is_simple_object(qeval)) {
		/* XXX */
		return FALSE;
	}
	switch(hg_quark_get_type(qeval)) {
	    case HG_TYPE_STRING:
	    case HG_TYPE_FILE:
		    if (!hg_quark_is_executable(qeval)) {
			    /* XXX */
			    return FALSE;
		    }
		    break;
	    default:
		    /* XXX */
		    return FALSE;
	}
	if (ostack) {
		old_ostack = vm->stacks[HG_VM_STACK_OSTACK];
		vm->stacks[HG_VM_STACK_OSTACK] = ostack;
	}
	if (estack) {
		old_estack = vm->stacks[HG_VM_STACK_ESTACK];
		vm->stacks[HG_VM_STACK_ESTACK] = estack;
	}
	if (dstack) {
		old_dstack = vm->stacks[HG_VM_STACK_DSTACK];
		vm->stacks[HG_VM_STACK_DSTACK] = dstack;
	}
	hg_stack_push(vm->stacks[HG_VM_STACK_ESTACK], qeval);
	retval = hg_vm_main_loop(vm);

	if (old_ostack)
		vm->stacks[HG_VM_STACK_OSTACK] = old_ostack;
	if (old_estack)
		vm->stacks[HG_VM_STACK_ESTACK] = old_estack;
	if (old_dstack)
		vm->stacks[HG_VM_STACK_DSTACK] = old_dstack;

	return retval;
}

/**
 * hg_vm_eval_from_cstring:
 * @vm:
 * @cstring:
 * @ostack:
 * @estack:
 * @dstack:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_eval_from_cstring(hg_vm_t      *vm,
			const gchar  *cstring,
			hg_stack_t   *ostack,
			hg_stack_t   *estack,
			hg_stack_t   *dstack,
			GError      **error)
{
	hg_quark_t qstring;
	GError *err = NULL;
	gboolean retval = FALSE;
	gchar *str;

	hg_return_val_with_gerror_if_fail (vm != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (cstring != NULL, FALSE, error);

	/* add \n at the end to avoid failing on scanner */
	str = g_strdup_printf("%s\n", cstring);
	qstring = HG_QSTRING (hg_vm_get_mem(vm),
			      str);
	g_free(str);
	if (qstring == Qnil) {
		g_set_error(&err, HG_ERROR, ENOMEM,
			    "Out of memory");
		goto error;
	}
	hg_quark_set_executable(&qstring, TRUE);
	retval = hg_vm_eval(vm, qstring, ostack, estack, dstack, &err);

	hg_vm_mfree(vm, qstring);
  error:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message, err->code);
		}
		g_error_free(err);
	}

	return retval;
}

/**
 * hg_vm_eval_from_file:
 * @vm:
 * @initfile:
 * @ostack:
 * @estack:
 * @dstack:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_eval_from_file(hg_vm_t      *vm,
		     const gchar  *initfile,
		     hg_stack_t   *ostack,
		     hg_stack_t   *estack,
		     hg_stack_t   *dstack,
		     GError      **error)
{
	gchar *filename;
	gboolean retval = FALSE;

	hg_return_val_with_gerror_if_fail (vm != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (initfile != NULL && initfile[0] != 0, FALSE, error);

	filename = _hg_vm_find_file(initfile);
	if (filename) {
		hg_quark_t qfile;
		GError *err = NULL;

		qfile = hg_file_new(hg_vm_get_mem(vm),
				    filename,
				    HG_FILE_IO_MODE_READ,
				    &err,
				    NULL);
		if (qfile == Qnil)
			goto error;
		hg_quark_set_executable(&qfile, TRUE);
		hg_vm_quark_set_attributes(vm, &qfile);
		retval = hg_vm_eval(vm, qfile, ostack, estack, dstack, &err);
		/* may better relying on GC
		 * hg_vm_mfree(vm, qfile);
		 */
	  error:
		if (err) {
			if (error) {
				*error = g_error_copy(err);
			} else {
				g_warning("%s (code: %d)",
					  err->message,
					  err->code);
			}
			g_error_free(err);
		}
	} else {
		g_warning("Unable to find a file `%s'", initfile);
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
gint
hg_vm_get_error_code(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, -1);

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
hg_vm_set_error_code(hg_vm_t *vm,
		     gint     error_code)
{
	hg_return_if_fail (vm != NULL);

	vm->error_code = error_code;
}

/**
 * hg_vm_startjob:
 * @vm:
 * @initializer:
 * @encapsulated:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_startjob(hg_vm_t           *vm,
	       hg_vm_langlevel_t  lang_level,
	       const gchar       *initializer,
	       gboolean           encapsulated)
{
	GError *err = NULL;
	hg_dict_t *dict;
	gboolean retval;

	hg_return_val_if_fail (vm != NULL, FALSE);

	/* initialize VM */
	if (!hg_vm_set_language_level(vm, lang_level)) {
		g_warning("Unable to initialize PostScript VM");
		return FALSE;
	}

	/* XXX: GC */

	/* XXX: restore memory */

	if (encapsulated) {
		/* XXX: save memory */
	}

	/* change the default memory */
	hg_vm_use_global_mem(vm, FALSE);

	retval = hg_vm_eval_from_file(vm, "hg_init.ps", NULL, NULL, NULL, &err);

	/* make systemdict read-only */
	dict = _HG_VM_LOCK (vm, vm->qsystemdict, &err);
	if (dict == NULL) {
		g_warning("Unable to obtain systemdict");
		return FALSE;
	}
	hg_quark_set_writable(&vm->qsystemdict, FALSE);
	if (!hg_dict_add(dict, HG_QNAME (vm->name, "systemdict"),
			 vm->qsystemdict,
			 NULL)) {
		g_warning("Unable to update the entry for systemdict");
		return FALSE;
	}
	_HG_VM_UNLOCK (vm, vm->qsystemdict);

	if (initializer && initializer[0] == 0) {
		return TRUE;
	} else if (initializer) {
		gchar *s = g_strdup_printf("{(%s)(r)file dup type/filetype eq{cvx exec}if}stopped{$error/newerror get{errordict/handleerror get exec 1 .quit}if}if", initializer);

		retval = hg_vm_eval_from_cstring(vm, s, NULL, NULL, NULL, NULL);

		g_free(s);

		return retval;
	}

	return (retval ? hg_vm_eval_from_cstring(vm, "systemdict/.loadhistory known{(.hghistory).loadhistory}if start systemdict/.savehistory known{(.hghistory).savehistory}if", NULL, NULL, NULL, NULL) : FALSE);
}

/**
 * hg_vm_shutdown:
 * @vm:
 * @error_code:
 *
 * FIXME
 */
void
hg_vm_shutdown(hg_vm_t *vm,
	       gint     error_code)
{
	hg_return_if_fail (vm != NULL);

	vm->error_code = error_code;
	vm->shutdown = TRUE;
}

/**
 * hg_vm_get_systemdict:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
#if 0
hg_dict_t *
hg_vm_get_systemdict(hg_vm_t *vm)
{
	return __hg_vm_systemdict;
}
#endif

/**
 * hg_vm_has_error:
 * @vm:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_has_error(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, TRUE);

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
	hg_dict_t *dict_error;

	hg_return_if_fail (vm != NULL);

	dict_error = _HG_VM_LOCK (vm, vm->qerror, NULL);
	if (dict_error == NULL) {
		g_printerr("Unable to obtain $error dict\n");
		return;
	}
	if (!hg_dict_add(dict_error,
			 HG_QNAME (vm->name, "newerror"),
			 HG_QBOOL (FALSE),
			 NULL))
		goto error;
	if (!hg_dict_add(dict_error,
			 HG_QNAME (vm->name, "errorname"),
			 HG_QNULL,
			 NULL))
		goto error;
	if (!hg_dict_add(dict_error,
			 HG_QNAME (vm->name, ".isstop"),
			 HG_QBOOL (FALSE),
			 NULL))
		goto error;

	vm->has_error = FALSE;

	return;
  error:
	g_printerr("Unable to reset entries in $error dict\n");
}

/**
 * hg_vm_set_error:
 * @vm:
 * @qdata:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_set_error(hg_vm_t       *vm,
		hg_quark_t     qdata,
		hg_vm_error_t  error)
{
	hg_quark_t qerrordict, qnerrordict, qhandler, q;
	hg_dict_t *errordict;
	GError *err = NULL;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (error < HG_VM_e_END, FALSE);

	if (vm->has_error) {
		hg_quark_t qerrorname = HG_QNAME (vm->name, "errorname");
		hg_quark_t qcommand = HG_QNAME (vm->name, "command");
		hg_quark_t qresult_err, qresult_cmd, qwhere;
		hg_dict_t *derror;
		hg_string_t *where;
		gchar *scommand, *swhere;

		derror = _HG_VM_LOCK (vm, vm->qerror, &err);
		if (derror == NULL)
			goto fatal_error;
		qresult_err = hg_dict_lookup(derror, qerrorname, &err);
		qresult_cmd = hg_dict_lookup(derror, qcommand, &err);
		_HG_VM_UNLOCK (vm, vm->qerror);

		if (qresult_cmd == Qnil) {
			scommand = g_strdup("-%unknown%-");
		} else {
			hg_string_t *s = NULL;

			q = hg_vm_quark_to_string(vm,
						  qresult_cmd,
						  (gpointer *)&s,
						  &err);
			if (q == Qnil)
				scommand = g_strdup("-%ENOMEM%-");
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
					       (gpointer *)&where,
					       &err);
		if (qwhere == Qnil)
			swhere = g_strdup("-%ENOMEM%-");
		else
			swhere = hg_string_get_cstr(where);
		hg_vm_mfree(vm, qwhere);

		if (qresult_err == Qnil) {
			g_printerr("Multiple errors occurred.\n"
				   "  previous error: unknown or this happened at %s prior to set /errorname\n"
				   "  current error: %s at %s\n",
				   scommand,
				   hg_name_lookup(vm->name, vm->qerror_name[error]),
				   swhere);
		} else {
			g_printerr("Multiple errors occurred.\n"
				   "  previous error: %s at %s\n"
				   "  current error: %s at %s\n",
				   hg_name_lookup(vm->name, qresult_err),
				   scommand,
				   hg_name_lookup(vm->name, vm->qerror_name[error]),
				   swhere);
		}
		g_free(swhere);
		g_free(scommand);

		hg_operator_invoke(HG_QOPER (HG_enc_private_abort), vm, &err);
	}
	qnerrordict = HG_QNAME (vm->name, "errordict");
	qerrordict = hg_vm_dict_lookup(vm, qnerrordict);
	if (qerrordict == Qnil) {
		g_set_error(&err, HG_ERROR, EINVAL,
			    "Unable to lookup errordict");
		goto fatal_error;
	}
	errordict = _HG_VM_LOCK (vm, qerrordict, &err);
	if (err)
		goto fatal_error;

	qhandler = hg_dict_lookup(errordict, vm->qerror_name[error], &err);
	_HG_VM_UNLOCK (vm, qerrordict);

	if (qhandler == Qnil) {
		g_set_error(&err, HG_ERROR, EINVAL,
			    "Unale to obtain the error handler for %s",
			    hg_name_lookup(vm->name, vm->qerror_name[error]));
		goto fatal_error;
	}

	q = hg_vm_quark_copy(vm, qhandler, NULL, &err);
	if (err)
		goto fatal_error;
	hg_stack_push(vm->stacks[HG_VM_STACK_ESTACK], q);
	hg_stack_push(vm->stacks[HG_VM_STACK_OSTACK], qdata);
	vm->has_error = TRUE;

	return TRUE;
  fatal_error:
	G_STMT_START {
		const gchar *errname = hg_name_lookup(vm->name, vm->qerror_name[error]);

		if (errname) {
			g_printerr("Fatal error during recovering from /%s: %s\n", errname, (err ? err->message : "no details"));
		} else {
			g_printerr("Fatal error during recovering from the unknown error: %s\n", err ? err->message : "no details");
		}
		if (err)
			g_clear_error(&err);

		return hg_operator_invoke(HG_QOPER (HG_enc_private_abort), vm, &err);
	} G_STMT_END;
}

/**
 * hg_vm_set_error_from_gerror:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_vm_set_error_from_gerror(hg_vm_t    *vm,
			    hg_quark_t  qdata,
			    GError     *error)
{
	if (error == NULL) {
		/* no error */
	} else {
		switch (error->code) {
		    case 0:
			    /* ??? */
			    break;
		    case EACCES:
		    case EBADF:
		    case EEXIST:
		    case ENOTDIR:
		    case ENOTEMPTY:
		    case EPERM:
		    case EROFS:
			    return hg_vm_set_error(vm, qdata, HG_VM_e_invalidaccess);
		    case EAGAIN:
		    case EBUSY:
		    case EIO:
		    case ENOSPC:
			    return hg_vm_set_error(vm, qdata, HG_VM_e_ioerror);
		    case EMFILE:
			    return hg_vm_set_error(vm, qdata, HG_VM_e_limitcheck);
		    case ENAMETOOLONG:
		    case ENODEV:
		    case ENOENT:
			    return hg_vm_set_error(vm, qdata, HG_VM_e_undefinedfilename);
		    case ENOMEM:
			    return hg_vm_set_error(vm, qdata, HG_VM_e_VMerror);
		    default:
			    g_warning("No matching error code for: %s (code: %d)",
				      error->message, error->code);
			    return hg_vm_set_error(vm, qdata, HG_VM_e_VMerror);
		}
	}

	return TRUE;
}

/**
 * hg_vm_stack_dump:
 * @stack:
 * @file:
 *
 * FIXME
 */
void
hg_vm_stack_dump(hg_vm_t    *vm,
		 hg_stack_t *stack,
		 hg_file_t  *output)
{
	hg_vm_stack_dump_data_t data;

	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (stack != NULL);
	hg_return_if_fail (output != NULL);

	/* to avoid unusable result when OOM */
	hg_mem_set_resizable(hg_vm_get_mem(vm), TRUE);

	data.vm = vm;
	data.stack = stack;
	data.ofile = output;

	hg_file_append_printf(output, "       value      |    type    |attr|content\n");
	hg_file_append_printf(output, "----------========+------------+----+---------------------------------\n");
	hg_stack_foreach(stack, _hg_vm_stack_real_dump, &data, NULL);
}
