/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * operator.c
 * Copyright (C) 2005-2006 Akira TAGOH
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "operator.h"
#include "version.h"
#include "hgmem.h"
#include "hgarray.h"
#include "hgdebug.h"
#include "hgdict.h"
#include "hgfile.h"
#include "hglineedit.h"
#include "hgmatrix.h"
#include "hgpath.h"
#include "hgstack.h"
#include "hgstring.h"
#include "hgvaluenode.h"
#include "hggraphics.h"
#include "scanner.h"
#include "vm.h"


struct _HieroGlyphOperator {
	HgObject        object;
	gchar          *name;
	HgOperatorFunc  operator;
};


static void     _hg_operator_real_set_flags(gpointer           data,
					    guint              flags);
static void     _hg_operator_real_relocate (gpointer           data,
					    HgMemRelocateInfo *info);
static gpointer _hg_operator_real_to_string(gpointer           data);


static HgObjectVTable __hg_operator_vtable = {
	.free      = NULL,
	.set_flags = _hg_operator_real_set_flags,
	.relocate  = _hg_operator_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = _hg_operator_real_to_string,
};
HgOperator *__hg_operator_list[HG_op_END];

/*
 * Operators
 */
#define DEFUNC_OP(func)					\
	static gboolean					\
	_hg_operator_op_##func(HgOperator *op,		\
			       gpointer    data)	\
	{						\
		HgVM *vm = data;			\
		gboolean retval = FALSE;
#define DEFUNC_OP_END				\
		return retval;			\
	}
#define DEFUNC_UNIMPLEMENTED_OP(func)			\
	static gboolean					\
	_hg_operator_op_##func(HgOperator *op,		\
			       gpointer    data)	\
	{						\
		g_warning("%s isn't yet implemented.", __PRETTY_FUNCTION__); \
							\
		return FALSE;				\
	}
#define _hg_operator_build_operator(vm, pool, sdict, name, func, ret_op) \
	hg_operator_build_operator__inline(_hg_operator_op_, vm, pool, sdict, name, func, ret_op)
#define BUILD_OP(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		_hg_operator_build_operator(vm, pool, sdict, name, func, __hg_op); \
		if (__hg_op != NULL) {					\
			__hg_operator_list[HG_op_##name] = __hg_op;	\
		}							\
	} G_STMT_END
#define BUILD_OP_(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		_hg_operator_build_operator(vm, pool, sdict, name, func, __hg_op); \
	} G_STMT_END

/* level 1 */
DEFUNC_OP (private_arraytomark)
G_STMT_START
{
	/* %arraytomark is the same as {counttomark array astore exch pop} */
	while (1) {
		retval = hg_operator_invoke(__hg_operator_list[HG_op_counttomark], vm);
		if (!retval)
			break;
		retval = hg_operator_invoke(__hg_operator_list[HG_op_array], vm);
		if (!retval)
			break;
		retval = hg_operator_invoke(__hg_operator_list[HG_op_astore], vm);
		if (!retval)
			break;
		retval = hg_operator_invoke(__hg_operator_list[HG_op_exch], vm);
		if (!retval)
			break;
		retval = hg_operator_invoke(__hg_operator_list[HG_op_pop], vm);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_dicttomark)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), i, j;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *node, *key, *val;
	HgDict *dict;

	while (1) {
		for (i = 0; i < depth; i++) {
			node = hg_stack_index(ostack, i);
			if (HG_IS_VALUE_MARK (node)) {
				if ((i % 2) != 0) {
					_hg_operator_set_error(vm, op, VM_e_rangecheck);
					break;
				}
				dict = hg_dict_new(pool, i / 2);
				if (dict == NULL) {
					_hg_operator_set_error(vm, op, VM_e_VMerror);
					break;
				}
				for (j = 1; j <= i; j += 2) {
					key = hg_stack_index(ostack, i - j);
					val = hg_stack_index(ostack, i - j - 1);
					if (!hg_mem_pool_is_own_object(pool, key) ||
					    !hg_mem_pool_is_own_object(pool, val)) {
						_hg_operator_set_error(vm, op, VM_e_invalidaccess);
						return retval;
					}
					if (!hg_dict_insert(pool, dict, key, val)) {
						_hg_operator_set_error(vm, op, VM_e_VMerror);
						return retval;
					}
				}
				for (j = 0; j <= i; j++)
					hg_stack_pop(ostack);
				HG_VALUE_MAKE_DICT (node, dict);
				if (node == NULL) {
					_hg_operator_set_error(vm, op, VM_e_VMerror);
					break;
				}
				retval = hg_stack_push(ostack, node);
				/* it must be true */
				break;
			}
		}
		if (i == depth) {
			_hg_operator_set_error(vm, op, VM_e_unmatchedmark);
			break;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_for_pos_int_continue)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nlimit, *ninc, *nini, *node;
	gint32 iini, iinc, ilimit;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);
		nlimit = hg_stack_index(estack, 2);
		ninc = hg_stack_index(estack, 3);
		nini = hg_stack_index(estack, 4);

		ilimit = HG_VALUE_GET_INTEGER (nlimit);
		iinc = HG_VALUE_GET_INTEGER (ninc);
		iini = HG_VALUE_GET_INTEGER (nini);
		if ((iinc > 0 && iini > ilimit) ||
		    (iinc < 0 && iini < ilimit)) {
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			retval = hg_stack_push(estack, nself);
			break;
		}
		HG_VALUE_SET_INTEGER (nini, iini + iinc);
		HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm), node, iini);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, node);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
			break;
		}
		node = hg_object_copy((HgObject *)nproc);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, nself); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_for_pos_real_continue)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nlimit, *ninc, *nini, *node;
	gdouble dini, dinc, dlimit;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);
		nlimit = hg_stack_index(estack, 2);
		ninc = hg_stack_index(estack, 3);
		nini = hg_stack_index(estack, 4);

		dlimit = HG_VALUE_GET_REAL (nlimit);
		dinc = HG_VALUE_GET_REAL (ninc);
		dini = HG_VALUE_GET_REAL (nini);
		if ((dinc > 0.0 && dini > dlimit) ||
		    (dinc < 0.0 && dini < dlimit)) {
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			retval = hg_stack_push(estack, nself);
			break;
		}
		HG_VALUE_SET_REAL (nini, dini + dinc);
		HG_VALUE_MAKE_REAL (hg_vm_get_current_pool(vm), node, dini);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, node);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
			break;
		}
		node = hg_object_copy((HgObject *)nproc);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, nself); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_forall_array_continue)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nval, *nn, *copy_proc, *node;
	HgArray *array;
	gint32 i;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);
		nval = hg_stack_index(estack, 2);
		nn = hg_stack_index(estack, 3);

		array = HG_VALUE_GET_ARRAY (nval);
		i = HG_VALUE_GET_INTEGER (nn);
		if (hg_array_length(array) <= i) {
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			retval = hg_stack_push(estack, nself); /* dummy */
			/* it must be true */
			break;
		}
		HG_VALUE_SET_INTEGER (nn, i + 1);
		node = hg_array_index(array, i);
		copy_proc = hg_object_copy((HgObject *)nproc);
		if (copy_proc == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, node);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
			break;
		}
		hg_stack_push(estack, copy_proc);
		retval = hg_stack_push(estack, nself); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_forall_dict_continue)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nval, *nn, *copy_proc, *key, *val;
	HgDict *dict;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);
		nval = hg_stack_index(estack, 2);
		nn = hg_stack_index(estack, 3);

		dict = HG_VALUE_GET_DICT (nval);
		if (!hg_dict_first(dict, &key, &val)) {
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			retval = hg_stack_push(estack, nself); /* dummy */
			/* it must be true */
			break;
		}
		hg_dict_remove(dict, key);
		copy_proc = hg_object_copy((HgObject *)nproc);
		if (copy_proc == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(ostack, key);
		retval = hg_stack_push(ostack, val);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
			break;
		}
		hg_stack_push(estack, copy_proc);
		retval = hg_stack_push(estack, nself); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_forall_string_continue)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nval, *nn, *copy_proc, *node;
	HgString *string;
	gint32 i;
	gchar c;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);
		nval = hg_stack_index(estack, 2);
		nn = hg_stack_index(estack, 3);

		string = HG_VALUE_GET_STRING (nval);
		i = HG_VALUE_GET_INTEGER (nn);
		if (hg_string_maxlength(string) <= i) {
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			retval = hg_stack_push(estack, nself); /* dummy */
			/* it must be true */
			break;
		}
		HG_VALUE_SET_INTEGER (nn, i + 1);
		c = hg_string_index(string, i);
		HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm), node, c);
		copy_proc = hg_object_copy((HgObject *)nproc);
		if (copy_proc == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, node);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
			break;
		}
		hg_stack_push(estack, copy_proc);
		retval = hg_stack_push(estack, nself); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_loop_continue)
G_STMT_START
{
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *node;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);

		node = hg_object_copy((HgObject *)nproc);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, nself); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_repeat_continue)
G_STMT_START
{
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nn, *node;
	gint32 n;

	while (1) {
		nself = hg_stack_index(estack, 0);
		nproc = hg_stack_index(estack, 1);
		nn = hg_stack_index(estack, 2);

		n = HG_VALUE_GET_INTEGER (nn);
		if (n > 0) {
			HG_VALUE_SET_INTEGER (nn, n - 1);
			node = hg_object_copy((HgObject *)nproc);
			if (node == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_push(estack, node);
			retval = hg_stack_push(estack, nself); /* dummy */
			if (!retval)
				_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			break;
		}
		hg_stack_pop(estack);
		hg_stack_pop(estack);
		hg_stack_pop(estack);
		retval = hg_stack_push(estack, nself);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_stopped_continue)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgDict *error = hg_vm_get_dict_error(vm);
	HgValueNode *node, *node2, *name;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	name = hg_vm_get_name_node(vm, ".isstop");
	node = hg_dict_lookup(error, name);
	if (node != NULL &&
	    HG_IS_VALUE_BOOLEAN (node) &&
	    HG_VALUE_GET_BOOLEAN (node) == TRUE) {
		HG_VALUE_MAKE_BOOLEAN (pool, node2, TRUE);
		HG_VALUE_SET_BOOLEAN (node, FALSE);
		hg_dict_insert(pool, error, name, node);
		hg_vm_clear_error(vm);
	} else {
		HG_VALUE_MAKE_BOOLEAN (pool, node2, FALSE);
	}
	retval = hg_stack_push(ostack, node2);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (private_findfont);
DEFUNC_UNIMPLEMENTED_OP (private_definefont);

DEFUNC_OP (private_stringcvs)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *node;
	HgString *hs;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		switch (HG_VALUE_GET_VALUE_TYPE (node)) {
		    case HG_TYPE_VALUE_BOOLEAN:
		    case HG_TYPE_VALUE_INTEGER:
		    case HG_TYPE_VALUE_REAL:
			    hs = hg_object_to_string((HgObject *)node);
			    break;
		    case HG_TYPE_VALUE_NAME:
			    hs = hg_string_new(pool, -1);
			    if (hs == NULL) {
				    _hg_operator_set_error(vm, op, VM_e_VMerror);
				    return retval;
			    }
			    hg_string_append(hs, HG_VALUE_GET_NAME (node), -1);
			    break;
		    case HG_TYPE_VALUE_STRING:
			    G_STMT_START {
				    if (hg_object_is_readable((HgObject *)node)) {
					    HgString *tmp = HG_VALUE_GET_STRING (node);

					    hs = hg_string_new(pool, -1);
					    hg_string_append(hs, hg_string_get_string(tmp),
							     hg_string_maxlength(tmp));
				    } else {
					    hs = hg_object_to_string((HgObject *)node);
				    }
			    } G_STMT_END;
			    break;
		    case HG_TYPE_VALUE_POINTER:
			    hs = hg_string_new(pool, -1);
			    if (hs == NULL) {
				    _hg_operator_set_error(vm, op, VM_e_VMerror);
				    return retval;
			    }
			    hg_string_append(hs,
					     hg_operator_get_name(HG_VALUE_GET_POINTER (node)),
					     -1);
			    break;
		    default:
			    hs = hg_string_new(pool, 16);
			    if (hs == NULL) {
				    _hg_operator_set_error(vm, op, VM_e_VMerror);
				    return retval;
			    }
			    hg_string_append(hs, "--nostringval--", -1);
			    break;
		}
		if (hs == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_string_fix_string_size(hs);
		HG_VALUE_MAKE_STRING (node, hs);
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

DEFUNC_UNIMPLEMENTED_OP (private_undefinefont);

DEFUNC_OP (private_write_eqeq_only)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *hs;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		hs = hg_object_to_string((HgObject *)n2);
		if (hs == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		} else {
			hg_file_object_write(file,
					     hg_string_get_string(hs),
					     sizeof (gchar),
					     hg_string_maxlength(hs));
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (abs)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n, *node;
	guint depth = hg_stack_depth(ostack);
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			HG_VALUE_MAKE_INTEGER (pool, node, abs(HG_VALUE_GET_INTEGER (n)));
		} else if (HG_IS_VALUE_REAL (n)) {
			HG_VALUE_MAKE_REAL (pool, node, fabs(HG_VALUE_GET_REAL (n)));
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
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

DEFUNC_OP (add)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	guint depth = hg_stack_depth(ostack);
	gboolean integer = TRUE;
	gdouble d1, d2;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
			integer = FALSE;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
			integer = FALSE;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (integer) {
			HG_VALUE_MAKE_INTEGER (pool, n1, (gint32)(d1 + d2));
		} else {
			HG_VALUE_MAKE_REAL (pool, n1, d1 + d2);
		}
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (aload)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), len, i;
	HgValueNode *narray;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		narray = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (narray)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)narray)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		hg_stack_pop(ostack);
		array = HG_VALUE_GET_ARRAY (narray);
		len = hg_array_length(array);
		for (i = 0; i < len; i++) {
			hg_stack_push(ostack, hg_array_index(array, i));
		}
		retval = hg_stack_push(ostack, narray);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (anchorsearch)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), length;
	HgValueNode *n1, *n2, *result;
	HgString *s, *seek, *post, *match;
	HgMemObject *obj;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_STRING (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		s = HG_VALUE_GET_STRING (n1);
		seek = HG_VALUE_GET_STRING (n2);
		length = hg_string_length(seek);
		if (hg_string_ncompare(s, seek, length)) {
			hg_mem_get_object__inline(s, obj);
			match = hg_string_make_substring(obj->pool, s, 0, length - 1);
			HG_VALUE_MAKE_STRING (n2, match);
			post = hg_string_make_substring(obj->pool, s, length, hg_string_length(s) - 1);
			HG_VALUE_MAKE_STRING (n1, post);
			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			HG_VALUE_MAKE_BOOLEAN (pool, result, TRUE);
			hg_stack_push(ostack, n1);
			hg_stack_push(ostack, n2);
			retval = hg_stack_push(ostack, result);
			if (!retval)
				_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		} else {
			hg_stack_pop(ostack);
			HG_VALUE_MAKE_BOOLEAN (pool, result, FALSE);
			retval = hg_stack_push(ostack, result);
			/* it must be true */
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (and)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_BOOLEAN (n1) &&
		    HG_IS_VALUE_BOOLEAN (n2)) {
			gboolean result = HG_VALUE_GET_BOOLEAN (n1) & HG_VALUE_GET_BOOLEAN (n2);

			HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
		} else if (HG_IS_VALUE_INTEGER (n1) &&
			   HG_IS_VALUE_INTEGER (n2)) {
			gint32 result = HG_VALUE_GET_INTEGER (n1) & HG_VALUE_GET_INTEGER (n2);

			HG_VALUE_MAKE_INTEGER (pool, n1, result);
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (arc)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx, *ny, *nr, *nangle1, *nangle2;
	gdouble dx, dy, dr, dangle1, dangle2;

	while (1) {
		if (depth < 5) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		nangle2 = hg_stack_index(ostack, 0);
		nangle1 = hg_stack_index(ostack, 1);
		nr = hg_stack_index(ostack, 2);
		ny = hg_stack_index(ostack, 3);
		nx = hg_stack_index(ostack, 4);
		if (HG_IS_VALUE_INTEGER (nx)) {
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		} else if (HG_IS_VALUE_REAL (nx)) {
			dx = HG_VALUE_GET_REAL (nx);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny)) {
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		} else if (HG_IS_VALUE_REAL (ny)) {
			dy = HG_VALUE_GET_REAL (ny);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nr)) {
			dr = HG_VALUE_GET_REAL_FROM_INTEGER (nr);
		} else if (HG_IS_VALUE_REAL (nr)) {
			dr = HG_VALUE_GET_REAL (nr);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nangle1)) {
			dangle1 = HG_VALUE_GET_REAL_FROM_INTEGER (nangle1);
		} else if (HG_IS_VALUE_REAL (nangle1)) {
			dangle1 = HG_VALUE_GET_REAL (nangle1);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nangle2)) {
			dangle2 = HG_VALUE_GET_REAL_FROM_INTEGER (nangle2);
		} else if (HG_IS_VALUE_REAL (nangle2)) {
			dangle2 = HG_VALUE_GET_REAL (nangle2);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		retval = hg_graphic_state_path_arc(hg_graphics_get_state(hg_vm_get_graphics(vm)),
						   dx, dy, dr,
						   dangle1 * (2 * M_PI / 360),
						   dangle2 * (2 * M_PI / 360));
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (arcn);
DEFUNC_UNIMPLEMENTED_OP (arcto);

DEFUNC_OP (array)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *n;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 size, i;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		size = HG_VALUE_GET_INTEGER (node);
		if (size < 0 || size > 65535) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		array = hg_array_new(pool, size);
		if (array == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		n = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm), "null");
		if (n == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			return FALSE;
		}
		for (i = 0; i < size; i++) {
			hg_array_append(array, n);
		}
		HG_VALUE_MAKE_ARRAY (node, array);
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

DEFUNC_UNIMPLEMENTED_OP (ashow);

DEFUNC_OP (astore)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), len, i;
	HgValueNode *node, *narray;
	HgArray *array;
	HgMemObject *obj;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		narray = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (narray)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)narray)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (narray);
		len = hg_array_length(array);
		if (depth < (len + 1)) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		hg_mem_get_object__inline(array, obj);
		for (i = 0; i < len; i++) {
			node = hg_stack_index(ostack, len - i);
			if (!hg_mem_pool_is_own_object(obj->pool, node)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				return FALSE;
			}
			hg_array_replace(array, node, i);
		}
		for (i = 0; i <= len; i++)
			hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, narray);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (atan)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2, *node;
	gdouble d1, d2, result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1)) {
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		} else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2)) {
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		} else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_VALUE_REAL_SIMILAR (d1, 0) && HG_VALUE_REAL_SIMILAR (d2, 0)) {
			_hg_operator_set_error(vm, op, VM_e_undefinedresult);
			break;
		}
		result = atan2(d1, d2) / (2 * M_PI / 360);
		if (result < 0)
			result = 360.0 + result;
		HG_VALUE_MAKE_REAL (hg_vm_get_current_pool(vm),
				    node, result);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (awidthshow);

DEFUNC_OP (begin)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgDict *dict;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_DICT (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (node);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		retval = hg_stack_push(dstack, node);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_dictstackoverflow);
			break;
		}
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (bind)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint length = hg_stack_depth(ostack), array_len, i;
	HgValueNode *n, *node;
	HgArray *array;

	while (1) {
		if (length < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (n)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n)) {
			/* just ignore it */
			retval = TRUE;
			break;
		}
		array = HG_VALUE_GET_ARRAY (n);
		array_len = hg_array_length(array);
		for (i = 0; i < array_len; i++) {
			n = hg_array_index(array, i);
			if (hg_object_is_executable((HgObject *)n)) {
				if (HG_IS_VALUE_ARRAY (n)) {
					if (!hg_stack_push(ostack, n)) {
						_hg_operator_set_error(vm, op, VM_e_stackoverflow);
					} else {
						_hg_operator_op_bind(op, vm);
						hg_stack_pop(ostack);
					}
				} else if (HG_IS_VALUE_NAME (n)) {
					node = hg_vm_lookup(vm, n);
					if (node != NULL && HG_IS_VALUE_POINTER (node)) {
						hg_array_replace(array, node, i);
					}
				}
			}
		}
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (bitshift)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2, *result;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 i1, i2;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n1) ||
		    !HG_IS_VALUE_INTEGER (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		i1 = HG_VALUE_GET_INTEGER (n1);
		i2 = HG_VALUE_GET_INTEGER (n2);
		if (i2 < 0) {
			HG_VALUE_MAKE_INTEGER (pool, result, i1 >> abs(i2));
		} else {
			HG_VALUE_MAKE_INTEGER (pool, result, i1 << i2);
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, result);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (bytesavailable)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *n;
	HgFileObject *file;
	gint32 result;
	gssize cur_pos;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_FILE (n)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n);
		if (!hg_file_object_is_readable(file)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		cur_pos = hg_file_object_seek(file, 0, HG_FILE_POS_CURRENT);
		result = hg_file_object_seek(file, 0, HG_FILE_POS_END);
		hg_file_object_seek(file, cur_pos, HG_FILE_POS_BEGIN);
		HG_VALUE_MAKE_INTEGER (pool, n, result);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (cachestatus);

DEFUNC_OP (ceiling)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *n;
	gdouble result;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			/* nothing to do */
			retval = TRUE;
		} else if (HG_IS_VALUE_REAL (n)) {
			result = ceil(HG_VALUE_GET_REAL (n));
			HG_VALUE_MAKE_REAL (pool, n, result);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, n);
			/* it must be true */
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (charpath);

DEFUNC_OP (clear)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);

	hg_stack_clear(ostack);
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cleardictstack)
G_STMT_START
{
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint depth = hg_stack_depth(dstack);
	gint i = 0;
	HgVMEmulationType type = hg_vm_get_emulation_level(vm);

	switch (type) {
	    case VM_EMULATION_LEVEL_1:
		    i = depth - 2;
		    break;
	    case VM_EMULATION_LEVEL_2:
		    i = depth - 3;
		    break;
	    default:
		    g_warning("Unknown emulation level %d\n", type);
		    break;
	}
	if (i < 0) {
		_hg_operator_set_error(vm, op, VM_e_VMerror);
	} else {
		for (; i > 0; i--)
			hg_stack_pop(dstack);
		retval = TRUE;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cleartomark)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), i, j;
	HgValueNode *node;

	for (i = 0; i < depth; i++) {
		node = hg_stack_index(ostack, i);
		if (HG_IS_VALUE_MARK (node)) {
			for (j = 0; j <= i; j++) {
				hg_stack_pop(ostack);
			}
			retval = TRUE;
			break;
		}
	}
	if (i == depth)
		_hg_operator_set_error(vm, op, VM_e_unmatchedmark);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (clip);

DEFUNC_OP (clippath)
G_STMT_START
{
	retval = hg_graphic_state_path_from_clip(hg_graphics_get_state(hg_vm_get_graphics(vm)));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (closefile)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n;
	HgFileObject *file;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_FILE (n)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n);
		if (hg_file_object_is_writable(file)) {
			if (!hg_object_is_writable((HgObject *)file)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			hg_file_object_flush(file);
		}
		hg_file_object_close(file);
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (closepath)
G_STMT_START
{
	retval = hg_graphic_state_path_close(hg_graphics_get_state(hg_vm_get_graphics(vm)));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (concat)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), i;
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgArray *matrix;
	gdouble dmatrix[6];
	HgMatrix *mtx, *new_ctm;
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		matrix = HG_VALUE_GET_ARRAY (node);
		if (hg_array_length(matrix) != 6) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		for (i = 0; i < 6; i++) {
			node = hg_array_index(matrix, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL (node);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
		}
		if (i == 6) {
			mtx = hg_matrix_new(pool,
					    dmatrix[0], dmatrix[1], dmatrix[2],
					    dmatrix[3], dmatrix[4], dmatrix[5]);
			if (mtx == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			new_ctm = hg_matrix_multiply(pool, mtx, &gstate->ctm);
			if (new_ctm == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			memcpy(&gstate->ctm, new_ctm, sizeof (HgMatrix));
			hg_mem_free(mtx);
			hg_mem_free(new_ctm);
			retval = hg_path_matrix(gstate->path,
						gstate->ctm.xx, gstate->ctm.yx,
						gstate->ctm.xy, gstate->ctm.yy,
						gstate->ctm.x0, gstate->ctm.y0);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (concatmatrix)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), i;
	HgValueNode *n1, *n2, *n3, *node;
	HgArray *m1, *m2, *m3;
	gdouble d1[6], d2[6];
	HgMatrix *mtx1, *mtx2, *mtx3;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n3 = hg_stack_index(ostack, 0);
		n2 = hg_stack_index(ostack, 1);
		n1 = hg_stack_index(ostack, 2);
		if (!HG_IS_VALUE_ARRAY (n1) ||
		    !HG_IS_VALUE_ARRAY (n2) ||
		    !HG_IS_VALUE_ARRAY (n3)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2) ||
		    !hg_object_is_writable((HgObject *)n3)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		m1 = HG_VALUE_GET_ARRAY (n1);
		m2 = HG_VALUE_GET_ARRAY (n2);
		m3 = HG_VALUE_GET_ARRAY (n3);
		if (hg_array_length(m1) != 6 ||
		    hg_array_length(m2) != 6 ||
		    hg_array_length(m3) != 6) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		for (i = 0; i < 6; i++) {
			node = hg_array_index(m1, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				d1[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				d1[i] = HG_VALUE_GET_REAL (node);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			node = hg_array_index(m2, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				d2[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				d2[i] = HG_VALUE_GET_REAL (node);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
		}
		mtx1 = hg_matrix_new(pool, d1[0], d1[1], d1[2], d1[3], d1[4], d1[5]);
		mtx2 = hg_matrix_new(pool, d2[0], d2[1], d2[2], d2[3], d2[4], d2[5]);
		if (mtx1 == NULL || mtx2 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		mtx3 = hg_matrix_multiply(pool, mtx1, mtx2);
		node = hg_array_index(m3, 0);
		HG_VALUE_SET_REAL (node, mtx3->xx);
		node = hg_array_index(m3, 1);
		HG_VALUE_SET_REAL (node, mtx3->yx);
		node = hg_array_index(m3, 2);
		HG_VALUE_SET_REAL (node, mtx3->xy);
		node = hg_array_index(m3, 3);
		HG_VALUE_SET_REAL (node, mtx3->yy);
		node = hg_array_index(m3, 4);
		HG_VALUE_SET_REAL (node, mtx3->x0);
		node = hg_array_index(m3, 5);
		HG_VALUE_SET_REAL (node, mtx3->y0);
		hg_mem_free(mtx1);
		hg_mem_free(mtx2);
		hg_mem_free(mtx3);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n3);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

static gboolean
_hg_operator_copy__traverse_dict(gpointer key,
				 gpointer val,
				 gpointer data)
{
	HgValueNode *nkey = key, *nval = val, *nnkey, *nnval;
	HgDict *dict = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(key, obj);
	nnkey = hg_object_dup((HgObject *)nkey);
	nnval = hg_object_dup((HgObject *)nval);
	hg_dict_insert(obj->pool, dict, nnkey, nnval);

	return TRUE;
}

DEFUNC_OP (copy)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *n2, *dup_node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			/* stack copy */
			guint32 i, n = HG_VALUE_GET_INTEGER (node);

			if (n < 0 || n >= depth) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			hg_stack_pop(ostack);
			for (i = 0; i < n; i++) {
				node = hg_stack_index(ostack, n - 1);
				dup_node = hg_object_dup((HgObject *)node);
				retval = hg_stack_push(ostack, dup_node);
				if (!retval) {
					_hg_operator_set_error(vm, op, VM_e_stackoverflow);
					break;
				}
			}
			if (n == 0)
				retval = TRUE;
		} else {
			if (depth < 2) {
				_hg_operator_set_error(vm, op, VM_e_stackunderflow);
				break;
			}
			n2 = hg_stack_index(ostack, 1);
			if (!hg_object_is_readable((HgObject *)node) ||
			    !hg_object_is_writable((HgObject *)n2)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			if (HG_IS_VALUE_ARRAY (node) &&
			    HG_IS_VALUE_ARRAY (n2)) {
				HgArray *a1, *a2;
				guint len1, len2, i;

				a2 = HG_VALUE_GET_ARRAY (node);
				a1 = HG_VALUE_GET_ARRAY (n2);
				len1 = hg_array_length(a1);
				len2 = hg_array_length(a2);
				if (len1 > len2) {
					_hg_operator_set_error(vm, op, VM_e_rangecheck);
					break;
				}
				for (i = 0; i < len1; i++) {
					node = hg_array_index(a1, i);
					dup_node = hg_object_dup((HgObject *)node);
					if (dup_node == NULL) {
						_hg_operator_set_error(vm, op, VM_e_VMerror);
						return retval;
					}
					hg_array_replace(a2, dup_node, i);
				}
				if (len2 > len1) {
					HgMemObject *obj;

					hg_mem_get_object__inline(a2, obj);
					/* need to make a subarray for that */
					a2 = hg_array_make_subarray(obj->pool, a2, 0, len1 - 1);
				}
				HG_VALUE_MAKE_ARRAY (dup_node, a2);
			} else if (HG_IS_VALUE_DICT (node) &&
				   HG_IS_VALUE_DICT (n2)) {
				HgDict *d1, *d2;
				HgVMEmulationType type = hg_vm_get_emulation_level(vm);

				d2 = HG_VALUE_GET_DICT (node);
				d1 = HG_VALUE_GET_DICT (n2);
				if (!hg_object_is_readable((HgObject *)d1) ||
				    !hg_object_is_writable((HgObject *)d2)) {
					_hg_operator_set_error(vm, op, VM_e_invalidaccess);
					break;
				}
				if (type == VM_EMULATION_LEVEL_1) {
					if (hg_dict_length(d2) != 0 ||
					    hg_dict_maxlength(d1) != hg_dict_maxlength(d2)) {
						_hg_operator_set_error(vm, op, VM_e_rangecheck);
						break;
					}
				}
				hg_dict_traverse(d1, _hg_operator_copy__traverse_dict, d2);
				HG_VALUE_MAKE_DICT (dup_node, d2);
			} else if (HG_IS_VALUE_STRING (node) &&
				   HG_IS_VALUE_STRING (n2)) {
				HgString *s1, *s2;
				guint len1, mlen1, mlen2, i;
				gchar c;

				s2 = HG_VALUE_GET_STRING (node);
				s1 = HG_VALUE_GET_STRING (n2);
				len1 = hg_string_length(s1);
				mlen1 = hg_string_maxlength(s1);
				mlen2 = hg_string_maxlength(s2);
				if (mlen1 > mlen2) {
					_hg_operator_set_error(vm, op, VM_e_rangecheck);
					break;
				}
				for (i = 0; i < len1; i++) {
					c = hg_string_index(s1, i);
					hg_string_insert_c(s2, c, i);
				}
				if (mlen2 > mlen1) {
					HgMemObject *obj;

					hg_mem_get_object__inline(s2, obj);
					/* need to make a substring for that */
					s2 = hg_string_make_substring(obj->pool, s2, 0, len1 - 1);
				}
				HG_VALUE_MAKE_STRING (dup_node, s2);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, dup_node);
			/* it must be true */
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (copypage);

DEFUNC_OP (cos)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (hg_vm_get_current_pool(vm),
				    node,
				    cos(d * (2 * M_PI / 360)));
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

DEFUNC_OP (count)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm), node, depth);
	if (node == NULL) {
		_hg_operator_set_error(vm, op, VM_e_VMerror);
	} else {
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (countdictstack)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint ddepth = hg_stack_depth(dstack);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm), node, ddepth);
	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (countexecstack)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint edepth = hg_stack_depth(estack);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm), node, edepth);
	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (counttomark)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node = NULL;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 i;

	for (i = 0; i < depth; i++) {
		node = hg_stack_index(ostack, i);
		if (HG_IS_VALUE_MARK (node)) {
			HG_VALUE_MAKE_INTEGER (pool, node, i);
			break;
		}
	}
	if (i == depth || node == NULL) {
		_hg_operator_set_error(vm, op, VM_e_unmatchedmark);
	} else {
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (currentdash);

DEFUNC_OP (currentdict)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	HgValueNode *node, *dup_node;

	node = hg_stack_index(dstack, 0);
	dup_node = hg_object_dup((HgObject *)node);
	retval = hg_stack_push(ostack, dup_node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (currentfile)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint edepth = hg_stack_depth(estack), i;
	HgValueNode *n;
	HgFileObject *file = NULL;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	for (i = 0; i < edepth; i++) {
		n = hg_stack_index(estack, i);
		if (HG_IS_VALUE_FILE (n)) {
			file = HG_VALUE_GET_FILE (n);
			break;
		}
	}
	if (file == NULL) {
		/* make an invalid file object */
		file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER, HG_FILE_MODE_READ, "%invalid", "", 0);
	}
	HG_VALUE_MAKE_FILE (n, file);
	retval = hg_stack_push(ostack, n);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (currentflat);
DEFUNC_UNIMPLEMENTED_OP (currentfont);
DEFUNC_UNIMPLEMENTED_OP (currentgray);
DEFUNC_UNIMPLEMENTED_OP (currenthsbcolor);
DEFUNC_UNIMPLEMENTED_OP (currentlinecap);
DEFUNC_UNIMPLEMENTED_OP (currentlinejoin);
DEFUNC_UNIMPLEMENTED_OP (currentlinewidth);

DEFUNC_OP (currentmatrix)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgArray *array;
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node) ||
		    !hg_object_is_writable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (node);
		if (hg_array_length(array) != 6) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		node = hg_array_index(array, 0);
		HG_VALUE_SET_REAL (node, gstate->ctm.xx);
		node = hg_array_index(array, 1);
		HG_VALUE_SET_REAL (node, gstate->ctm.yx);
		node = hg_array_index(array, 2);
		HG_VALUE_SET_REAL (node, gstate->ctm.xy);
		node = hg_array_index(array, 3);
		HG_VALUE_SET_REAL (node, gstate->ctm.yy);
		node = hg_array_index(array, 4);
		HG_VALUE_SET_REAL (node, gstate->ctm.x0);
		node = hg_array_index(array, 5);
		HG_VALUE_SET_REAL (node, gstate->ctm.y0);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (currentmiterlimit);

DEFUNC_OP (currentpoint)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *node;
	gdouble dx, dy;

	while (1) {
		if (!hg_path_compute_current_point(gstate->path, &dx, &dy)) {
			_hg_operator_set_error(vm, op, VM_e_nocurrentpoint);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, node, dx);
		hg_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, dy);
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (currentrgbcolor);
DEFUNC_UNIMPLEMENTED_OP (currentscreen);
DEFUNC_UNIMPLEMENTED_OP (currenttransfer);

DEFUNC_OP (curveto)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx1, *ny1, *nx2, *ny2, *nx3, *ny3;
	gdouble dx1, dy1, dx2, dy2, dx3, dy3, dx, dy;
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));

	while (1) {
		if (depth < 6) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		ny3 = hg_stack_index(ostack, 0);
		nx3 = hg_stack_index(ostack, 1);
		ny2 = hg_stack_index(ostack, 2);
		nx2 = hg_stack_index(ostack, 3);
		ny1 = hg_stack_index(ostack, 4);
		nx1 = hg_stack_index(ostack, 5);
		if (HG_IS_VALUE_INTEGER (nx1)) {
			dx1 = HG_VALUE_GET_REAL_FROM_INTEGER (nx1);
		} else if (HG_IS_VALUE_REAL (nx1)) {
			dx1 = HG_VALUE_GET_REAL (nx1);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny1)) {
			dy1 = HG_VALUE_GET_REAL_FROM_INTEGER (ny1);
		} else if (HG_IS_VALUE_REAL (ny1)) {
			dy1 = HG_VALUE_GET_REAL (ny1);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nx2)) {
			dx2 = HG_VALUE_GET_REAL_FROM_INTEGER (nx2);
		} else if (HG_IS_VALUE_REAL (nx2)) {
			dx2 = HG_VALUE_GET_REAL (nx2);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny2)) {
			dy2 = HG_VALUE_GET_REAL_FROM_INTEGER (ny2);
		} else if (HG_IS_VALUE_REAL (ny2)) {
			dy2 = HG_VALUE_GET_REAL (ny2);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nx3)) {
			dx3 = HG_VALUE_GET_REAL_FROM_INTEGER (nx3);
		} else if (HG_IS_VALUE_REAL (nx3)) {
			dx3 = HG_VALUE_GET_REAL (nx3);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny3)) {
			dy3 = HG_VALUE_GET_REAL_FROM_INTEGER (ny3);
		} else if (HG_IS_VALUE_REAL (ny3)) {
			dy3 = HG_VALUE_GET_REAL (ny3);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_path_compute_current_point(gstate->path, &dx, &dy)) {
			_hg_operator_set_error(vm, op, VM_e_nocurrentpoint);
			break;
		}
		retval = hg_graphic_state_path_curveto(gstate,
						       dx1, dy1,
						       dx2, dy2,
						       dx3, dy3);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}

		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvi)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			/* nothing to do here */
		} else if (HG_IS_VALUE_REAL (node)) {
			gdouble d = HG_VALUE_GET_REAL (node);

			if (d >= G_MAXINT) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			HG_VALUE_MAKE_INTEGER (pool, node, (gint32)d);
		} else if (HG_IS_VALUE_STRING (node)) {
			HgString *str;
			HgValueNode *n;
			HgFileObject *file;

			if (!hg_object_is_readable((HgObject *)node)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			str = HG_VALUE_GET_STRING (node);
			file = hg_file_object_new(pool,
						  HG_FILE_TYPE_BUFFER,
						  HG_FILE_MODE_READ,
						  "--%cvi--",
						  hg_string_get_string(str),
						  hg_string_maxlength(str));
			n = hg_scanner_get_object(vm, file);
			if (n == NULL) {
				if (!hg_vm_has_error(vm))
					_hg_operator_set_error(vm, op, VM_e_syntaxerror);
				break;
			} else if (HG_IS_VALUE_INTEGER (n)) {
				HG_VALUE_MAKE_INTEGER (pool, node, HG_VALUE_GET_INTEGER (n));
			} else if (HG_IS_VALUE_REAL (n)) {
				gdouble d = HG_VALUE_GET_REAL (n);

				if (d >= G_MAXINT) {
					_hg_operator_set_error(vm, op, VM_e_rangecheck);
					break;
				}
				HG_VALUE_MAKE_INTEGER (pool, node, (gint32)d);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			hg_mem_free(n);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
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

DEFUNC_OP (cvlit)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		hg_object_inexecutable((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvn)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgString *hs;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n1 = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (n1)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		hs = HG_VALUE_GET_STRING (n1);
		HG_VALUE_MAKE_NAME (n2, hg_string_get_string(hs));
		if (n2 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		if (hg_object_is_executable((HgObject *)n1))
			hg_object_executable((HgObject *)n2);
		if (hg_object_is_executeonly((HgObject *)n1))
			hg_object_executeonly((HgObject *)n2);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n2);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvr)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_REAL (node)) {
			/* nothing to do here */
		} else if (HG_IS_VALUE_INTEGER (node)) {
			gint32 i = HG_VALUE_GET_INTEGER (node);

			HG_VALUE_MAKE_REAL (pool, node, (gdouble)i);
		} else if (HG_IS_VALUE_STRING (node)) {
			HgString *str;
			HgValueNode *n;
			HgFileObject *file;

			if (!hg_object_is_readable((HgObject *)node)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			str = HG_VALUE_GET_STRING (node);
			file = hg_file_object_new(pool,
						  HG_FILE_TYPE_BUFFER,
						  HG_FILE_MODE_READ,
						  "--%cvr--",
						  hg_string_get_string(str),
						  hg_string_maxlength(str));
			n = hg_scanner_get_object(vm, file);
			if (n == NULL) {
				if (!hg_vm_has_error(vm))
					_hg_operator_set_error(vm, op, VM_e_syntaxerror);
				break;
			} else if (HG_IS_VALUE_REAL (n)) {
				HG_VALUE_MAKE_REAL (pool, node, HG_VALUE_GET_REAL (n));
			} else if (HG_IS_VALUE_INTEGER (n)) {
				gint32 i = HG_VALUE_GET_INTEGER (n);

				HG_VALUE_MAKE_REAL (pool, node, (gdouble)i);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			hg_mem_free(n);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
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

DEFUNC_OP (cvrs)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2, *n3;
	gint32 radix;
	HgString *s, *sresult;
	gboolean is_real = FALSE;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n3 = hg_stack_index(ostack, 0);
		n2 = hg_stack_index(ostack, 1);
		n1 = hg_stack_index(ostack, 2);
		if (HG_IS_VALUE_REAL (n1)) {
			is_real = TRUE;
		} else if (!HG_IS_VALUE_INTEGER (n1)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!HG_IS_VALUE_INTEGER (n2) ||
		    !HG_IS_VALUE_STRING (n3)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		radix = HG_VALUE_GET_INTEGER (n2);
		if (radix < 2 || radix > 36) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n3)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		s = HG_VALUE_GET_STRING (n3);
		if (radix == 10 && is_real) {
			HgString *stmp = hg_object_to_string((HgObject *)n1);
			gchar *str;

			if (stmp == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			if (hg_string_length(stmp) > hg_string_maxlength(s)) {
				hg_mem_free(stmp);
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			str = hg_string_get_string(stmp);
			hg_string_clear(s);
			hg_string_append(s, str, hg_string_length(stmp));
			hg_mem_free(stmp);
		} else if (is_real) {
			if (!hg_string_convert_from_integer(s,
							    HG_VALUE_GET_INTEGER_FROM_REAL (n1),
							    radix, FALSE)) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
		} else {
			if (!hg_string_convert_from_integer(s,
							    HG_VALUE_GET_INTEGER (n1),
							    radix, FALSE)) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
		}
		if (hg_string_maxlength(s) > hg_string_length(s)) {
			HgMemObject *obj;

			hg_mem_get_object__inline(s, obj);
			sresult = hg_string_make_substring(obj->pool, s, 0, hg_string_length(s) - 1);
		} else {
			sresult = s;
		}
		HG_VALUE_MAKE_STRING (n1, sresult);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvx)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		hg_object_executable((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (def)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint odepth = hg_stack_depth(ostack);
	HgValueNode *nd, *nk, *nv;
	HgDict *dict;
	HgMemObject *obj;

	while (1) {
		if (odepth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nv = hg_stack_index(ostack, 0);
		nk = hg_stack_index(ostack, 1);
		nd = hg_stack_index(dstack, 0);
		dict = HG_VALUE_GET_DICT (nd);
		hg_mem_get_object__inline(dict, obj);
		if (!hg_object_is_writable((HgObject *)dict) ||
		    !hg_mem_pool_is_own_object(obj->pool, nk) ||
		    !hg_mem_pool_is_own_object(obj->pool, nv)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		retval = hg_dict_insert(hg_vm_get_current_pool(vm),
					dict, nk, nv);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (defaultmatrix);

DEFUNC_OP (dict)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgDict *dict;
	gint32 n_dict;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		n_dict = HG_VALUE_GET_INTEGER (node);
		if (n_dict < 0) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		if (n_dict > 65535) {
			_hg_operator_set_error(vm, op, VM_e_limitcheck);
			break;
		}
		dict = hg_dict_new(pool, n_dict);
		if (dict == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_DICT (node, dict);
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

DEFUNC_OP (dictstack)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint depth = hg_stack_depth(ostack);
	guint ddepth = hg_stack_depth(dstack);
	guint len, i;
	HgValueNode *node, *dup_node;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (node);
		len = hg_array_length(array);
		if (ddepth > len) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		for (i = 0; i < ddepth; i++) {
			node = hg_stack_index(dstack, ddepth - i - 1);
			dup_node = hg_object_dup((HgObject *)node);
			hg_array_replace(array, dup_node, i);
		}
		if (ddepth != len) {
			HgMemObject *obj;

			hg_mem_get_object__inline(array, obj);
			if (obj == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			array = hg_array_make_subarray(obj->pool, array, 0, ddepth - 1);
			HG_VALUE_MAKE_ARRAY (node, array);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, node);
			/* it must be true */
		} else {
			retval = TRUE;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (div)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gdouble d1, d2;
	guint depth = hg_stack_depth(ostack);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_VALUE_REAL_SIMILAR(d2, 0)) {
			_hg_operator_set_error(vm, op, VM_e_undefinedresult);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, n1, d1 / d2);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (dtransform);

DEFUNC_OP (dup)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *dup_node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		dup_node = hg_object_dup((HgObject *)node);
		retval = hg_stack_push(ostack, dup_node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (echo);
DEFUNC_UNIMPLEMENTED_OP (eexec);

DEFUNC_OP (end)
G_STMT_START
{
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint depth = hg_stack_depth(dstack);
	HgVMEmulationType type = hg_vm_get_emulation_level(vm);

	while (1) {
		if ((type >= VM_EMULATION_LEVEL_2 && depth <= 3) ||
		    (type == VM_EMULATION_LEVEL_1 && depth <= 2)) {
			_hg_operator_set_error(vm, op, VM_e_dictstackunderflow);
			break;
		}
		hg_stack_pop(dstack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (eoclip);

DEFUNC_OP (eofill)
G_STMT_START
{
	retval = hg_graphics_render_eofill(hg_vm_get_graphics(vm));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (eq)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		result = hg_value_node_compare(n1, n2);
		if (!result) {
			if ((HG_IS_VALUE_NAME (n1) || HG_IS_VALUE_STRING (n1)) &&
			    (HG_IS_VALUE_NAME (n2) || HG_IS_VALUE_STRING (n2))) {
				gchar *str1, *str2;

				if (HG_IS_VALUE_NAME (n1))
					str1 = HG_VALUE_GET_NAME (n1);
				else
					str1 = hg_string_get_string(HG_VALUE_GET_STRING (n1));
				if (HG_IS_VALUE_NAME (n2))
					str2 = HG_VALUE_GET_NAME (n2);
				else
					str2 = hg_string_get_string(HG_VALUE_GET_STRING (n2));
				if (str1 != NULL && str2 != NULL)
					result = (strcmp(str1, str2) == 0);
			} else if ((HG_IS_VALUE_INTEGER (n1) || HG_IS_VALUE_REAL (n1)) &&
				   (HG_IS_VALUE_INTEGER (n2) || HG_IS_VALUE_REAL (n2))) {
				gdouble d1;

				if (HG_IS_VALUE_INTEGER (n1))
					d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
				else
					d1 = HG_VALUE_GET_REAL (n1);
				if (HG_IS_VALUE_INTEGER (n2))
					result = HG_VALUE_REAL_SIMILAR (d1, HG_VALUE_GET_REAL_FROM_INTEGER (n2));
				else
					result = HG_VALUE_REAL_SIMILAR (d1, HG_VALUE_GET_REAL (n2));
			}
		}
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (erasepage);

DEFUNC_OP (exch)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_pop(ostack);
		n1 = hg_stack_pop(ostack);
		hg_stack_push(ostack, n2);
		hg_stack_push(ostack, n1);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (exec)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *self, *tmp;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (hg_object_is_executable((HgObject *)node)) {
			if (!hg_object_is_executeonly((HgObject *)node) &&
			    !hg_object_is_readable((HgObject *)node)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			self = hg_stack_pop(estack);
			tmp = hg_object_copy((HgObject *)node);
			if (tmp)
				hg_stack_push(estack, tmp);
			retval = hg_stack_push(estack, self);
			if (!tmp)
				_hg_operator_set_error(vm, op, VM_e_VMerror);
			else if (!retval)
				_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			else
				hg_stack_pop(ostack);
		} else {
			retval = TRUE;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (execstack)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	guint edepth = hg_stack_depth(estack);
	guint len, i;
	HgValueNode *node, *dup_node;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (node);
		len = hg_array_length(array);
		if (edepth > len) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		/* don't include the last node. it's a node for this call */
		for (i = 0; i < edepth - 1; i++) {
			node = hg_stack_index(estack, edepth - i - 1);
			dup_node = hg_object_dup((HgObject *)node);
			hg_array_replace(array, dup_node, i);
		}
		if (edepth != (len + 1)) {
			HgMemObject *obj;

			hg_mem_get_object__inline(array, obj);
			if (obj == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			array = hg_array_make_subarray(obj->pool, array, 0, edepth - 2);
			HG_VALUE_MAKE_ARRAY (node, array);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, node);
			/* it must be true */
		} else {
			retval = TRUE;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (executeonly)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node) &&
		    !HG_IS_VALUE_FILE (node) &&
		    !HG_IS_VALUE_STRING (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executeonly((HgObject *)node) &&
		    !hg_object_is_readable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		hg_object_executeonly((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (exit)
G_STMT_START
{
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(estack), i, j;
	const gchar *name;
	HgValueNode *node;

	for (i = 0; i < depth; i++) {
		node = hg_stack_index(estack, i);
		if (HG_IS_VALUE_POINTER (node)) {
			/* target operators are:
			 * cshow filenameforall for forall kshow loop pathforall
			 * repeat resourceforall
			 */
			name = hg_operator_get_name(HG_VALUE_GET_POINTER (node));
			if (strcmp(name, "%for_pos_int_continue") == 0 ||
			    strcmp(name, "%for_pos_real_continue") == 0) {
				/* drop down ini inc limit proc in estack */
				hg_stack_pop(estack);
				hg_stack_pop(estack);
				hg_stack_pop(estack);
				hg_stack_pop(estack);
			} else if (strcmp(name, "%loop_continue") == 0) {
				/* drop down proc in estack */
				hg_stack_pop(estack);
			} else if (strcmp(name, "%repeat_continue") == 0) {
				/* drop down n proc in estack */
				hg_stack_pop(estack);
				hg_stack_pop(estack);
			} else if (strcmp(name, "%forall_array_continue") == 0 ||
				   strcmp(name, "%forall_dict_continue") == 0 ||
				   strcmp(name, "%forall_string_continue") == 0) {
				/* drop down n val proc in estack */
				hg_stack_pop(estack);
				hg_stack_pop(estack);
				hg_stack_pop(estack);
			} else {
				continue;
			}
			for (j = 0; j < i; j++)
				hg_stack_pop(estack);
			retval = TRUE;
			break;
		} else if (HG_IS_VALUE_FILE (node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidexit);
			break;
		}
	}
	if (i == depth)
		_hg_operator_set_error(vm, op, VM_e_invalidexit);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (exp)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gdouble base, exponent, result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1)) {
			base = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		} else if (HG_IS_VALUE_REAL (n1)) {
			base = HG_VALUE_GET_REAL (n1);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2)) {
			exponent = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		} else if (HG_IS_VALUE_REAL (n2)) {
			exponent = HG_VALUE_GET_REAL (n2);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_VALUE_REAL_SIMILAR (base, 0.0) &&
		    HG_VALUE_REAL_SIMILAR (exponent, 0.0)) {
			_hg_operator_set_error(vm, op, VM_e_undefinedresult);
			break;
		}
		result = pow(base, exponent);
		HG_VALUE_MAKE_REAL (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

static HgFileType
_hg_operator_get_file_type(const gchar *p)
{
	HgFileType retval;

	if (p == NULL || *p == 0)
		retval = 0;
	else if (strcmp(p, "%stdin") == 0)
		retval = HG_FILE_TYPE_STDIN;
	else if (strcmp(p, "%stdout") == 0)
		retval = HG_FILE_TYPE_STDOUT;
	else if (strcmp(p, "%stderr") == 0)
		retval = HG_FILE_TYPE_STDERR;
	else if (strcmp(p, "%statementedit") == 0)
		retval = HG_FILE_TYPE_STATEMENT_EDIT;
	else if (strcmp(p, "%lineedit") == 0)
		retval = HG_FILE_TYPE_LINE_EDIT;
	else
		retval = HG_FILE_TYPE_FILE;

	return retval;
}

static guint
_hg_operator_get_file_mode(const gchar *p)
{
	guint retval;

	if (p == NULL || *p == 0)
		retval = 0;
	else if (strcmp(p, "r+") == 0)
		retval = HG_FILE_MODE_READ | HG_FILE_MODE_READWRITE;
	else if (strcmp(p, "r") == 0)
		retval = HG_FILE_MODE_READ;
	else if (strcmp(p, "w+") == 0)
		retval = HG_FILE_MODE_WRITE | HG_FILE_MODE_READWRITE;
	else if (strcmp(p, "w") == 0)
		retval = HG_FILE_MODE_WRITE;
	else if (strcmp(p, "a+") == 0)
		retval = HG_FILE_MODE_READ | HG_FILE_MODE_WRITE | HG_FILE_MODE_READWRITE;
	else if (strcmp(p, "a") == 0)
		retval = HG_FILE_MODE_READ | HG_FILE_MODE_WRITE;
	else
		retval = 0;

	return retval;
}

DEFUNC_OP (file)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), file_mode;
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gchar *s1 = NULL, *s2 = NULL;
	HgFileObject *file = NULL;
	HgFileType file_type;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_STRING (n1) || !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}

		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		s1 = hg_string_get_string(HG_VALUE_GET_STRING (n1));
		s2 = hg_string_get_string(HG_VALUE_GET_STRING (n2));
		file_type = _hg_operator_get_file_type(s1);
		file_mode = _hg_operator_get_file_mode(s2);

		if (file_type == HG_FILE_TYPE_FILE) {
			file = hg_file_object_new(pool, file_type, file_mode, s1);
			if (hg_file_object_has_error(file)) {
				_hg_operator_set_error_from_file(vm, op, file);
				break;
			}
		} else if (file_type == HG_FILE_TYPE_STDIN) {
			if (file_mode != HG_FILE_MODE_READ) {
				_hg_operator_set_error(vm, op, VM_e_invalidfileaccess);
				break;
			}
			file = hg_vm_get_io(vm, VM_IO_STDIN);
		} else if (file_type == HG_FILE_TYPE_STDOUT) {
			if (file_mode != HG_FILE_MODE_WRITE) {
				_hg_operator_set_error(vm, op, VM_e_invalidfileaccess);
				break;
			}
			file = hg_vm_get_io(vm, VM_IO_STDOUT);
		} else if (file_type == HG_FILE_TYPE_STDERR) {
			if (file_mode != HG_FILE_MODE_WRITE) {
				_hg_operator_set_error(vm, op, VM_e_invalidfileaccess);
				break;
			}
			file = hg_vm_get_io(vm, VM_IO_STDERR);
		} else if (file_type == HG_FILE_TYPE_STATEMENT_EDIT) {
			if (file_mode != HG_FILE_MODE_READ) {
				_hg_operator_set_error(vm, op, VM_e_invalidfileaccess);
				break;
			}
			file = hg_file_object_new(pool, file_type, hg_vm_get_line_editor(vm));
			if (file == NULL) {
				_hg_operator_set_error(vm, op, VM_e_undefinedfilename);
				break;
			}
			if (hg_file_object_has_error(file)) {
				_hg_operator_set_error_from_file(vm, op, file);
				break;
			}
		} else if (file_type == HG_FILE_TYPE_LINE_EDIT) {
			if (file_mode != HG_FILE_MODE_READ) {
				_hg_operator_set_error(vm, op, VM_e_invalidfileaccess);
				break;
			}
			file = hg_file_object_new(pool, file_type, hg_vm_get_line_editor(vm));
			if (file == NULL) {
				_hg_operator_set_error(vm, op, VM_e_undefinedfilename);
				break;
			}
			if (hg_file_object_has_error(file)) {
				_hg_operator_set_error_from_file(vm, op, file);
				break;
			}
		} else {
			g_warning("unknown open type: %d", file_type);
		}
		if (file == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		HG_VALUE_MAKE_FILE (n1, file);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (fill)
G_STMT_START
{
	retval = hg_graphics_render_fill(hg_vm_get_graphics(vm));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (flattenpath);

DEFUNC_OP (flush)
G_STMT_START
{
	(void)vm; /* dummy to stop an warning */
	/* FIXME: buffering isn't supported. */
	sync();
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (flushfile)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n;
	HgFileObject *file;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_FILE (n)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		file = HG_VALUE_GET_FILE (n);
		hg_file_object_flush(file);
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (fontdirectory);

DEFUNC_OP (for)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint odepth = hg_stack_depth(ostack);
	HgValueNode *nini, *ninc, *nlimit, *nproc, *node, *self;
	gboolean fint = TRUE;

	while (1) {
		if (odepth < 4) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nproc = hg_stack_index(ostack, 0);
		nlimit = hg_stack_index(ostack, 1);
		ninc = hg_stack_index(ostack, 2);
		nini = hg_stack_index(ostack, 3);
		if (!HG_IS_VALUE_ARRAY (nproc)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    (!hg_object_is_readable((HgObject *)nproc) &&
		     !hg_object_is_executeonly((HgObject *)nproc))) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_REAL (nlimit) ||
		    HG_IS_VALUE_REAL (ninc) ||
		    HG_IS_VALUE_REAL (nini))
			fint = FALSE;
		if (HG_IS_VALUE_REAL (nlimit)) {
			/* nothing to do */
		} else if (HG_IS_VALUE_INTEGER (nlimit)) {
			if (!fint)
				HG_VALUE_SET_REAL (nlimit, HG_VALUE_GET_INTEGER (nlimit));
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_REAL (ninc)) {
			/* nothing to do */
		} else if (HG_IS_VALUE_INTEGER (ninc)) {
			if (!fint)
				HG_VALUE_SET_REAL (ninc, HG_VALUE_GET_INTEGER (ninc));
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_REAL (nini)) {
			/* nothing to do */
		} else if (HG_IS_VALUE_INTEGER (nini)) {
			if (!fint)
				HG_VALUE_SET_REAL (nini, HG_VALUE_GET_INTEGER (nini));
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}

		self = hg_stack_pop(estack);
		hg_stack_push(estack, nini);
		hg_stack_push(estack, ninc);
		hg_stack_push(estack, nlimit);
		hg_stack_push(estack, nproc);
		if (fint)
			node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
							  "%for_pos_int_continue");
		else
			node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
							  "%for_pos_real_continue");
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, self); /* dummy */
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			break;
		}

		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (forall)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *nval, *nproc, *nn, *self;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nproc = hg_stack_index(ostack, 0);
		nval = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_ARRAY (nproc)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    (!hg_object_is_readable((HgObject *)nproc) &&
		     !hg_object_is_executeonly((HgObject *)nproc)) ||
		    !hg_object_is_readable((HgObject *)nval)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (nval)) {
			node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
							  "%forall_array_continue");
			HG_VALUE_MAKE_INTEGER (pool, nn, 0);
		} else if (HG_IS_VALUE_DICT (nval)) {
			HgDict *dict = HG_VALUE_GET_DICT (nval);

			if (!hg_object_is_readable((HgObject *)dict)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
							  "%forall_dict_continue");
			HG_VALUE_SET_DICT (nval, hg_object_dup((HgObject *)dict));
			HG_VALUE_MAKE_INTEGER (pool, nn, 0);
		} else if (HG_IS_VALUE_STRING (nval)) {
			node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
							  "%forall_string_continue");
			HG_VALUE_MAKE_INTEGER (pool, nn, 0);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		self = hg_stack_pop(estack);
		hg_stack_push(estack, nn);
		hg_stack_push(estack, nval);
		hg_stack_push(estack, nproc);
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, self); /* dummy */
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ge)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if ((HG_IS_VALUE_INTEGER (n1) || HG_IS_VALUE_REAL (n1)) &&
		    (HG_IS_VALUE_INTEGER (n2) || HG_IS_VALUE_REAL (n2))) {
			gdouble d1;

			if (HG_IS_VALUE_INTEGER (n1))
				d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
			else
				d1 = HG_VALUE_GET_REAL (n1);
			if (HG_IS_VALUE_INTEGER (n2))
				result = d1 >= HG_VALUE_GET_REAL_FROM_INTEGER (n2);
			else
				result = d1 >= HG_VALUE_GET_REAL (n2);
		} else if (HG_IS_VALUE_STRING (n1) &&
			   HG_IS_VALUE_STRING (n2)) {
			HgString *s1, *s2;

			if (!hg_object_is_readable((HgObject *)n1) ||
			    !hg_object_is_readable((HgObject *)n2)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) >= 0);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (get)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), len;
	HgValueNode *n1, *n2, *node = NULL;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgArray *array;
	HgDict *dict;
	HgString *string;
	gint32 index;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_ARRAY (n1)) {
			if (!hg_object_is_readable((HgObject *)n1)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			if (!HG_IS_VALUE_INTEGER (n2)) {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			array = HG_VALUE_GET_ARRAY (n1);
			len = hg_array_length(array);
			if (index < 0 || index >= len) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			node = hg_array_index(array, index);
		} else if (HG_IS_VALUE_DICT (n1)) {
			dict = HG_VALUE_GET_DICT (n1);
			if (!hg_object_is_readable((HgObject *)dict)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			node = hg_dict_lookup(dict, n2);
			if (node == NULL) {
				_hg_operator_set_error(vm, op, VM_e_undefined);
				break;
			}
		} else if (HG_IS_VALUE_STRING (n1)) {
			if (!hg_object_is_readable((HgObject *)n1)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			if (!HG_IS_VALUE_INTEGER (n2)) {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			string = HG_VALUE_GET_STRING (n1);
			len = hg_string_maxlength(string);
			if (index < 0 || index >= len) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			HG_VALUE_MAKE_INTEGER (pool, n1, hg_string_index(string, index));
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			node = n1;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (getinterval)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2, *n3;
	HgMemObject *obj;
	gint32 index, count;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n3 = hg_stack_index(ostack, 0);
		n2 = hg_stack_index(ostack, 1);
		n1 = hg_stack_index(ostack, 2);
		if (!HG_IS_VALUE_INTEGER (n2) ||
		    !HG_IS_VALUE_INTEGER (n3)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		index = HG_VALUE_GET_INTEGER (n2);
		count = HG_VALUE_GET_INTEGER (n3);
		if (!hg_object_is_readable((HgObject *)n1)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (n1)) {
			HgArray *array = HG_VALUE_GET_ARRAY (n1), *subarray;
			guint len = hg_array_length(array);

			if (index >= len ||
			    (len - index) < count) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			hg_mem_get_object__inline(array, obj);
			subarray = hg_array_make_subarray(obj->pool, array, index, index + count - 1);
			HG_VALUE_MAKE_ARRAY (n1, subarray);
		} else if (HG_IS_VALUE_STRING (n1)) {
			HgString *string = HG_VALUE_GET_STRING (n1), *substring;
			guint len = hg_string_maxlength(string);

			if (index >= len ||
			    (len - index) < count) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			hg_mem_get_object__inline(string, obj);
			substring = hg_string_make_substring(obj->pool, string, index, index + count - 1);
			HG_VALUE_MAKE_STRING (n1, substring);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (grestore)
G_STMT_START
{
	retval = hg_graphics_restore(hg_vm_get_graphics(vm));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (grestoreall);

DEFUNC_OP (gsave)
G_STMT_START
{
	retval = hg_graphics_save(hg_vm_get_graphics(vm));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (gt)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if ((HG_IS_VALUE_INTEGER (n1) || HG_IS_VALUE_REAL (n1)) &&
		    (HG_IS_VALUE_INTEGER (n2) || HG_IS_VALUE_REAL (n2))) {
			gdouble d1;

			if (HG_IS_VALUE_INTEGER (n1))
				d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
			else
				d1 = HG_VALUE_GET_REAL (n1);
			if (HG_IS_VALUE_INTEGER (n2))
				result = d1 > HG_VALUE_GET_REAL_FROM_INTEGER (n2);
			else
				result = d1 > HG_VALUE_GET_REAL (n2);
		} else if (HG_IS_VALUE_STRING (n1) &&
			   HG_IS_VALUE_STRING (n2)) {
			HgString *s1, *s2;

			if (!hg_object_is_readable((HgObject *)n1) ||
			    !hg_object_is_readable((HgObject *)n2)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) > 0);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (identmatrix)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n, *node;
	HgArray *array;
	guint i;
	gdouble d[6] = {1.0, .0, .0, 1.0, .0, .0};
	HgMemObject *obj;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (n)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (n);
		if (hg_array_length(array) != 6) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		hg_mem_get_object__inline(array, obj);
		for (i = 0; i < 6; i++) {
			HG_VALUE_MAKE_REAL (obj->pool, node, d[i]);
			if (node == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_array_replace(array, node, i);
		}
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (idiv)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 i1, i2;
	guint depth = hg_stack_depth(ostack);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n1) ||
		    !HG_IS_VALUE_INTEGER (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		i1 = HG_VALUE_GET_INTEGER (n1);
		i2 = HG_VALUE_GET_INTEGER (n2);
		if (i2 == 0) {
			_hg_operator_set_error(vm, op, VM_e_undefinedresult);
			break;
		}
		HG_VALUE_MAKE_INTEGER (pool, n1, i1 / i2);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (idtransform);

DEFUNC_OP (if)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nflag, *nif, *self, *node;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nif = hg_stack_index(ostack, 0);
		nflag = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_ARRAY (nif) ||
		    !HG_IS_VALUE_BOOLEAN (nflag)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nif) ||
		    (!hg_object_is_readable((HgObject *)nif) &&
		     !hg_object_is_executeonly((HgObject *)nif))) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		self = hg_stack_pop(estack);
		if (HG_VALUE_GET_BOOLEAN (nflag)) {
			node = hg_object_copy((HgObject *)nif);
			retval = hg_stack_push(estack, node);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
				break;
			}
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(estack, self); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ifelse)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nflag, *nif, *nelse, *self, *node;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nelse = hg_stack_index(ostack, 0);
		nif = hg_stack_index(ostack, 1);
		nflag = hg_stack_index(ostack, 2);
		if (!HG_IS_VALUE_ARRAY (nelse) ||
		    !HG_IS_VALUE_ARRAY (nif) ||
		    !HG_IS_VALUE_BOOLEAN (nflag)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nif) ||
		    !hg_object_is_executable((HgObject *)nelse) ||
		    (!hg_object_is_readable((HgObject *)nif) &&
		     !hg_object_is_executeonly((HgObject *)nif)) ||
		    (!hg_object_is_readable((HgObject *)nelse) &&
		     !hg_object_is_executeonly((HgObject *)nelse))) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		self = hg_stack_pop(estack);
		if (HG_VALUE_GET_BOOLEAN (nflag)) {
			node = hg_object_copy((HgObject *)nif);
			retval = hg_stack_push(estack, node);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
				break;
			}
		} else {
			node = hg_object_copy((HgObject *)nelse);
			retval = hg_stack_push(estack, node);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
				break;
			}
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(estack, self); /* dummy */
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (image);
DEFUNC_UNIMPLEMENTED_OP (imagemask);

DEFUNC_OP (index)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *node;
	gint32 i;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n1 = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (n1)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		i = HG_VALUE_GET_INTEGER (n1);
		if (i < 0 || (i + 1) >= depth) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		node = hg_stack_index(ostack, i + 1);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (initclip)
G_STMT_START
{
	retval = hg_graphics_initclip(hg_vm_get_graphics(vm));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (initgraphics)
G_STMT_START
{
	retval = hg_graphics_init(hg_vm_get_graphics(vm));
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (initmatrix);
DEFUNC_UNIMPLEMENTED_OP (internaldict);
DEFUNC_UNIMPLEMENTED_OP (invertmatrix);
DEFUNC_UNIMPLEMENTED_OP (itransform);

DEFUNC_OP (known)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *ndict, *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgDict *dict;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		ndict = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_DICT (ndict)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (ndict);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (hg_dict_lookup(dict, node) != NULL) {
			HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
		} else {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (kshow);

DEFUNC_OP (le)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if ((HG_IS_VALUE_INTEGER (n1) || HG_IS_VALUE_REAL (n1)) &&
		    (HG_IS_VALUE_INTEGER (n2) || HG_IS_VALUE_REAL (n2))) {
			gdouble d1;

			if (HG_IS_VALUE_INTEGER (n1))
				d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
			else
				d1 = HG_VALUE_GET_REAL (n1);
			if (HG_IS_VALUE_INTEGER (n2))
				result = d1 <= HG_VALUE_GET_REAL_FROM_INTEGER (n2);
			else
				result = d1 <= HG_VALUE_GET_REAL (n2);
		} else if (HG_IS_VALUE_STRING (n1) &&
			   HG_IS_VALUE_STRING (n2)) {
			HgString *s1, *s2;

			if (!hg_object_is_readable((HgObject *)n1) ||
			    !hg_object_is_readable((HgObject *)n2)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) <= 0);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (length)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!hg_object_is_readable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (node)) {
			HgArray *array = HG_VALUE_GET_ARRAY (node);

			HG_VALUE_MAKE_INTEGER (pool, node, hg_array_length(array));
		} else if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);

			HG_VALUE_MAKE_INTEGER (pool, node, hg_dict_length(dict));
		} else if (HG_IS_VALUE_STRING (node)) {
			HgString *string = HG_VALUE_GET_STRING (node);

			HG_VALUE_MAKE_INTEGER (pool, node, hg_string_maxlength(string));
		} else if (HG_IS_VALUE_NAME (node)) {
			gchar *name = HG_VALUE_GET_NAME (node);

			HG_VALUE_MAKE_INTEGER (pool, node, strlen(name));
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (lineto)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx, *ny;
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));
	gdouble dx, dy, d;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		ny = hg_stack_index(ostack, 0);
		nx = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_path_compute_current_point(gstate->path, &d, &d)) {
			_hg_operator_set_error(vm, op, VM_e_nocurrentpoint);
			break;
		}
		retval = hg_graphic_state_path_lineto(gstate, dx, dy);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ln)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n;
	gdouble d;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (n);
		} else if (HG_IS_VALUE_REAL (n)) {
			d = HG_VALUE_GET_REAL (n);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, n, log(d));
		if (n == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (load)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nkey, *nval;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nkey = hg_stack_index(ostack, 0);
		nval = hg_vm_lookup(vm, nkey);
		if (nval == NULL) {
			_hg_operator_set_error(vm, op, VM_e_undefined);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, nval);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (log)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n;
	gdouble d;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (n);
		} else if (HG_IS_VALUE_REAL (n)) {
			d = HG_VALUE_GET_REAL (n);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, n, log10(d));
		if (n == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (loop)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *self, *nproc, *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nproc = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (nproc)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    (!hg_object_is_readable((HgObject *)nproc) &&
		     !hg_object_is_executeonly((HgObject *)nproc))) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		self = hg_stack_pop(estack);
		hg_stack_push(estack, nproc);
		node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
						  "%loop_continue");
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, self); /* dummy */
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			break;
		}

		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (lt)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if ((HG_IS_VALUE_INTEGER (n1) || HG_IS_VALUE_REAL (n1)) &&
		    (HG_IS_VALUE_INTEGER (n2) || HG_IS_VALUE_REAL (n2))) {
			gdouble d1;

			if (HG_IS_VALUE_INTEGER (n1))
				d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
			else
				d1 = HG_VALUE_GET_REAL (n1);
			if (HG_IS_VALUE_INTEGER (n2))
				result = d1 < HG_VALUE_GET_REAL_FROM_INTEGER (n2);
			else
				result = d1 < HG_VALUE_GET_REAL (n2);
		} else if (HG_IS_VALUE_STRING (n1) &&
			   HG_IS_VALUE_STRING (n2)) {
			HgString *s1, *s2;

			if (!hg_object_is_readable((HgObject *)n1) ||
			    !hg_object_is_readable((HgObject *)n2)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) < 0);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (makefont);

DEFUNC_OP (maxlength)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgDict *dict;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_DICT (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (node);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		HG_VALUE_MAKE_INTEGER (pool, node, hg_dict_maxlength(dict));
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (mod)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	guint depth = hg_stack_depth(ostack);
	gint32 i2, result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n1) || !HG_IS_VALUE_INTEGER (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		i2 = HG_VALUE_GET_INTEGER (n2);
		if (i2 == 0) {
			_hg_operator_set_error(vm, op, VM_e_undefinedresult);
			break;
		}
		result = HG_VALUE_GET_INTEGER (n1) % i2;
		HG_VALUE_MAKE_INTEGER (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (moveto)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx, *ny;
	gdouble dx, dy;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		ny = hg_stack_index(ostack, 0);
		nx = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		retval = hg_graphic_state_path_moveto(hg_graphics_get_state(hg_vm_get_graphics(vm)),
						      dx, dy);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (mul)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gdouble d1, d2;
	guint depth = hg_stack_depth(ostack);
	gdouble integer = TRUE;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
			integer = FALSE;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
			integer = FALSE;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		d1 *= d2;
		if (integer && d1 <= G_MAXINT) {
			HG_VALUE_MAKE_INTEGER (pool, n1, d1);
		} else {
			HG_VALUE_MAKE_REAL (pool, n1, d1);
		}
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ne)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		result = !hg_value_node_compare(n1, n2);
		if (result) {
			if ((HG_IS_VALUE_NAME (n1) || HG_IS_VALUE_STRING (n1)) &&
			    (HG_IS_VALUE_NAME (n2) || HG_IS_VALUE_STRING (n2))) {
				gchar *str1, *str2;

				if (HG_IS_VALUE_NAME (n1))
					str1 = HG_VALUE_GET_NAME (n1);
				else
					str1 = hg_string_get_string(HG_VALUE_GET_STRING (n1));
				if (HG_IS_VALUE_NAME (n2))
					str2 = HG_VALUE_GET_NAME (n2);
				else
					str2 = hg_string_get_string(HG_VALUE_GET_STRING (n2));
				if (str1 != NULL && str2 != NULL)
					result = (strcmp(str1, str2) != 0);
			} else if ((HG_IS_VALUE_INTEGER (n1) || HG_IS_VALUE_REAL (n1)) &&
				   (HG_IS_VALUE_INTEGER (n2) || HG_IS_VALUE_REAL (n2))) {
				gdouble d1;

				if (HG_IS_VALUE_INTEGER (n1))
					d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
				else
					d1 = HG_VALUE_GET_REAL (n1);
				if (HG_IS_VALUE_INTEGER (n2))
					result = !HG_VALUE_REAL_SIMILAR (d1, HG_VALUE_GET_REAL_FROM_INTEGER (n2));
				else
					result = !HG_VALUE_REAL_SIMILAR (d1, HG_VALUE_GET_REAL (n2));
			}
		}
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (neg)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n, *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	guint depth = hg_stack_depth(ostack);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			HG_VALUE_MAKE_INTEGER (pool, node, -HG_VALUE_GET_INTEGER (n));
		} else if (HG_IS_VALUE_REAL (n)) {
			HG_VALUE_MAKE_REAL (pool, node, -HG_VALUE_GET_REAL (n));
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
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

DEFUNC_OP (newpath)
G_STMT_START
{
	retval = hg_graphic_state_path_new(hg_graphics_get_state(hg_vm_get_graphics(vm)));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (noaccess)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node) &&
		    !HG_IS_VALUE_DICT (node) &&
		    !HG_IS_VALUE_FILE (node) &&
		    !HG_IS_VALUE_STRING (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);

			if (!hg_object_is_writable((HgObject *)dict)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			hg_object_inaccessible((HgObject *)dict);
		}
		hg_object_inaccessible((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (not)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_BOOLEAN (node)) {
			gboolean bool = HG_VALUE_GET_BOOLEAN (node);

			HG_VALUE_MAKE_BOOLEAN (pool, node, !bool);
		} else if (HG_IS_VALUE_INTEGER (node)) {
			gint32 i = HG_VALUE_GET_INTEGER (node);

			HG_VALUE_MAKE_INTEGER (pool, node, ~i);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (nulldevice);

DEFUNC_OP (or)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_BOOLEAN (n1) &&
		    HG_IS_VALUE_BOOLEAN (n2)) {
			gboolean result = HG_VALUE_GET_BOOLEAN (n1) | HG_VALUE_GET_BOOLEAN (n2);

			HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
		} else if (HG_IS_VALUE_INTEGER (n1) &&
			   HG_IS_VALUE_INTEGER (n2)) {
			gint32 result = HG_VALUE_GET_INTEGER (n1) | HG_VALUE_GET_INTEGER (n2);

			HG_VALUE_MAKE_INTEGER (pool, n1, result);
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (pathbbox)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *node;
	HgPathBBox bbox;

	while (1) {
		if (hg_vm_get_emulation_level(vm) == VM_EMULATION_LEVEL_1) {
			retval = hg_graphic_state_get_bbox_from_path(hg_graphics_get_state(hg_vm_get_graphics(vm)),
								     FALSE, &bbox);
		} else {
			retval = hg_graphic_state_get_bbox_from_path(hg_graphics_get_state(hg_vm_get_graphics(vm)),
								     TRUE, &bbox);
		}
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_nocurrentpoint);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, node, bbox.llx);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, bbox.lly);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, bbox.urx);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, bbox.ury);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (pathforall);

DEFUNC_OP (pop)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (print)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgString *hs;
	HgFileObject *stdout;

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
		if (!hg_object_is_readable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		hs = HG_VALUE_GET_STRING (node);
		stdout = hg_vm_get_io(vm, VM_IO_STDOUT);
		hg_file_object_write(stdout,
				     hg_string_get_string(hs),
				     sizeof (gchar),
				     hg_string_maxlength(hs));
		if (hg_file_object_has_error(stdout)) {
			_hg_operator_set_error_from_file(vm, op, stdout);
			break;
		}
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

/* almost code is shared to _hg_operator_op_private_hg_forceput */
DEFUNC_OP (put)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), len;
	gint32 index;
	HgValueNode *n1, *n2, *n3;
	HgMemObject *obj;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n3 = hg_stack_index(ostack, 0);
		n2 = hg_stack_index(ostack, 1);
		n1 = hg_stack_index(ostack, 2);
		if (!hg_object_is_writable((HgObject *)n1)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		hg_mem_get_object__inline(n3, obj);
		if (hg_vm_is_global_object(vm, n1) &&
		    hg_mem_is_complex_mark(obj) &&
		    !hg_vm_is_global_object(vm, n3)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (n1)) {
			HgArray *array = HG_VALUE_GET_ARRAY (n1);

			hg_mem_get_object__inline(array, obj);
			if (!HG_IS_VALUE_INTEGER (n2)) {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			len = hg_array_length(array);
			index = HG_VALUE_GET_INTEGER (n2);
			if (index > len || index > 65535) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			retval = hg_array_replace_forcibly(array, n3, index, TRUE);
		} else if (HG_IS_VALUE_DICT (n1)) {
			HgDict *dict = HG_VALUE_GET_DICT (n1);

			hg_mem_get_object__inline(dict, obj);
			if (!hg_object_is_writable((HgObject *)dict)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			retval = hg_dict_insert_forcibly(obj->pool, dict, n2, n3, TRUE);
		} else if (HG_IS_VALUE_STRING (n1)) {
			HgString *string = HG_VALUE_GET_STRING (n1);
			gint32 c;

			if (!HG_IS_VALUE_INTEGER (n2) || !HG_IS_VALUE_INTEGER (n3)) {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			c = HG_VALUE_GET_INTEGER (n3);
			retval = hg_string_insert_c(string, c, index);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (quit)
G_STMT_START
{
	HgStack *estack = hg_vm_get_estack(vm);
	HgValueNode *self = hg_stack_index(estack, 0);

	hg_stack_clear(estack);
	retval = hg_stack_push(estack, self);
	/* FIXME */
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (rand)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *node;
	GRand *rand_ = hg_vm_get_random_generator(vm);

	HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm),
			       node, g_rand_int_range(rand_, 0, RAND_MAX));
	if (node == NULL) {
		_hg_operator_set_error(vm, op, VM_e_VMerror);
	} else {
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (rcheck)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (node) ||
		    HG_IS_VALUE_FILE (node) ||
		    HG_IS_VALUE_STRING (node)) {
			gboolean result = hg_object_is_readable((HgObject *)node);

			HG_VALUE_MAKE_BOOLEAN (pool, node, result);
		} else if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);
			gboolean result = hg_object_is_readable((HgObject *)dict);

			HG_VALUE_MAKE_BOOLEAN (pool, node, result);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (rcurveto);

DEFUNC_OP (read)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nfile, *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgFileObject *file, *stdin;
	gchar c;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nfile = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_FILE (nfile)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)nfile)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (nfile);
		stdin = hg_vm_get_io(vm, VM_IO_STDIN);
		node = hg_vm_lookup_with_string(vm, "JOBSERVER");
		if (file == stdin &&
		    node != NULL &&
		    HG_IS_VALUE_BOOLEAN (node) &&
		    HG_VALUE_GET_BOOLEAN (node) == TRUE) {
			/* it's under the job server mode now.
			 * this means stdin is being used to get PostScript.
			 * just returns false to not breaks it.
			 */
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, node);
			/* it must be true */
			break;
		}			
		c = hg_file_object_getc(file);
		if (hg_file_object_is_eof(file)) {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, node);
			/* it must be true */
			break;
		} else if (hg_file_object_has_error(file)) {
			_hg_operator_set_error_from_file(vm, op, file);
			break;
		} else {
			hg_stack_pop(ostack);
			HG_VALUE_MAKE_INTEGER (pool, node, c);
			hg_stack_push(ostack, node);
			HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
			retval = hg_stack_push(ostack, node);
			if (!retval)
				_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (readhexstring)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *s, *sub;
	guint maxlength, length = 0;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gchar c, hex = 0;
	gint count = 2;
	HgMemObject *obj;
	gboolean found;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_writable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		s = HG_VALUE_GET_STRING (n2);
		maxlength = hg_string_maxlength(s);
		while (length < maxlength) {
			found = FALSE;
			c = hg_file_object_getc(file);
			if (hg_file_object_is_eof(file)) {
				break;
			}
			if (c >= '0' && c <= '9') {
				c = c - '0';
				found = TRUE;
			} else if (c >= 'a' && c <= 'f') {
				c = c - 'a' + 10;
				found = TRUE;
			} else if (c >= 'A' && c <= 'F') {
				c = c - 'A' + 10;
				found = TRUE;
			}
			if (found) {
				hex = (hex << 4) | c;
				count--;
				if (count == 0) {
					hg_string_insert_c(s, hex, length++);
					count = 2;
					hex = 0;
				}
			}
		}
		if (length < maxlength) {
			hg_mem_get_object__inline(s, obj);
			HG_VALUE_MAKE_BOOLEAN (pool, n1, FALSE);
			sub = hg_string_make_substring(obj->pool, s, 0, length - 1);
			HG_VALUE_MAKE_STRING (n2, sub);
		} else {
			HG_VALUE_MAKE_BOOLEAN (pool, n1, TRUE);
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_push(ostack, n2);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (readline)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgString *s, *sresult;
	HgFileObject *file;
	guchar c;
	guint length = 0, maxlength;
	gboolean result = FALSE;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_writable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		s = HG_VALUE_GET_STRING (n2);
		maxlength = hg_string_maxlength(s);
		while (1) {
			c = hg_file_object_getc(file);
			if (hg_file_object_is_eof(file))
				break;
			if (c == '\r') {
				c = hg_file_object_getc(file);
				if (c != '\n')
					hg_file_object_ungetc(file, c);
				result = TRUE;
				break;
			} else if (c == '\n') {
				result = TRUE;
				break;
			}
			if (length > maxlength) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				return FALSE;
			}
			hg_string_insert_c(s, c, length++);
		}
		sresult = hg_string_make_substring(pool, s, 0, length - 1);
		HG_VALUE_MAKE_STRING (n1, sresult);
		HG_VALUE_MAKE_BOOLEAN (pool, n2, result);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_push(ostack, n1);
		retval = hg_stack_push(ostack, n2);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (readonly)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node) &&
		    !HG_IS_VALUE_DICT (node) &&
		    !HG_IS_VALUE_FILE (node) &&
		    !HG_IS_VALUE_STRING (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)node) &&
		    !hg_object_is_readable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);

			if (!hg_object_is_writable((HgObject *)dict) &&
			    !hg_object_is_readable((HgObject *)dict)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			hg_object_unwritable((HgObject *)dict);
		}
		hg_object_unwritable((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (readstring)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *s, *sub;
	guint length = 0, maxlength;
	gchar c;
	HgMemObject *obj;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_writable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		s = HG_VALUE_GET_STRING (n2);
		maxlength = hg_string_maxlength(s);
		if (maxlength <= 0) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		while (length < maxlength) {
			c = hg_file_object_getc(file);
			if (hg_file_object_is_eof(file))
				break;
			hg_string_insert_c(s, c, length++);
		}
		if (length < maxlength) {
			hg_mem_get_object__inline(s, obj);
			HG_VALUE_MAKE_BOOLEAN (pool, n1, FALSE);
			sub = hg_string_make_substring(obj->pool, s, 0, length - 1);
			HG_VALUE_MAKE_STRING (n2, sub);
		} else {
			HG_VALUE_MAKE_BOOLEAN (pool, n1, TRUE);
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_push(ostack, n2);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (repeat)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n, *nproc, *node, *self;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nproc = hg_stack_index(ostack, 0);
		n = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n) || !HG_IS_VALUE_ARRAY (nproc)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    (!hg_object_is_readable((HgObject *)nproc) &&
		     !hg_object_is_executeonly((HgObject *)nproc))) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_VALUE_GET_INTEGER (n) < 0) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		self = hg_stack_pop(estack);
		/* only n must be copied. */
		node = hg_object_copy((HgObject *)n);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(estack, node);
		hg_stack_push(estack, nproc);
		node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
						  "%repeat_continue");
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, self); /* dummy */
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			break;
		}

		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (resetfile);

DEFUNC_OP (restore)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemSnapshot *snapshot;
	HgMemPool *pool;
	gboolean global_mode;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_SNAPSHOT (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		snapshot = HG_VALUE_GET_SNAPSHOT (node);
		global_mode = hg_vm_is_global_pool_used(vm);
		hg_vm_use_global_pool(vm, FALSE);
		pool = hg_vm_get_current_pool(vm);
		hg_vm_use_global_pool(vm, global_mode);
		retval = hg_mem_pool_restore_snapshot(pool, snapshot);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_invalidrestore);
			break;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (reversepath);

DEFUNC_OP (rlineto)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx, *ny;
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));
	gdouble dx, dy, d;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		ny = hg_stack_index(ostack, 0);
		nx = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_path_compute_current_point(gstate->path, &d, &d)) {
			_hg_operator_set_error(vm, op, VM_e_nocurrentpoint);
			break;
		}
		retval = hg_graphic_state_path_rlineto(gstate, dx, dy);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (rmoveto);

DEFUNC_OP (roll)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nd, *ni;
	gint32 index, d;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nd = hg_stack_index(ostack, 0);
		ni = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (nd) || !HG_IS_VALUE_INTEGER (ni)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		index = HG_VALUE_GET_INTEGER (ni);
		if (index < 0) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		if ((depth - 2) < index) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		d = HG_VALUE_GET_INTEGER (nd);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_roll(ostack, index, d);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (rotate)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), len;
	HgValueNode *node, *nmatrix;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgArray *matrix = NULL;
	HgMatrix *mtx;
	gdouble angle;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nmatrix = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (nmatrix)) {
			if (depth < 2) {
				_hg_operator_set_error(vm, op, VM_e_stackunderflow);
				break;
			}
			if (!hg_object_is_writable((HgObject *)nmatrix)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			len = hg_array_length(matrix);
			if (len != 6) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			node = hg_stack_index(ostack, 1);
		} else {
			node = nmatrix;
			nmatrix = NULL;
		}
		if (HG_IS_VALUE_INTEGER (node))
			angle = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		else if (HG_IS_VALUE_REAL (node))
			angle = HG_VALUE_GET_REAL (node);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		angle *= 2 * M_PI / 360;
		if (nmatrix != NULL) {
			mtx = hg_matrix_rotate(pool, angle);
			if (mtx == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			HG_VALUE_SET_REAL (hg_array_index(matrix, 0), mtx->xx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 1), mtx->yx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 2), mtx->xy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 3), mtx->yy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 4), mtx->x0);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 5), mtx->y0);

			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, nmatrix);
		} else {
			retval = hg_graphics_matrix_rotate(hg_vm_get_graphics(vm),
							   angle);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (round)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n;
	gdouble result;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			/* nothing to do */
			retval = TRUE;
		} else if (HG_IS_VALUE_REAL (n)) {
			result = round(HG_VALUE_GET_REAL (n));
			HG_VALUE_MAKE_REAL (pool, n, result);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, n);
			/* it must be true */
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (rrand);

DEFUNC_OP (save)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgMemSnapshot *snapshot;
	HgMemPool *pool;
	gboolean global_mode = hg_vm_is_global_pool_used(vm);
	HgValueNode *node;

	while (1) {
		hg_vm_use_global_pool(vm, FALSE);
		pool = hg_vm_get_current_pool(vm);
		snapshot = hg_mem_pool_save_snapshot(pool);
		HG_VALUE_MAKE_SNAPSHOT (node, snapshot);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_vm_use_global_pool(vm, global_mode);
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (scale)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx, *ny, *nmatrix;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgArray *matrix;
	HgMatrix *mtx;
	gdouble dx, dy;
	guint len, offset;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nmatrix = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (nmatrix)) {
			if (depth < 3) {
				_hg_operator_set_error(vm, op, VM_e_stackunderflow);
				break;
			}
			if (!hg_object_is_writable((HgObject *)nmatrix)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			len = hg_array_length(matrix);
			if (len != 6) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			ny = hg_stack_index(ostack, 1);
			offset = 2;
		} else {
			ny = nmatrix;
			nmatrix = NULL;
			offset = 1;
		}
		nx = hg_stack_index(ostack, offset);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (nmatrix != NULL) {
			mtx = hg_matrix_scale(pool, dx, dy);
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 0), mtx->xx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 1), mtx->yx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 2), mtx->xy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 3), mtx->yy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 4), mtx->x0);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 5), mtx->y0);

			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, nmatrix);
		} else {
			retval = hg_graphics_matrix_scale(hg_vm_get_graphics(vm),
							  dx, dy);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (scalefont);

DEFUNC_OP (search)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), length1, length2, i;
	HgValueNode *n1, *n2, *npre, *npost, *nmatch, *nresult;
	HgString *s, *seek, *pre, *post, *match;
	gchar c1, c2;
	HgMemObject *obj;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean found = FALSE;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_STRING (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		s = HG_VALUE_GET_STRING (n1);
		seek = HG_VALUE_GET_STRING (n2);
		length1 = hg_string_length(s);
		length2 = hg_string_length(seek);
		c2 = hg_string_index(seek, 0);
		for (i = 0; i <= (length1 - length2); i++) {
			c1 = hg_string_index(s, i);
			if (c1 == c2) {
				if (hg_string_ncompare_offset_later(s, seek, i, 0, length2)) {
					found = TRUE;
					hg_mem_get_object__inline(s, obj);
					if (i == 0) {
						pre = hg_string_new(obj->pool, 0);
					} else {
						pre = hg_string_make_substring(obj->pool, s, 0, i - 1);
					}
					match = hg_string_make_substring(obj->pool, s, i, i + length2 - 1);
					if ((i + length2) == length1) {
						post = hg_string_new(obj->pool, 0);
					} else {
						post = hg_string_make_substring(obj->pool, s, i + length2, length1 - 1);
					}

					HG_VALUE_MAKE_STRING (npost, post);
					HG_VALUE_MAKE_STRING (nmatch, match);
					HG_VALUE_MAKE_STRING (npre, pre);
					HG_VALUE_MAKE_BOOLEAN (pool, nresult, TRUE);

					hg_stack_pop(ostack);
					hg_stack_pop(ostack);
					hg_stack_push(ostack, npost);
					hg_stack_push(ostack, nmatch);
					hg_stack_push(ostack, npre);
					retval = hg_stack_push(ostack, nresult);
					if (!retval)
						_hg_operator_set_error(vm, op, VM_e_stackoverflow);
				}
			}
		}
		if (!found) {
			hg_stack_pop(ostack);
			HG_VALUE_MAKE_BOOLEAN (pool, nresult, FALSE);
			retval = hg_stack_push(ostack, nresult);
			/* it must be true */
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (setcachedevice);
DEFUNC_UNIMPLEMENTED_OP (setcachelimit);
DEFUNC_UNIMPLEMENTED_OP (setcharwidth);
DEFUNC_UNIMPLEMENTED_OP (setdash);
DEFUNC_UNIMPLEMENTED_OP (setflat);
DEFUNC_UNIMPLEMENTED_OP (setfont);

DEFUNC_OP (setgray)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node))
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		else if (HG_IS_VALUE_REAL (node))
			d = HG_VALUE_GET_REAL (node);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_graphic_state_color_set_rgb(hg_vm_get_graphics(vm)->current_gstate,
							d, d, d);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (sethsbcolor)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nh, *ns, *nb;
	gdouble dh, ds, db;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nh = hg_stack_index(ostack, 0);
		ns = hg_stack_index(ostack, 1);
		nb = hg_stack_index(ostack, 2);
		if (HG_IS_VALUE_INTEGER (nh))
			dh = HG_VALUE_GET_REAL_FROM_INTEGER (nh);
		else if (HG_IS_VALUE_REAL (nh))
			dh = HG_VALUE_GET_REAL (nh);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ns))
			ds = HG_VALUE_GET_REAL_FROM_INTEGER (ns);
		else if (HG_IS_VALUE_REAL (ns))
			ds = HG_VALUE_GET_REAL (ns);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nb))
			db = HG_VALUE_GET_REAL_FROM_INTEGER (nb);
		else if (HG_IS_VALUE_REAL (nb))
			db = HG_VALUE_GET_REAL (nb);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		retval = hg_graphic_state_color_set_hsv(hg_vm_get_graphics(vm)->current_gstate,
							dh, ds, db);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (setlinecap);
DEFUNC_UNIMPLEMENTED_OP (setlinejoin);

DEFUNC_OP (setlinewidth)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node))
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		else if (HG_IS_VALUE_REAL (node))
			d = HG_VALUE_GET_REAL (node);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_graphic_state_set_linewidth(hg_vm_get_graphics(vm)->current_gstate, d);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (setmatrix)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), i;
	HgValueNode *node;
	HgArray *matrix;
	HgGraphicState *gstate = hg_graphics_get_state(hg_vm_get_graphics(vm));
	HgMatrix *mtx;
	gdouble dmatrix[6];

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		matrix = HG_VALUE_GET_ARRAY (node);
		if (hg_array_length(matrix) != 6) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		for (i = 0; i < 6; i++) {
			node = hg_array_index(matrix, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL (node);
			} else {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
		}
		if (i == 6) {
			mtx = hg_matrix_new(hg_vm_get_current_pool(vm),
					    dmatrix[0], dmatrix[1], dmatrix[2],
					    dmatrix[3], dmatrix[4], dmatrix[5]);
			if (mtx == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			memcpy(&gstate->ctm, mtx, sizeof (HgMatrix));
			hg_mem_free(mtx);
			retval = hg_path_matrix(gstate->path,
						gstate->ctm.xx, gstate->ctm.yx,
						gstate->ctm.xy, gstate->ctm.yy,
						gstate->ctm.x0, gstate->ctm.y0);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (setmiterlimit);

DEFUNC_OP (setrgbcolor)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nr, *ng, *nb;
	gdouble dr, dg, db;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nb = hg_stack_index(ostack, 0);
		ng = hg_stack_index(ostack, 1);
		nr = hg_stack_index(ostack, 2);
		if (HG_IS_VALUE_INTEGER (nr))
			dr = HG_VALUE_GET_REAL_FROM_INTEGER (nr);
		else if (HG_IS_VALUE_REAL (nr))
			dr = HG_VALUE_GET_REAL (nr);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ng))
			dg = HG_VALUE_GET_REAL_FROM_INTEGER (ng);
		else if (HG_IS_VALUE_REAL (ng))
			dg = HG_VALUE_GET_REAL (ng);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nb))
			db = HG_VALUE_GET_REAL_FROM_INTEGER (nb);
		else if (HG_IS_VALUE_REAL (nb))
			db = HG_VALUE_GET_REAL (nb);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		retval = hg_graphic_state_color_set_rgb(hg_vm_get_graphics(vm)->current_gstate,
							dr, dg, db);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (setscreen);
DEFUNC_UNIMPLEMENTED_OP (settransfer);
DEFUNC_UNIMPLEMENTED_OP (show);

DEFUNC_OP (showpage)
G_STMT_START
{
	/* FIXME */
	/* correct procedure are:
	 * 1) invoke an EndPage procedure in pagedevice dictionary and
	 *    push a page number and the code to ostack, which means
	 *    that EndPage procedure was invoked from showpage. (see Section 6.2.6)
	 * 2) if EndPage procedure returned true, invoke erasepage to clear
	 *    the raster memory after transfering the current page to the output device.
	 * 3) initialize the graphic state with initgraphics.
	 * 4) invoke a BeginPage procedure in pagedevice dictionary and
	 *    push a page number to ostack.
	 */
	retval = hg_graphics_show_page(hg_vm_get_graphics(vm));
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (sin)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (hg_vm_get_current_pool(vm),
				    node,
				    sin(d * (2 * M_PI / 360)));
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

DEFUNC_OP (sqrt)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gdouble d;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (d < 0) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, node, sqrt(d));
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (srand)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	GRand *rand_;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		rand_ = hg_vm_get_random_generator(vm);
		g_rand_set_seed(rand_, (guint32)HG_VALUE_GET_INTEGER (node));
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (standardencoding);
DEFUNC_UNIMPLEMENTED_OP (start);

DEFUNC_OP (status)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n, *npages, *nbytes, *nreferenced, *ncreated;
	HgFileObject *file;
	HgString *s;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gchar *filename;
	struct stat st;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_FILE (n)) {
			file = HG_VALUE_GET_FILE (n);
			HG_VALUE_MAKE_BOOLEAN (pool, n, !hg_file_object_is_closed(file));
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, n);
			/* it must be true */
		} else if (HG_IS_VALUE_STRING (n)) {
			hg_stack_pop(ostack);

			s = HG_VALUE_GET_STRING (n);
			filename = hg_string_get_string(s);
			if (lstat(filename, &st) == -1) {
				HG_VALUE_MAKE_BOOLEAN (pool, n, FALSE);
			} else {
				HG_VALUE_MAKE_INTEGER (pool, npages, st.st_blocks);
				HG_VALUE_MAKE_INTEGER (pool, nbytes, st.st_size);
				HG_VALUE_MAKE_INTEGER (pool, nreferenced, st.st_atime);
				HG_VALUE_MAKE_INTEGER (pool, ncreated, st.st_mtime);
				HG_VALUE_MAKE_BOOLEAN (pool, n, TRUE);
				hg_stack_push(ostack, npages);
				hg_stack_push(ostack, nbytes);
				hg_stack_push(ostack, nreferenced);
				hg_stack_push(ostack, ncreated);
			}
			retval = hg_stack_push(ostack, n);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_stackoverflow);
			}
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (stop)
G_STMT_START
{
	HgStack *estack = hg_vm_get_estack(vm);
	guint edepth = hg_stack_depth(estack), i;
	HgValueNode *node, *self, *key;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	for (i = 0; i < edepth; i++) {
		node = hg_stack_index(estack, i);
		if (HG_IS_VALUE_POINTER (node) &&
		    strcmp("%stopped_continue", hg_operator_get_name(HG_VALUE_GET_POINTER (node))) == 0) {
			break;
		}
	}
	if (i == edepth) {
		hg_stderr_printf("No /stopped operator found. exiting...\n");
		node = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm), ".abort");
		self = hg_stack_pop(estack);
		hg_stack_push(estack, node);
		retval = hg_stack_push(estack, self);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
	} else {
		/* this operator itself is still in the estack. */
		for (; i > 1; i--)
			hg_stack_pop(estack);
		key = hg_vm_get_name_node(vm, ".isstop");
		HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
		hg_dict_insert(pool, hg_vm_get_dict_error(vm), key, node);
		retval = TRUE;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (stopped)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *self, *proc;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!hg_object_is_readable((HgObject *)node) &&
		    !hg_object_is_executeonly((HgObject *)node)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (hg_object_is_executable((HgObject *)node)) {
			hg_stack_pop(ostack);
			self = hg_stack_pop(estack);
			proc = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
							  "%stopped_continue");
			hg_stack_push(estack, proc);
			hg_stack_push(estack, node);
			retval = hg_stack_push(estack, self);
			if (!retval)
				_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
		} else {
			HG_VALUE_MAKE_BOOLEAN (hg_vm_get_current_pool(vm),
					       node, FALSE);
			retval = hg_stack_push(ostack, node);
			if (!retval)
				_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (string)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	gint32 size;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgString *hs;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		size = HG_VALUE_GET_INTEGER (node);
		if (size < 0 || size > 65535) {
			_hg_operator_set_error(vm, op, VM_e_rangecheck);
			break;
		}
		hs = hg_string_new(pool, size);
		if (hs == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_STRING (node, hs);
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

DEFUNC_UNIMPLEMENTED_OP (stringwidth);

DEFUNC_OP (stroke)
G_STMT_START
{
	retval = hg_graphics_render_stroke(hg_vm_get_graphics(vm));
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (strokepath);

DEFUNC_OP (sub)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gdouble d1, d2;
	guint depth = hg_stack_depth(ostack);
	gdouble integer = TRUE;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
			integer = FALSE;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
			integer = FALSE;
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (integer) {
			HG_VALUE_MAKE_INTEGER (pool, n1, (gint32)(d1 - d2));
		} else {
			HG_VALUE_MAKE_REAL (pool, n1, d1 - d2);
		}
		if (n1 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (token)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *nresult, *npost;
	HgFileObject *file;
	HgString *s, *sub;
	gboolean result = FALSE;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgMemObject *obj;
	gchar c;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n1 = hg_stack_index(ostack, 0);
		if (!hg_object_is_readable((HgObject *)n1)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_FILE (n1)) {
			file = HG_VALUE_GET_FILE (n1);
			nresult = hg_scanner_get_object(vm, file);
			c = hg_file_object_getc(file);
			if (!_hg_scanner_isspace(c)) {
				hg_file_object_ungetc(file, c);
			}
			hg_stack_pop(ostack);
			if (nresult != NULL) {
				hg_stack_push(ostack, nresult);
				result = TRUE;
			}
		} else if (HG_IS_VALUE_STRING (n1)) {
			s = HG_VALUE_GET_STRING (n1);
			file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER,
						  HG_FILE_MODE_READ, "%token",
						  hg_string_get_string(s), hg_string_maxlength(s));
			nresult = hg_scanner_get_object(vm, file);
			c = hg_file_object_getc(file);
			if (!_hg_scanner_isspace(c)) {
				hg_file_object_ungetc(file, c);
			}
			hg_stack_pop(ostack);
			if (nresult != NULL) {
				gssize pos = hg_file_object_seek(file, 0, HG_FILE_POS_CURRENT);
				guint maxlength = hg_string_maxlength(s) - 1;

				hg_mem_get_object__inline(s, obj);
				if (pos > maxlength) {
					sub = hg_string_new(obj->pool, 0);
				} else {
					sub = hg_string_make_substring(obj->pool, s, pos, maxlength);
				}
				HG_VALUE_MAKE_STRING (npost, sub);
				hg_stack_push(ostack, npost);
				hg_stack_push(ostack, nresult);
				result = TRUE;
			}
			hg_mem_free(file);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		retval = hg_stack_push(ostack, n1);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (transform);

DEFUNC_OP (translate)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nx, *ny, *nmatrix;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgArray *matrix;
	HgMatrix *mtx;
	gdouble dx, dy;
	guint len, offset;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nmatrix = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (nmatrix)) {
			if (depth < 3) {
				_hg_operator_set_error(vm, op, VM_e_stackunderflow);
				break;
			}
			if (!hg_object_is_writable((HgObject *)nmatrix)) {
				_hg_operator_set_error(vm, op, VM_e_invalidaccess);
				break;
			}
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			len = hg_array_length(matrix);
			if (len != 6) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			ny = hg_stack_index(ostack, 1);
			offset = 2;
		} else {
			ny = nmatrix;
			nmatrix = NULL;
			offset = 1;
		}
		nx = hg_stack_index(ostack, offset);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (nmatrix != NULL) {
			mtx = hg_matrix_translate(pool, dx, dy);
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 0), mtx->xx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 1), mtx->yx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 2), mtx->xy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 3), mtx->yy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 4), mtx->x0);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 5), mtx->y0);

			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, nmatrix);
		} else {
			retval = hg_graphics_matrix_translate(hg_vm_get_graphics(vm),
							      dx, dy);
			if (!retval) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_pop(ostack);
			hg_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (truncate)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			/* nothing to do here */
			retval = TRUE;
		} else if (HG_IS_VALUE_REAL (node)) {
			gdouble result = floor(HG_VALUE_GET_REAL (node));

			HG_VALUE_MAKE_REAL (pool, node, result);
			if (node == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_pop(ostack);
			retval = hg_stack_push(ostack, node);
			/* it must be true */
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (type)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		switch (HG_VALUE_GET_VALUE_TYPE (node)) {
		    case HG_TYPE_VALUE_BOOLEAN:
			    node = hg_vm_get_name_node(vm, "booleantype");
			    break;
		    case HG_TYPE_VALUE_INTEGER:
			    node = hg_vm_get_name_node(vm, "integertype");
			    break;
		    case HG_TYPE_VALUE_REAL:
			    node = hg_vm_get_name_node(vm, "realtype");
			    break;
		    case HG_TYPE_VALUE_NAME:
			    node = hg_vm_get_name_node(vm, "nametype");
			    break;
		    case HG_TYPE_VALUE_ARRAY:
			    node = hg_vm_get_name_node(vm, "arraytype");
			    break;
		    case HG_TYPE_VALUE_STRING:
			    node = hg_vm_get_name_node(vm, "stringtype");
			    break;
		    case HG_TYPE_VALUE_DICT:
			    node = hg_vm_get_name_node(vm, "dicttype");
			    break;
		    case HG_TYPE_VALUE_NULL:
			    node = hg_vm_get_name_node(vm, "nulltype");
			    break;
		    case HG_TYPE_VALUE_POINTER:
			    node = hg_vm_get_name_node(vm, "operatortype");
			    break;
		    case HG_TYPE_VALUE_MARK:
			    node = hg_vm_get_name_node(vm, "marktype");
			    break;
		    case HG_TYPE_VALUE_FILE:
			    node = hg_vm_get_name_node(vm, "filetype");
			    break;
		    case HG_TYPE_VALUE_SNAPSHOT:
			    node = hg_vm_get_name_node(vm, "savetype");
			    break;
		    case HG_TYPE_VALUE_END:
		    default:
			    g_warning("[BUG] Unknown node type %d was given.",
				      HG_VALUE_GET_VALUE_TYPE (node));
			    _hg_operator_set_error(vm, op, VM_e_typecheck);

			    return retval;
		}
		hg_object_executable((HgObject *)node);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (usertime)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (hg_vm_get_current_pool(vm),
			       node, hg_vm_get_current_time(vm));
	if (node == NULL) {
		_hg_operator_set_error(vm, op, VM_e_VMerror);
	} else {
		retval = hg_stack_push(ostack, node);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (vmstatus)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *n1, *n2, *n3;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 level, used, maximum;

	while (1) {
		level = hg_vm_get_save_level(vm);
		used = hg_mem_pool_get_used_heap_size(pool);
		maximum = hg_mem_pool_get_free_heap_size(pool);
		HG_VALUE_MAKE_INTEGER (pool, n1, level);
		HG_VALUE_MAKE_INTEGER (pool, n2, used);
		HG_VALUE_MAKE_INTEGER (pool, n3, maximum);
		if (n1 == NULL ||
		    n2 == NULL ||
		    n3 == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		hg_stack_push(ostack, n1);
		hg_stack_push(ostack, n2);
		retval = hg_stack_push(ostack, n3);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (wcheck)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (node) ||
		    HG_IS_VALUE_FILE (node) ||
		    HG_IS_VALUE_STRING (node)) {
			gboolean result = hg_object_is_writable((HgObject *)node);

			HG_VALUE_MAKE_BOOLEAN (pool, node, result);
		} else if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);
			gboolean result = hg_object_is_writable((HgObject *)dict);

			HG_VALUE_MAKE_BOOLEAN (pool, node, result);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (where)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	guint depth = hg_stack_depth(ostack);
	guint ddepth = hg_stack_depth(dstack), i;
	HgValueNode *node, *ndict;
	HgDict *dict = NULL;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		for (i = 0; i < ddepth; i++) {
			ndict = hg_stack_index(dstack, i);
			if (!HG_IS_VALUE_DICT (ndict)) {
				g_warning("[BUG] Invalid dictionary was in the dictionary stack.");
				return retval;
			}
			dict = HG_VALUE_GET_DICT (ndict);
			if (hg_dict_lookup(dict, node) != NULL)
				break;
		}
		hg_stack_pop(ostack);
		if (i == ddepth || dict == NULL) {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
			retval = hg_stack_push(ostack, node);
			/* it must be true */
		} else {
			HG_VALUE_MAKE_DICT (node, dict);
			hg_stack_push(ostack, node);
			HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
			retval = hg_stack_push(ostack, node);
			if (!retval)
				_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (widthshow);

DEFUNC_OP (write)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	gint32 i;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_INTEGER (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n1)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		i = HG_VALUE_GET_INTEGER (n2);
		hg_file_object_printf(file, "%c", i % 256);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (writehexstring)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *s;
	guint length, i;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		s = HG_VALUE_GET_STRING (n2);
		length = hg_string_maxlength(s);
		for (i = 0; i < length; i++) {
			hg_file_object_printf(file, "%02X", hg_string_index(s, i) & 0xff);
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (writestring)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *hs;

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		hs = HG_VALUE_GET_STRING (n2);
		hg_file_object_write(file,
				     hg_string_get_string(hs),
				     sizeof (gchar),
				     hg_string_maxlength(hs));
		if (hg_file_object_has_error(file)) {
			_hg_operator_set_error_from_file(vm, op, file);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (xcheck)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (node) ||
		    HG_IS_VALUE_FILE (node) ||
		    HG_IS_VALUE_STRING (node) ||
		    HG_IS_VALUE_NAME (node)) {
			gboolean result = hg_object_is_executable((HgObject *)node);

			HG_VALUE_MAKE_BOOLEAN (pool, node, result);
		} else if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);
			gboolean result = hg_object_is_executable((HgObject *)dict);

			HG_VALUE_MAKE_BOOLEAN (pool, node, result);
		} else {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
		}
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (xor)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = hg_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (HG_IS_VALUE_BOOLEAN (n1) &&
		    HG_IS_VALUE_BOOLEAN (n2)) {
			gboolean result = HG_VALUE_GET_BOOLEAN (n1) ^ HG_VALUE_GET_BOOLEAN (n2);

			HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
		} else if (HG_IS_VALUE_INTEGER (n1) &&
			   HG_IS_VALUE_INTEGER (n2)) {
			gint32 result = HG_VALUE_GET_INTEGER (n1) ^ HG_VALUE_GET_INTEGER (n2);

			HG_VALUE_MAKE_INTEGER (pool, n1, result);
			if (n1 == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

/* level 2 */
DEFUNC_UNIMPLEMENTED_OP (arct);
DEFUNC_UNIMPLEMENTED_OP (colorimage);
DEFUNC_UNIMPLEMENTED_OP (cshow);
DEFUNC_UNIMPLEMENTED_OP (currentblackgeneration);
DEFUNC_UNIMPLEMENTED_OP (currentcacheparams);
DEFUNC_UNIMPLEMENTED_OP (currentcmykcolor);
DEFUNC_UNIMPLEMENTED_OP (currentcolor);
DEFUNC_UNIMPLEMENTED_OP (currentcolorrendering);
DEFUNC_UNIMPLEMENTED_OP (currentcolorscreen);
DEFUNC_UNIMPLEMENTED_OP (currentcolorspace);
DEFUNC_UNIMPLEMENTED_OP (currentcolortransfer);
DEFUNC_UNIMPLEMENTED_OP (currentdevparams);
DEFUNC_UNIMPLEMENTED_OP (currentgstate);
DEFUNC_UNIMPLEMENTED_OP (currenthalftone);
DEFUNC_UNIMPLEMENTED_OP (currentobjectformat);
DEFUNC_UNIMPLEMENTED_OP (currentoverprint);
DEFUNC_UNIMPLEMENTED_OP (currentpacking);
DEFUNC_UNIMPLEMENTED_OP (currentpagedevice);
DEFUNC_UNIMPLEMENTED_OP (currentshared);
DEFUNC_UNIMPLEMENTED_OP (currentstrokeadjust);
DEFUNC_UNIMPLEMENTED_OP (currentsystemparams);
DEFUNC_UNIMPLEMENTED_OP (currentundercolorremoval);
DEFUNC_UNIMPLEMENTED_OP (currentuserparams);
DEFUNC_UNIMPLEMENTED_OP (defineresource);
DEFUNC_UNIMPLEMENTED_OP (defineuserobject);
DEFUNC_UNIMPLEMENTED_OP (deletefile);
DEFUNC_UNIMPLEMENTED_OP (execform);
DEFUNC_UNIMPLEMENTED_OP (execuserobject);
DEFUNC_UNIMPLEMENTED_OP (filenameforall);
DEFUNC_UNIMPLEMENTED_OP (fileposition);
DEFUNC_UNIMPLEMENTED_OP (filter);
DEFUNC_UNIMPLEMENTED_OP (findencoding);
DEFUNC_UNIMPLEMENTED_OP (findresource);
DEFUNC_UNIMPLEMENTED_OP (gcheck);
DEFUNC_UNIMPLEMENTED_OP (globalfontdirectory);
DEFUNC_UNIMPLEMENTED_OP (glyphshow);
DEFUNC_UNIMPLEMENTED_OP (gstate);
DEFUNC_UNIMPLEMENTED_OP (ineofill);
DEFUNC_UNIMPLEMENTED_OP (infill);
DEFUNC_UNIMPLEMENTED_OP (instroke);
DEFUNC_UNIMPLEMENTED_OP (inueofill);
DEFUNC_UNIMPLEMENTED_OP (inufill);
DEFUNC_UNIMPLEMENTED_OP (inustroke);
DEFUNC_UNIMPLEMENTED_OP (isolatin1encoding);
DEFUNC_UNIMPLEMENTED_OP (languagelevel);
DEFUNC_UNIMPLEMENTED_OP (makepattern);
DEFUNC_UNIMPLEMENTED_OP (packedarray);
DEFUNC_UNIMPLEMENTED_OP (printobject);
DEFUNC_UNIMPLEMENTED_OP (realtime);
DEFUNC_UNIMPLEMENTED_OP (rectclip);
DEFUNC_UNIMPLEMENTED_OP (rectfill);
DEFUNC_UNIMPLEMENTED_OP (rectstroke);
DEFUNC_UNIMPLEMENTED_OP (renamefile);
DEFUNC_UNIMPLEMENTED_OP (resourceforall);
DEFUNC_UNIMPLEMENTED_OP (resourcestatus);
DEFUNC_UNIMPLEMENTED_OP (rootfont);
DEFUNC_UNIMPLEMENTED_OP (scheck);
DEFUNC_UNIMPLEMENTED_OP (selectfont);
DEFUNC_UNIMPLEMENTED_OP (serialnumber);
DEFUNC_UNIMPLEMENTED_OP (setbbox);
DEFUNC_UNIMPLEMENTED_OP (setblackgeneration);
DEFUNC_UNIMPLEMENTED_OP (setcachedevice2);
DEFUNC_UNIMPLEMENTED_OP (setcacheparams);
DEFUNC_UNIMPLEMENTED_OP (setcmykcolor);
DEFUNC_UNIMPLEMENTED_OP (setcolor);
DEFUNC_UNIMPLEMENTED_OP (setcolorrendering);
DEFUNC_UNIMPLEMENTED_OP (setcolorscreen);
DEFUNC_UNIMPLEMENTED_OP (setcolorspace);
DEFUNC_UNIMPLEMENTED_OP (setcolortransfer);
DEFUNC_UNIMPLEMENTED_OP (setdevparams);
DEFUNC_UNIMPLEMENTED_OP (setfileposition);
DEFUNC_UNIMPLEMENTED_OP (setgstate);
DEFUNC_UNIMPLEMENTED_OP (sethalftone);
DEFUNC_UNIMPLEMENTED_OP (setobjectformat);
DEFUNC_UNIMPLEMENTED_OP (setoverprint);
DEFUNC_UNIMPLEMENTED_OP (setpacking);
DEFUNC_UNIMPLEMENTED_OP (setpagedevice);
DEFUNC_UNIMPLEMENTED_OP (setpattern);
DEFUNC_UNIMPLEMENTED_OP (setshared);
DEFUNC_UNIMPLEMENTED_OP (setstrokeadjust);
DEFUNC_UNIMPLEMENTED_OP (setsystemparams);
DEFUNC_UNIMPLEMENTED_OP (setucacheparams);
DEFUNC_UNIMPLEMENTED_OP (setundercoloremoval);
DEFUNC_UNIMPLEMENTED_OP (setuserparams);
DEFUNC_UNIMPLEMENTED_OP (setvmthreshold);
DEFUNC_UNIMPLEMENTED_OP (shareddict);
DEFUNC_UNIMPLEMENTED_OP (sharedfontdirectory);
DEFUNC_UNIMPLEMENTED_OP (startjob);
DEFUNC_UNIMPLEMENTED_OP (uappend);
DEFUNC_UNIMPLEMENTED_OP (ucache);
DEFUNC_UNIMPLEMENTED_OP (ucachestatus);
DEFUNC_UNIMPLEMENTED_OP (ueofill);
DEFUNC_UNIMPLEMENTED_OP (ufill);
DEFUNC_UNIMPLEMENTED_OP (undef);
DEFUNC_UNIMPLEMENTED_OP (undefineresource);
DEFUNC_UNIMPLEMENTED_OP (undefineuserobject);
DEFUNC_UNIMPLEMENTED_OP (upath);
DEFUNC_UNIMPLEMENTED_OP (userobjects);
DEFUNC_UNIMPLEMENTED_OP (ustroke);
DEFUNC_UNIMPLEMENTED_OP (ustrokepath);
DEFUNC_UNIMPLEMENTED_OP (vmreclaim);
DEFUNC_UNIMPLEMENTED_OP (writeobject);
DEFUNC_UNIMPLEMENTED_OP (xshow);
DEFUNC_UNIMPLEMENTED_OP (xyshow);
DEFUNC_UNIMPLEMENTED_OP (yshow);

/* level 3 */
DEFUNC_UNIMPLEMENTED_OP (addglyph);
DEFUNC_UNIMPLEMENTED_OP (beginbfchar);
DEFUNC_UNIMPLEMENTED_OP (beginbfrange);
DEFUNC_UNIMPLEMENTED_OP (begincidchar);
DEFUNC_UNIMPLEMENTED_OP (begincidrange);
DEFUNC_UNIMPLEMENTED_OP (begincmap);
DEFUNC_UNIMPLEMENTED_OP (begincodespacerange);
DEFUNC_UNIMPLEMENTED_OP (beginnotdefchar);
DEFUNC_UNIMPLEMENTED_OP (beginnotdefrange);
DEFUNC_UNIMPLEMENTED_OP (beginrearrangedfont);
DEFUNC_UNIMPLEMENTED_OP (beginusematrix);
DEFUNC_UNIMPLEMENTED_OP (cliprestore);
DEFUNC_UNIMPLEMENTED_OP (clipsave);
DEFUNC_UNIMPLEMENTED_OP (composefont);
DEFUNC_UNIMPLEMENTED_OP (currentsmoothness);
DEFUNC_UNIMPLEMENTED_OP (currentrapparams);
DEFUNC_UNIMPLEMENTED_OP (endbfchar);
DEFUNC_UNIMPLEMENTED_OP (endbfrange);
DEFUNC_UNIMPLEMENTED_OP (endcidchar);
DEFUNC_UNIMPLEMENTED_OP (endcidrange);
DEFUNC_UNIMPLEMENTED_OP (endcmap);
DEFUNC_UNIMPLEMENTED_OP (endcodespacerange);
DEFUNC_UNIMPLEMENTED_OP (endnotdefchar);
DEFUNC_UNIMPLEMENTED_OP (endnotdefrange);
DEFUNC_UNIMPLEMENTED_OP (endrearrangedfont);
DEFUNC_UNIMPLEMENTED_OP (endusematrix);
DEFUNC_UNIMPLEMENTED_OP (findcolorrendering);
DEFUNC_UNIMPLEMENTED_OP (gethalftonename);
DEFUNC_UNIMPLEMENTED_OP (getpagedevicename);
DEFUNC_UNIMPLEMENTED_OP (getsubstitutecrd);
DEFUNC_UNIMPLEMENTED_OP (removeall);
DEFUNC_UNIMPLEMENTED_OP (removeglyphs);
DEFUNC_UNIMPLEMENTED_OP (setsmoothness);
DEFUNC_UNIMPLEMENTED_OP (settrapparams);
DEFUNC_UNIMPLEMENTED_OP (settrapzone);
DEFUNC_UNIMPLEMENTED_OP (shfill);
DEFUNC_UNIMPLEMENTED_OP (startdata);
DEFUNC_UNIMPLEMENTED_OP (usecmap);
DEFUNC_UNIMPLEMENTED_OP (usefont);

/* hieroglyph specific operators */
DEFUNC_OP (private_hg_abort)
G_STMT_START
{
	HgFileObject *file = hg_vm_get_io(vm, VM_IO_STDERR);
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	HgStack *dstack = hg_vm_get_dstack(vm);
	HgMemPool *local_pool, *global_pool;
	gboolean flag = hg_vm_is_global_pool_used(vm);
	gsize free_size, used_size;

	hg_vm_use_global_pool(vm, TRUE);
	global_pool = hg_vm_get_current_pool(vm);
	hg_vm_use_global_pool(vm, FALSE);
	local_pool = hg_vm_get_current_pool(vm);
	hg_vm_use_global_pool(vm, flag);
	/* allow resizing to avoid /VMerror during dumping */
	hg_mem_pool_allow_resize(global_pool, TRUE);
	hg_mem_pool_allow_resize(local_pool, TRUE);

	hg_file_object_printf(file, "\nOperand stack:\n");
	hg_stack_dump(ostack, file);
	hg_file_object_printf(file, "\nExecution stack:\n");
	hg_stack_dump(estack, file);
	hg_file_object_printf(file, "\nDictionary stack:\n");
	hg_stack_dump(dstack, file);
	hg_file_object_printf(file, "\nVM Status:\n");
	free_size = hg_mem_pool_get_free_heap_size(global_pool);
	used_size = hg_mem_pool_get_used_heap_size(global_pool);
	hg_file_object_printf(file, "  Total (Global): %" G_GSIZE_FORMAT "\n", free_size + used_size);
	hg_file_object_printf(file, "  Free  (Global): %" G_GSIZE_FORMAT "\n", free_size);
	hg_file_object_printf(file, "  Used  (Global): %" G_GSIZE_FORMAT "\n", used_size);
	free_size = hg_mem_pool_get_free_heap_size(local_pool);
	used_size = hg_mem_pool_get_used_heap_size(local_pool);
	hg_file_object_printf(file, "  Total (Local): %" G_GSIZE_FORMAT "\n", free_size + used_size);
	hg_file_object_printf(file, "  Free  (Local): %" G_GSIZE_FORMAT "\n", free_size);
	hg_file_object_printf(file, "  Used  (Local): %" G_GSIZE_FORMAT "\n", used_size);
	abort();
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_clearerror)
G_STMT_START
{
	hg_vm_reset_error(vm);
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_currentglobal)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *node;

	HG_VALUE_MAKE_BOOLEAN (hg_vm_get_current_pool(vm), node, hg_vm_is_global_pool_used(vm));
	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_execn)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *nn, *node, *copy_node, *self;
	gint32 i, n;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		nn = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (nn)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		n = HG_VALUE_GET_INTEGER (nn);
		if (depth < (n + 1)) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		self = hg_stack_pop(estack);
		for (i = 0; i < n; i++) {
			node = hg_stack_index(ostack, i + 1);
			copy_node = hg_object_copy((HgObject *)node);
			if (copy_node == NULL) {
				_hg_operator_set_error(vm, op, VM_e_VMerror);
				break;
			}
			hg_stack_push(estack, copy_node);
		}
		if (!hg_vm_has_error(vm)) {
			hg_stack_pop(ostack);
			for (i = 0; i < n; i++)
				hg_stack_pop(ostack);
		}
		retval = hg_stack_push(estack, self);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_findlibfile)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node, *nresult;
	HgString *s;
	gchar *filename;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result = FALSE;

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
		s = HG_VALUE_GET_STRING (node);
		filename = hg_vm_find_libfile(vm, hg_string_get_string(s));
		hg_stack_pop(ostack);
		if (filename) {
			result = TRUE;
			s = hg_string_new(pool, -1);
			hg_string_append(s, filename, -1);
			hg_string_fix_string_size(s);
			HG_VALUE_MAKE_STRING(node, s);
			hg_stack_push(ostack, node);
		}
		HG_VALUE_MAKE_BOOLEAN (pool, nresult, result);
		retval = hg_stack_push(ostack, nresult);
		if (!retval)
			_hg_operator_set_error(vm, op, VM_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_hgrevision)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 revision = 0;

	sscanf(__hg_rcsid, "$Rev: %d $", &revision);
	HG_VALUE_MAKE_INTEGER (pool, node, revision);
	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_initplugins)
G_STMT_START
{
	hg_vm_load_plugins_all(vm);
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_odef)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgStack *estack = hg_vm_get_estack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n1, *n2, *n, *nself;
	HgArray *array;
	HgDict *systemdict = hg_vm_get_dict_systemdict(vm);

	while (1) {
		if (depth < 2) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n2 = hg_stack_index(ostack, 0);
		n1 = hg_stack_index(ostack, 1);
		if (!HG_IS_VALUE_NAME (n1) ||
		    !HG_IS_VALUE_ARRAY (n2)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		n = hg_dict_lookup_with_string(systemdict, "def");
		if (n == NULL) {
			_hg_operator_set_error(vm, op, VM_e_undefined);
			break;
		}
		if (hg_object_is_executable((HgObject *)n2)) {
			array = HG_VALUE_GET_ARRAY (n2);
			hg_array_set_name(array, HG_VALUE_GET_NAME (n1));
			hg_object_executeonly((HgObject *)n2);
		}
		nself = hg_stack_pop(estack);
		hg_stack_push(estack, n);
		retval = hg_stack_push(estack, nself);
		if (!retval) {
			_hg_operator_set_error(vm, op, VM_e_execstackoverflow);
			break;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_product)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgString *hs = hg_string_new(pool, -1);

	hg_string_append(hs, "Hieroglyph PostScript Interpreter", -1);
	hg_string_fix_string_size(hs);
	HG_VALUE_MAKE_STRING (node, hs);
	hg_object_unwritable((HgObject *)node);
	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

/* almost code is shared to _hg_operator_op_put */
DEFUNC_OP (private_hg_forceput)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack), len;
	gint32 index;
	HgValueNode *n1, *n2, *n3;
	HgMemObject *obj;

	while (1) {
		if (depth < 3) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		n3 = hg_stack_index(ostack, 0);
		n2 = hg_stack_index(ostack, 1);
		n1 = hg_stack_index(ostack, 2);
		if (HG_IS_VALUE_ARRAY (n1)) {
			HgArray *array = HG_VALUE_GET_ARRAY (n1);

			if (!HG_IS_VALUE_INTEGER (n2)) {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			len = hg_array_length(array);
			index = HG_VALUE_GET_INTEGER (n2);
			if (index > len || index > 65535) {
				_hg_operator_set_error(vm, op, VM_e_rangecheck);
				break;
			}
			retval = hg_array_replace_forcibly(array, n3, index, TRUE);
		} else if (HG_IS_VALUE_DICT (n1)) {
			HgDict *dict = HG_VALUE_GET_DICT (n1);

			hg_mem_get_object__inline(dict, obj);
			retval = hg_dict_insert_forcibly(obj->pool, dict, n2, n3, TRUE);
		} else if (HG_IS_VALUE_STRING (n1)) {
			HgString *string = HG_VALUE_GET_STRING (n1);
			gint32 c;

			if (!HG_IS_VALUE_INTEGER (n2) || !HG_IS_VALUE_INTEGER (n3)) {
				_hg_operator_set_error(vm, op, VM_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			c = HG_VALUE_GET_INTEGER (n3);
			retval = hg_string_insert_c(string, c, index);
		} else {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		hg_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_revision)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	HgValueNode *node;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gint32 major, minor, release;

	sscanf(HIEROGLYPH_VERSION, "%d.%d.%d", &major, &minor, &release);
	HG_VALUE_MAKE_INTEGER (pool, node, major * 1000000 + minor * 1000 + release);
	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_setglobal)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		node = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_BOOLEAN (node)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		hg_vm_use_global_pool(vm, HG_VALUE_GET_BOOLEAN (node));
		hg_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_startjobserver)
G_STMT_START
{
	HgValueNode *key, *val;

	key = hg_vm_get_name_node(vm, "%initialized");
	val = hg_dict_lookup_with_string(hg_vm_get_dict_systemdict(vm),
					 "true");
	hg_dict_insert(hg_vm_get_current_pool(vm),
		       hg_vm_get_dict_statusdict(vm),
		       key, val);

	/* set read-only attribute to systemdict */
	hg_object_unwritable((HgObject *)hg_vm_get_dict_systemdict(vm));
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

#undef DEFUNC_UNIMPLEMENTED_OP
#undef DEFUNC_OP
#undef DEFUNC_OP_END

/*
 * Private Functions
 */
static void
_hg_operator_real_set_flags(gpointer data,
				  guint    flags)
{
	HgOperator *op = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(op->name, obj);
	if (obj == NULL) {
		g_warning("Invalid object %p to be marked: HgOperator->name", op->name);
	} else {
#ifdef DEBUG_GC
		G_STMT_START {
			if ((flags & HG_FL_MARK) != 0) {
				if (!hg_mem_is_flags__inline(obj, flags)) {
					hg_debug_print_gc_state(HG_DEBUG_GC_MARK, HG_TYPE_VALUE_POINTER, NULL, data, op->name);
				} else {
					hg_debug_print_gc_state(HG_DEBUG_GC_ALREADYMARK, HG_TYPE_VALUE_POINTER, NULL, data, op->name);
				}
			}
		} G_STMT_END;
#endif /* DEBUG_GC */
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
}

static void
_hg_operator_real_relocate(gpointer           data,
				 HgMemRelocateInfo *info)
{
	HgOperator *op = data;

	if ((gsize)op->name >= info->start &&
	    (gsize)op->name <= info->end) {
		op->name = (gchar *)((gsize)op->name + info->diff);
	}
}

static gpointer
_hg_operator_real_to_string(gpointer data)
{
	HgOperator *op = data;
	HgMemObject *obj;
	HgString *retval;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_string_new(obj->pool, strlen(op->name) + 4);
	hg_string_append(retval, "--", 2);
	hg_string_append(retval, op->name, -1);
	hg_string_append(retval, "--", 2);

	return retval;
}

static void
hg_operator_level1_init(HgVM      *vm,
			HgMemPool *pool,
			HgDict    *dict)
{
	BUILD_OP_ (vm, pool, dict, %arraytomark, private_arraytomark);
	BUILD_OP_ (vm, pool, dict, %dicttomark, private_dicttomark);
	BUILD_OP_ (vm, pool, dict, %for_pos_int_continue, private_for_pos_int_continue);
	BUILD_OP_ (vm, pool, dict, %for_pos_real_continue, private_for_pos_real_continue);
	BUILD_OP_ (vm, pool, dict, %forall_array_continue, private_forall_array_continue);
	BUILD_OP_ (vm, pool, dict, %forall_dict_continue, private_forall_dict_continue);
	BUILD_OP_ (vm, pool, dict, %forall_string_continue, private_forall_string_continue);
	BUILD_OP_ (vm, pool, dict, %loop_continue, private_loop_continue);
	BUILD_OP_ (vm, pool, dict, %repeat_continue, private_repeat_continue);
	BUILD_OP_ (vm, pool, dict, %stopped_continue, private_stopped_continue);
	BUILD_OP_ (vm, pool, dict, .findfont, private_findfont);
	BUILD_OP_ (vm, pool, dict, .definefont, private_definefont);
	BUILD_OP_ (vm, pool, dict, .stringcvs, private_stringcvs);
	BUILD_OP_ (vm, pool, dict, .undefinefont, private_undefinefont);
	BUILD_OP_ (vm, pool, dict, .write==only, private_write_eqeq_only);
	BUILD_OP (vm, pool, dict, abs, abs);
	BUILD_OP (vm, pool, dict, add, add);
	BUILD_OP (vm, pool, dict, aload, aload);
	BUILD_OP (vm, pool, dict, anchorsearch, anchorsearch);
	BUILD_OP (vm, pool, dict, and, and);
	BUILD_OP (vm, pool, dict, arc, arc);
	BUILD_OP (vm, pool, dict, arcn, arcn);
	BUILD_OP (vm, pool, dict, arcto, arcto);
	BUILD_OP (vm, pool, dict, array, array);
	BUILD_OP (vm, pool, dict, ashow, ashow);
	BUILD_OP (vm, pool, dict, astore, astore);
	BUILD_OP (vm, pool, dict, atan, atan);
	BUILD_OP (vm, pool, dict, awidthshow, awidthshow);
	BUILD_OP (vm, pool, dict, begin, begin);
	BUILD_OP (vm, pool, dict, bind, bind);
	BUILD_OP (vm, pool, dict, bitshift, bitshift);
	BUILD_OP (vm, pool, dict, bytesavailable, bytesavailable);
	BUILD_OP (vm, pool, dict, cachestatus, cachestatus);
	BUILD_OP (vm, pool, dict, ceiling, ceiling);
	BUILD_OP (vm, pool, dict, charpath, charpath);
	BUILD_OP (vm, pool, dict, clear, clear);
	BUILD_OP (vm, pool, dict, cleardictstack, cleardictstack);
	BUILD_OP (vm, pool, dict, cleartomark, cleartomark);
	BUILD_OP (vm, pool, dict, clip, clip);
	BUILD_OP (vm, pool, dict, clippath, clippath);
	BUILD_OP (vm, pool, dict, closefile, closefile);
	BUILD_OP (vm, pool, dict, closepath, closepath);
	BUILD_OP (vm, pool, dict, concat, concat);
	BUILD_OP (vm, pool, dict, concatmatrix, concatmatrix);
	BUILD_OP (vm, pool, dict, copy, copy);
	BUILD_OP (vm, pool, dict, copypage, copypage);
	BUILD_OP (vm, pool, dict, cos, cos);
	BUILD_OP (vm, pool, dict, count, count);
	BUILD_OP (vm, pool, dict, countdictstack, countdictstack);
	BUILD_OP (vm, pool, dict, countexecstack, countexecstack);
	BUILD_OP (vm, pool, dict, counttomark, counttomark);
	BUILD_OP (vm, pool, dict, currentdash, currentdash);
	BUILD_OP (vm, pool, dict, currentdict, currentdict);
	BUILD_OP (vm, pool, dict, currentfile, currentfile);
	BUILD_OP (vm, pool, dict, currentflat, currentflat);
	BUILD_OP (vm, pool, dict, currentfont, currentfont);
	BUILD_OP (vm, pool, dict, currentgray, currentgray);
	BUILD_OP (vm, pool, dict, currenthsbcolor, currenthsbcolor);
	BUILD_OP (vm, pool, dict, currentlinecap, currentlinecap);
	BUILD_OP (vm, pool, dict, currentlinejoin, currentlinejoin);
	BUILD_OP (vm, pool, dict, currentlinewidth, currentlinewidth);
	BUILD_OP (vm, pool, dict, currentmatrix, currentmatrix);
	BUILD_OP (vm, pool, dict, currentmiterlimit, currentmiterlimit);
	BUILD_OP (vm, pool, dict, currentpoint, currentpoint);
	BUILD_OP (vm, pool, dict, currentrgbcolor, currentrgbcolor);
	BUILD_OP (vm, pool, dict, currentscreen, currentscreen);
	BUILD_OP (vm, pool, dict, currenttransfer, currenttransfer);
	BUILD_OP (vm, pool, dict, curveto, curveto);
	BUILD_OP (vm, pool, dict, cvi, cvi);
	BUILD_OP (vm, pool, dict, cvlit, cvlit);
	BUILD_OP (vm, pool, dict, cvn, cvn);
	BUILD_OP (vm, pool, dict, cvr, cvr);
	BUILD_OP (vm, pool, dict, cvrs, cvrs);
	BUILD_OP (vm, pool, dict, cvx, cvx);
	BUILD_OP (vm, pool, dict, def, def);
	BUILD_OP (vm, pool, dict, defaultmatrix, defaultmatrix);
	BUILD_OP (vm, pool, dict, dict, dict);
	BUILD_OP (vm, pool, dict, dictstack, dictstack);
	BUILD_OP (vm, pool, dict, div, div);
	BUILD_OP (vm, pool, dict, dtransform, dtransform);
	BUILD_OP (vm, pool, dict, dup, dup);
	BUILD_OP (vm, pool, dict, echo, echo);
	BUILD_OP_ (vm, pool, dict, eexec, eexec);
	BUILD_OP (vm, pool, dict, end, end);
	BUILD_OP (vm, pool, dict, eoclip, eoclip);
	BUILD_OP (vm, pool, dict, eofill, eofill);
	BUILD_OP (vm, pool, dict, eq, eq);
	BUILD_OP (vm, pool, dict, erasepage, erasepage);
	BUILD_OP (vm, pool, dict, exch, exch);
	BUILD_OP (vm, pool, dict, exec, exec);
	BUILD_OP (vm, pool, dict, execstack, execstack);
	BUILD_OP (vm, pool, dict, executeonly, executeonly);
	BUILD_OP (vm, pool, dict, exit, exit);
	BUILD_OP (vm, pool, dict, exp, exp);
	BUILD_OP (vm, pool, dict, file, file);
	BUILD_OP (vm, pool, dict, fill, fill);
	BUILD_OP (vm, pool, dict, flattenpath, flattenpath);
	BUILD_OP (vm, pool, dict, flush, flush);
	BUILD_OP (vm, pool, dict, flushfile, flushfile);
	BUILD_OP (vm, pool, dict, FontDirectory, fontdirectory);
	BUILD_OP (vm, pool, dict, for, for);
	BUILD_OP (vm, pool, dict, forall, forall);
	BUILD_OP (vm, pool, dict, ge, ge);
	BUILD_OP (vm, pool, dict, get, get);
	BUILD_OP (vm, pool, dict, getinterval, getinterval);
	BUILD_OP (vm, pool, dict, grestore, grestore);
	BUILD_OP (vm, pool, dict, grestoreall, grestoreall);
	BUILD_OP (vm, pool, dict, gsave, gsave);
	BUILD_OP (vm, pool, dict, gt, gt);
	BUILD_OP (vm, pool, dict, identmatrix, identmatrix);
	BUILD_OP (vm, pool, dict, idiv, idiv);
	BUILD_OP (vm, pool, dict, idtransform, idtransform);
	BUILD_OP (vm, pool, dict, if, if);
	BUILD_OP (vm, pool, dict, ifelse, ifelse);
	BUILD_OP (vm, pool, dict, image, image);
	BUILD_OP (vm, pool, dict, imagemask, imagemask);
	BUILD_OP (vm, pool, dict, index, index);
	BUILD_OP (vm, pool, dict, initclip, initclip);
	BUILD_OP (vm, pool, dict, initgraphics, initgraphics);
	BUILD_OP (vm, pool, dict, initmatrix, initmatrix);
	BUILD_OP_ (vm, pool, dict, internaldict, internaldict);
	BUILD_OP (vm, pool, dict, invertmatrix, invertmatrix);
	BUILD_OP (vm, pool, dict, itransform, itransform);
	BUILD_OP (vm, pool, dict, known, known);
	BUILD_OP (vm, pool, dict, kshow, kshow);
	BUILD_OP (vm, pool, dict, le, le);
	BUILD_OP (vm, pool, dict, length, length);
	BUILD_OP (vm, pool, dict, lineto, lineto);
	BUILD_OP (vm, pool, dict, ln, ln);
	BUILD_OP (vm, pool, dict, load, load);
	BUILD_OP (vm, pool, dict, log, log);
	BUILD_OP (vm, pool, dict, loop, loop);
	BUILD_OP (vm, pool, dict, lt, lt);
	BUILD_OP (vm, pool, dict, makefont, makefont);
	BUILD_OP (vm, pool, dict, maxlength, maxlength);
	BUILD_OP (vm, pool, dict, mod, mod);
	BUILD_OP (vm, pool, dict, moveto, moveto);
	BUILD_OP (vm, pool, dict, mul, mul);
	BUILD_OP (vm, pool, dict, ne, ne);
	BUILD_OP (vm, pool, dict, neg, neg);
	BUILD_OP (vm, pool, dict, newpath, newpath);
	BUILD_OP (vm, pool, dict, noaccess, noaccess);
	BUILD_OP (vm, pool, dict, not, not);
	BUILD_OP (vm, pool, dict, nulldevice, nulldevice);
	BUILD_OP (vm, pool, dict, or, or);
	BUILD_OP (vm, pool, dict, pathbbox, pathbbox);
	BUILD_OP (vm, pool, dict, pathforall, pathforall);
	BUILD_OP (vm, pool, dict, pop, pop);
	BUILD_OP (vm, pool, dict, print, print);
	BUILD_OP (vm, pool, dict, put, put);
	BUILD_OP (vm, pool, dict, quit, quit);
	BUILD_OP (vm, pool, dict, rand, rand);
	BUILD_OP (vm, pool, dict, rcheck, rcheck);
	BUILD_OP (vm, pool, dict, rcurveto, rcurveto);
	BUILD_OP (vm, pool, dict, read, read);
	BUILD_OP (vm, pool, dict, readhexstring, readhexstring);
	BUILD_OP (vm, pool, dict, readline, readline);
	BUILD_OP (vm, pool, dict, readonly, readonly);
	BUILD_OP (vm, pool, dict, readstring, readstring);
	BUILD_OP (vm, pool, dict, repeat, repeat);
	BUILD_OP (vm, pool, dict, resetfile, resetfile);
	BUILD_OP (vm, pool, dict, restore, restore);
	BUILD_OP (vm, pool, dict, reversepath, reversepath);
	BUILD_OP (vm, pool, dict, rlineto, rlineto);
	BUILD_OP (vm, pool, dict, rmoveto, rmoveto);
	BUILD_OP (vm, pool, dict, roll, roll);
	BUILD_OP (vm, pool, dict, rotate, rotate);
	BUILD_OP (vm, pool, dict, round, round);
	BUILD_OP (vm, pool, dict, rrand, rrand);
	BUILD_OP (vm, pool, dict, save, save);
	BUILD_OP (vm, pool, dict, scale, scale);
	BUILD_OP (vm, pool, dict, scalefont, scalefont);
	BUILD_OP (vm, pool, dict, search, search);
	BUILD_OP (vm, pool, dict, setcachedevice, setcachedevice);
	BUILD_OP (vm, pool, dict, setcachelimit, setcachelimit);
	BUILD_OP (vm, pool, dict, setcharwidth, setcharwidth);
	BUILD_OP (vm, pool, dict, setdash, setdash);
	BUILD_OP (vm, pool, dict, setflat, setflat);
	BUILD_OP (vm, pool, dict, setfont, setfont);
	BUILD_OP (vm, pool, dict, setgray, setgray);
	BUILD_OP (vm, pool, dict, sethsbcolor, sethsbcolor);
	BUILD_OP (vm, pool, dict, setlinecap, setlinecap);
	BUILD_OP (vm, pool, dict, setlinejoin, setlinejoin);
	BUILD_OP (vm, pool, dict, setlinewidth, setlinewidth);
	BUILD_OP (vm, pool, dict, setmatrix, setmatrix);
	BUILD_OP (vm, pool, dict, setmiterlimit, setmiterlimit);
	BUILD_OP (vm, pool, dict, setrgbcolor, setrgbcolor);
	BUILD_OP (vm, pool, dict, setscreen, setscreen);
	BUILD_OP (vm, pool, dict, settransfer, settransfer);
	BUILD_OP (vm, pool, dict, show, show);
	BUILD_OP (vm, pool, dict, showpage, showpage);
	BUILD_OP (vm, pool, dict, sin, sin);
	BUILD_OP (vm, pool, dict, sqrt, sqrt);
	BUILD_OP (vm, pool, dict, srand, srand);
	BUILD_OP (vm, pool, dict, StandardEncoding, standardencoding);
	BUILD_OP_ (vm, pool, dict, start, start);
	BUILD_OP (vm, pool, dict, status, status);
	BUILD_OP (vm, pool, dict, stop, stop);
	BUILD_OP (vm, pool, dict, stopped, stopped);
	BUILD_OP (vm, pool, dict, string, string);
	BUILD_OP (vm, pool, dict, stringwidth, stringwidth);
	BUILD_OP (vm, pool, dict, stroke, stroke);
	BUILD_OP (vm, pool, dict, strokepath, strokepath);
	BUILD_OP (vm, pool, dict, sub, sub);
	BUILD_OP (vm, pool, dict, token, token);
	BUILD_OP (vm, pool, dict, transform, transform);
	BUILD_OP (vm, pool, dict, translate, translate);
	BUILD_OP (vm, pool, dict, truncate, truncate);
	BUILD_OP (vm, pool, dict, type, type);
	BUILD_OP (vm, pool, dict, usertime, usertime);
	BUILD_OP (vm, pool, dict, vmstatus, vmstatus);
	BUILD_OP (vm, pool, dict, wcheck, wcheck);
	BUILD_OP (vm, pool, dict, where, where);
	BUILD_OP (vm, pool, dict, widthshow, widthshow);
	BUILD_OP (vm, pool, dict, write, write);
	BUILD_OP (vm, pool, dict, writehexstring, writehexstring);
	BUILD_OP (vm, pool, dict, writestring, writestring);
	BUILD_OP (vm, pool, dict, xcheck, xcheck);
	BUILD_OP (vm, pool, dict, xor, xor);
}

static void
hg_operator_level2_init(HgVM      *vm,
			HgMemPool *pool,
			HgDict    *dict)
{
	BUILD_OP (vm, pool, dict, arct, arct);
	BUILD_OP (vm, pool, dict, colorimage, colorimage);
	BUILD_OP (vm, pool, dict, cshow, cshow);
	BUILD_OP (vm, pool, dict, currentblackgeneration, currentblackgeneration);
	BUILD_OP (vm, pool, dict, currentcacheparams, currentcacheparams);
	BUILD_OP (vm, pool, dict, currentcmykcolor, currentcmykcolor);
	BUILD_OP (vm, pool, dict, currentcolor, currentcolor);
	BUILD_OP (vm, pool, dict, currentcolorrendering, currentcolorrendering);
	BUILD_OP (vm, pool, dict, currentcolorscreen, currentcolorscreen);
	BUILD_OP (vm, pool, dict, currentcolorspace, currentcolorspace);
	BUILD_OP (vm, pool, dict, currentcolortransfer, currentcolortransfer);
	BUILD_OP (vm, pool, dict, currentdevparams, currentdevparams);
	BUILD_OP (vm, pool, dict, currentgstate, currentgstate);
	BUILD_OP (vm, pool, dict, currenthalftone, currenthalftone);
	BUILD_OP (vm, pool, dict, currentobjectformat, currentobjectformat);
	BUILD_OP (vm, pool, dict, currentoverprint, currentoverprint);
	BUILD_OP (vm, pool, dict, currentpacking, currentpacking);
	BUILD_OP (vm, pool, dict, currentpagedevice, currentpagedevice);
	BUILD_OP (vm, pool, dict, currentshared, currentshared);
	BUILD_OP (vm, pool, dict, currentstrokeadjust, currentstrokeadjust);
	BUILD_OP (vm, pool, dict, currentsystemparams, currentsystemparams);
	BUILD_OP (vm, pool, dict, currentundercolorremoval, currentundercolorremoval);
	BUILD_OP (vm, pool, dict, currentuserparams, currentuserparams);
	BUILD_OP (vm, pool, dict, defineresource, defineresource);
	BUILD_OP (vm, pool, dict, defineuserobject, defineuserobject);
	BUILD_OP (vm, pool, dict, deletefile, deletefile);
	BUILD_OP (vm, pool, dict, execform, execform);
	BUILD_OP (vm, pool, dict, execuserobject, execuserobject);
	BUILD_OP (vm, pool, dict, filenameforall, filenameforall);
	BUILD_OP (vm, pool, dict, fileposition, fileposition);
	BUILD_OP (vm, pool, dict, filter, filter);
	BUILD_OP (vm, pool, dict, findencoding, findencoding);
	BUILD_OP (vm, pool, dict, findresource, findresource);
	BUILD_OP (vm, pool, dict, gcheck, gcheck);
	BUILD_OP (vm, pool, dict, GlobalFontDirectory, globalfontdirectory);
	BUILD_OP (vm, pool, dict, glyphshow, glyphshow);
	BUILD_OP (vm, pool, dict, gstate, gstate);
	BUILD_OP (vm, pool, dict, ineofill, ineofill);
	BUILD_OP (vm, pool, dict, infill, infill);
	BUILD_OP (vm, pool, dict, instroke, instroke);
	BUILD_OP (vm, pool, dict, inueofill, inueofill);
	BUILD_OP (vm, pool, dict, inufill, inufill);
	BUILD_OP (vm, pool, dict, inustroke, inustroke);
	BUILD_OP (vm, pool, dict, ISOLatin1Encoding, isolatin1encoding);
	BUILD_OP (vm, pool, dict, languagelevel, languagelevel);
	BUILD_OP (vm, pool, dict, makepattern, makepattern);
	BUILD_OP (vm, pool, dict, packedarray, packedarray);
	BUILD_OP (vm, pool, dict, printobject, printobject);
	BUILD_OP (vm, pool, dict, realtime, realtime);
	BUILD_OP (vm, pool, dict, rectclip, rectclip);
	BUILD_OP (vm, pool, dict, rectfill, rectfill);
	BUILD_OP (vm, pool, dict, rectstroke, rectstroke);
	BUILD_OP (vm, pool, dict, renamefile, renamefile);
	BUILD_OP (vm, pool, dict, resourceforall, resourceforall);
	BUILD_OP (vm, pool, dict, resourcestatus, resourcestatus);
	BUILD_OP (vm, pool, dict, rootfont, rootfont);
	BUILD_OP (vm, pool, dict, scheck, scheck);
	BUILD_OP (vm, pool, dict, selectfont, selectfont);
	BUILD_OP (vm, pool, dict, serialnumber, serialnumber);
	BUILD_OP (vm, pool, dict, setbbox, setbbox);
	BUILD_OP (vm, pool, dict, setblackgeneration, setblackgeneration);
	BUILD_OP (vm, pool, dict, setcachedevice2, setcachedevice2);
	BUILD_OP (vm, pool, dict, setcacheparams, setcacheparams);
	BUILD_OP (vm, pool, dict, setcmykcolor, setcmykcolor);
	BUILD_OP (vm, pool, dict, setcolor, setcolor);
	BUILD_OP (vm, pool, dict, setcolorrendering, setcolorrendering);
	BUILD_OP (vm, pool, dict, setcolorscreen, setcolorscreen);
	BUILD_OP (vm, pool, dict, setcolorspace, setcolorspace);
	BUILD_OP (vm, pool, dict, setcolortransfer, setcolortransfer);
	BUILD_OP (vm, pool, dict, setdevparams, setdevparams);
	BUILD_OP (vm, pool, dict, setfileposition, setfileposition);
	BUILD_OP (vm, pool, dict, setgstate, setgstate);
	BUILD_OP (vm, pool, dict, sethalftone, sethalftone);
	BUILD_OP (vm, pool, dict, setobjectformat, setobjectformat);
	BUILD_OP (vm, pool, dict, setoverprint, setoverprint);
	BUILD_OP (vm, pool, dict, setpacking, setpacking);
	BUILD_OP (vm, pool, dict, setpagedevice, setpagedevice);
	BUILD_OP (vm, pool, dict, setpattern, setpattern);
	BUILD_OP (vm, pool, dict, setshared, setshared);
	BUILD_OP (vm, pool, dict, setstrokeadjust, setstrokeadjust);
	BUILD_OP (vm, pool, dict, setsystemparams, setsystemparams);
	BUILD_OP (vm, pool, dict, setucacheparams, setucacheparams);
	BUILD_OP (vm, pool, dict, setundercolorremoval, setundercoloremoval);
	BUILD_OP (vm, pool, dict, setuserparams, setuserparams);
	BUILD_OP (vm, pool, dict, setvmthreshold, setvmthreshold);
	BUILD_OP (vm, pool, dict, shareddict, shareddict);
	BUILD_OP (vm, pool, dict, SharedFontDirectory, sharedfontdirectory);
	BUILD_OP (vm, pool, dict, startjob, startjob);
	BUILD_OP (vm, pool, dict, uappend, uappend);
	BUILD_OP (vm, pool, dict, ucache, ucache);
	BUILD_OP (vm, pool, dict, ucachestatus, ucachestatus);
	BUILD_OP (vm, pool, dict, ueofill, ueofill);
	BUILD_OP (vm, pool, dict, ufill, ufill);
	BUILD_OP (vm, pool, dict, undef, undef);
	BUILD_OP (vm, pool, dict, undefineresource, undefineresource);
	BUILD_OP (vm, pool, dict, undefineuserobject, undefineuserobject);
	BUILD_OP (vm, pool, dict, upath, upath);
	BUILD_OP (vm, pool, dict, UserObjects, userobjects);
	BUILD_OP (vm, pool, dict, ustroke, ustroke);
	BUILD_OP (vm, pool, dict, ustrokepath, ustrokepath);
	BUILD_OP (vm, pool, dict, vmreclaim, vmreclaim);
	BUILD_OP (vm, pool, dict, writeobject, writeobject);
	BUILD_OP (vm, pool, dict, xshow, xshow);
	BUILD_OP (vm, pool, dict, xyshow, xyshow);
	BUILD_OP (vm, pool, dict, yshow, yshow);
}

static void
hg_operator_level3_init(HgVM      *vm,
			HgMemPool *pool,
			HgDict    *dict)
{
	BUILD_OP_ (vm, pool, dict, addglyph, addglyph);
	BUILD_OP_ (vm, pool, dict, beginbfchar, beginbfchar);
	BUILD_OP_ (vm, pool, dict, beginbfrange, beginbfrange);
	BUILD_OP_ (vm, pool, dict, begincidchar, begincidchar);
	BUILD_OP_ (vm, pool, dict, begincidrange, begincidrange);
	BUILD_OP_ (vm, pool, dict, begincmap, begincmap);
	BUILD_OP_ (vm, pool, dict, begincodespacerange, begincodespacerange);
	BUILD_OP_ (vm, pool, dict, beginnotdefchar, beginnotdefchar);
	BUILD_OP_ (vm, pool, dict, beginnotdefrange, beginnotdefrange);
	BUILD_OP_ (vm, pool, dict, beginrearrangedfont, beginrearrangedfont);
	BUILD_OP_ (vm, pool, dict, beginusematrix, beginusematrix);
	BUILD_OP_ (vm, pool, dict, cliprestore, cliprestore);
	BUILD_OP_ (vm, pool, dict, clipsave, clipsave);
	BUILD_OP_ (vm, pool, dict, composefont, composefont);
	BUILD_OP_ (vm, pool, dict, currentsmoothness, currentsmoothness);
	BUILD_OP_ (vm, pool, dict, currentrapparams, currentrapparams);
	BUILD_OP_ (vm, pool, dict, endbfchar, endbfchar);
	BUILD_OP_ (vm, pool, dict, endbfrange, endbfrange);
	BUILD_OP_ (vm, pool, dict, endcidchar, endcidchar);
	BUILD_OP_ (vm, pool, dict, endcidrange, endcidrange);
	BUILD_OP_ (vm, pool, dict, endcmap, endcmap);
	BUILD_OP_ (vm, pool, dict, endcodespacerange, endcodespacerange);
	BUILD_OP_ (vm, pool, dict, endnotdefchar, endnotdefchar);
	BUILD_OP_ (vm, pool, dict, endnotdefrange, endnotdefrange);
	BUILD_OP_ (vm, pool, dict, endrearrangedfont, endrearrangedfont);
	BUILD_OP_ (vm, pool, dict, endusematrix, endusematrix);
	BUILD_OP_ (vm, pool, dict, findcolorrendering, findcolorrendering);
	BUILD_OP_ (vm, pool, dict, GetHalftoneName, gethalftonename);
	BUILD_OP_ (vm, pool, dict, GetPageDeviceName, getpagedevicename);
	BUILD_OP_ (vm, pool, dict, GetSubstituteCRD, getsubstitutecrd);
	BUILD_OP_ (vm, pool, dict, removeall, removeall);
	BUILD_OP_ (vm, pool, dict, removeglyphs, removeglyphs);
	BUILD_OP_ (vm, pool, dict, setsmoothness, setsmoothness);
	BUILD_OP_ (vm, pool, dict, settrapparams, settrapparams);
	BUILD_OP_ (vm, pool, dict, settrapzone, settrapzone);
	BUILD_OP_ (vm, pool, dict, shfill, shfill);
	BUILD_OP_ (vm, pool, dict, StartData, startdata);
	BUILD_OP_ (vm, pool, dict, usecmap, usecmap);
	BUILD_OP_ (vm, pool, dict, usefont, usefont);
}

static void
hg_operator_hieroglyph_init(HgVM      *vm,
			    HgMemPool *pool,
			    HgDict    *dict)
{
	BUILD_OP_ (vm, pool, dict, .abort, private_hg_abort);
	BUILD_OP_ (vm, pool, dict, .clearerror, private_hg_clearerror);
	BUILD_OP_ (vm, pool, dict, .currentglobal, private_hg_currentglobal);
	BUILD_OP_ (vm, pool, dict, .execn, private_hg_execn);
	BUILD_OP_ (vm, pool, dict, .findlibfile, private_hg_findlibfile);
	BUILD_OP_ (vm, pool, dict, .hgrevision, private_hg_hgrevision);
	BUILD_OP_ (vm, pool, dict, .initplugins, private_hg_initplugins);
	BUILD_OP_ (vm, pool, dict, .odef, private_hg_odef);
	BUILD_OP_ (vm, pool, dict, .product, private_hg_product);
	BUILD_OP_ (vm, pool, dict, .forceput, private_hg_forceput);
	BUILD_OP_ (vm, pool, dict, .revision, private_hg_revision);
	BUILD_OP_ (vm, pool, dict, .setglobal, private_hg_setglobal);
	BUILD_OP_ (vm, pool, dict, .startjobserver, private_hg_startjobserver);
}

/*
 * Public Functions
 */
HgOperator *
hg_operator_new(HgMemPool      *pool,
		const gchar    *name,
		HgOperatorFunc  func)
{
	HgOperator *retval;
	size_t len;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (func != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgOperator),
					 HG_FL_HGOBJECT);
	if (retval == NULL) {
		g_warning("Failed to create an operator.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_operator_vtable);

	len = strlen(name);
	retval->name = hg_mem_alloc(pool, len + 1);
	if (retval->name == NULL) {
		g_warning("Failed to create an operator.");
		return NULL;
	}
	strncpy(retval->name, name, len);
	retval->name[len] = 0;
	retval->operator = func;

	return retval;
}

gboolean
hg_operator_init(HgVM *vm)
{
	HgVMEmulationType type;
	HgStack *ostack;
	gboolean pool_mode;
	HgMemPool *pool;
	HgDict *systemdict, *dict;
	HgValueNode *key, *val;

	g_return_val_if_fail (vm != NULL, FALSE);

	type = hg_vm_get_emulation_level(vm);
	if (type < VM_EMULATION_LEVEL_1 ||
	    type > VM_EMULATION_LEVEL_3) {
		g_warning("[BUG] Unknown emulation level %d", type);

		return FALSE;
	}

	pool_mode = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);
	systemdict = hg_vm_get_dict_systemdict(vm);

	/* systemdict is a read-only dictionary.
	 * it needs to be changed to the writable once.
	 */
	hg_object_writable((HgObject *)systemdict);

	/* true */
	key = _hg_vm_get_name_node(vm, "true");
	HG_VALUE_MAKE_BOOLEAN (pool, val, TRUE);
	hg_dict_insert(pool, systemdict, key, val);
	/* false */
	key = _hg_vm_get_name_node(vm, "false");
	HG_VALUE_MAKE_BOOLEAN (pool, val, FALSE);
	hg_dict_insert(pool, systemdict, key, val);
	/* null */
	key = _hg_vm_get_name_node(vm, "null");
	HG_VALUE_MAKE_NULL (pool, val, NULL);
	hg_dict_insert(pool, systemdict, key, val);
	/* mark */
	key = _hg_vm_get_name_node(vm, "mark");
	HG_VALUE_MAKE_MARK (pool, val);
	hg_dict_insert(pool, systemdict, key, val);
	/* $error */
	key = _hg_vm_get_name_node(vm, "$error");
	dict = hg_vm_get_dict_error(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	/* errordict */
	key = _hg_vm_get_name_node(vm, "errordict");
	dict = hg_vm_get_dict_errordict(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	/* serverdict */
	key = _hg_vm_get_name_node(vm, "serverdict");
	dict = hg_vm_get_dict_serverdict(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	/* statusdict */
	key = _hg_vm_get_name_node(vm, "statusdict");
	dict = hg_vm_get_dict_statusdict(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	/* systemdict */
	key = _hg_vm_get_name_node(vm, "systemdict");
	HG_VALUE_MAKE_DICT (val, systemdict);
	hg_dict_insert(pool, systemdict, key, val);

	hg_operator_level1_init(vm, pool, systemdict);

	ostack = hg_vm_get_ostack(vm);

	/* hieroglyph specific operators */
	hg_operator_hieroglyph_init(vm, pool, systemdict);

	if (type >= VM_EMULATION_LEVEL_2)
		hg_operator_level2_init(vm, pool, systemdict);

	if (type >= VM_EMULATION_LEVEL_3)
		hg_operator_level3_init(vm, pool, systemdict);

	hg_vm_use_global_pool(vm, pool_mode);

	return TRUE;
}

gboolean
hg_operator_invoke(HgOperator *op,
		   HgVM       *vm)
{
	g_return_val_if_fail (op != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	return op->operator(op, vm);
}

const gchar *
hg_operator_get_name(HgOperator *op)
{
	g_return_val_if_fail (op != NULL, NULL);

	return op->name;
}
