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
#include <hieroglyph/version.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgdebug.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hglineedit.h>
#include <hieroglyph/hgmatrix.h>
#include <hieroglyph/hgpath.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/hgvaluenode.h>
#include "operator.h"
#include "lbgraphics.h"
#include "lbstack.h"
#include "scanner.h"
#include "vm.h"


struct _LibrettoOperator {
	HgObject              object;
	gchar                *name;
	LibrettoOperatorFunc  operator;
};


static void     _libretto_operator_real_set_flags(gpointer           data,
						  guint              flags);
static void     _libretto_operator_real_relocate (gpointer           data,
						  HgMemRelocateInfo *info);
static gpointer _libretto_operator_real_to_string(gpointer           data);


static HgObjectVTable __lb_operator_vtable = {
	.free      = NULL,
	.set_flags = _libretto_operator_real_set_flags,
	.relocate  = _libretto_operator_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = _libretto_operator_real_to_string,
};
LibrettoOperator *__lb_operator_list[LB_op_END];

/*
 * Operators
 */
#define DEFUNC_OP(func)							\
	static gboolean							\
	_libretto_operator_op_##func(LibrettoOperator *op,		\
				     gpointer          data)		\
	{								\
		LibrettoVM *vm = data;					\
		gboolean retval = FALSE;
#define DEFUNC_OP_END				\
		return retval;			\
	}
#define DEFUNC_UNIMPLEMENTED_OP(func)		\
	static gboolean				\
	_libretto_operator_op_##func(LibrettoOperator *op,		\
				     gpointer          data)		\
	{								\
		g_warning("%s isn't yet implemented.", __PRETTY_FUNCTION__); \
									\
		return FALSE;						\
	}

/* level 1 */
DEFUNC_OP (private_arraytomark)
G_STMT_START {
	/* %arraytomark is the same as {counttomark array astore exch pop} */
	while (1) {
		retval = libretto_operator_invoke(__lb_operator_list[LB_op_counttomark], vm);
		if (!retval)
			break;
		retval = libretto_operator_invoke(__lb_operator_list[LB_op_array], vm);
		if (!retval)
			break;
		retval = libretto_operator_invoke(__lb_operator_list[LB_op_astore], vm);
		if (!retval)
			break;
		retval = libretto_operator_invoke(__lb_operator_list[LB_op_exch], vm);
		if (!retval)
			break;
		retval = libretto_operator_invoke(__lb_operator_list[LB_op_pop], vm);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_dicttomark)
G_STMT_START {
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), i, j;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgValueNode *node, *key, *val;
	HgDict *dict;

	while (1) {
		for (i = 0; i < depth; i++) {
			node = libretto_stack_index(ostack, i);
			if (HG_IS_VALUE_MARK (node)) {
				if ((i % 2) != 0) {
					_libretto_operator_set_error(vm, op, LB_e_rangecheck);
					break;
				}
				dict = hg_dict_new(pool, i / 2);
				if (dict == NULL) {
					_libretto_operator_set_error(vm, op, LB_e_VMerror);
					break;
				}
				for (j = 1; j <= i; j += 2) {
					key = libretto_stack_index(ostack, i - j);
					val = libretto_stack_index(ostack, i - j - 1);
					if (!hg_mem_pool_is_own_object(pool, key) ||
					    !hg_mem_pool_is_own_object(pool, val)) {
						_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
						return retval;
					}
					if (!hg_dict_insert(pool, dict, key, val)) {
						_libretto_operator_set_error(vm, op, LB_e_VMerror);
						return retval;
					}
				}
				for (j = 0; j <= i; j++)
					libretto_stack_pop(ostack);
				HG_VALUE_MAKE_DICT (node, dict);
				if (node == NULL) {
					_libretto_operator_set_error(vm, op, LB_e_VMerror);
					break;
				}
				retval = libretto_stack_push(ostack, node);
				/* it must be true */
				break;
			}
		}
		if (i == depth) {
			_libretto_operator_set_error(vm, op, LB_e_unmatchedmark);
			break;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_for_pos_int_continue)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nlimit, *ninc, *nini, *node;
	gint32 iini, iinc, ilimit;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);
		nlimit = libretto_stack_index(estack, 2);
		ninc = libretto_stack_index(estack, 3);
		nini = libretto_stack_index(estack, 4);

		ilimit = HG_VALUE_GET_INTEGER (nlimit);
		iinc = HG_VALUE_GET_INTEGER (ninc);
		iini = HG_VALUE_GET_INTEGER (nini);
		if ((iinc > 0 && iini > ilimit) ||
		    (iinc < 0 && iini < ilimit)) {
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			retval = libretto_stack_push(estack, nself);
			break;
		}
		HG_VALUE_SET_INTEGER (nini, iini + iinc);
		HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm), node, iini);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, node);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
			break;
		}
		node = hg_object_copy((HgObject *)nproc);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, nself); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_for_pos_real_continue)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nlimit, *ninc, *nini, *node;
	gdouble dini, dinc, dlimit;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);
		nlimit = libretto_stack_index(estack, 2);
		ninc = libretto_stack_index(estack, 3);
		nini = libretto_stack_index(estack, 4);

		dlimit = HG_VALUE_GET_REAL (nlimit);
		dinc = HG_VALUE_GET_REAL (ninc);
		dini = HG_VALUE_GET_REAL (nini);
		if ((dinc > 0.0 && dini > dlimit) ||
		    (dinc < 0.0 && dini < dlimit)) {
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			retval = libretto_stack_push(estack, nself);
			break;
		}
		HG_VALUE_SET_REAL (nini, dini + dinc);
		HG_VALUE_MAKE_REAL (libretto_vm_get_current_pool(vm), node, dini);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, node);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
			break;
		}
		node = hg_object_copy((HgObject *)nproc);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, nself); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_forall_array_continue)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nval, *nn, *copy_proc, *node;
	HgArray *array;
	gint32 i;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);
		nval = libretto_stack_index(estack, 2);
		nn = libretto_stack_index(estack, 3);

		array = HG_VALUE_GET_ARRAY (nval);
		i = HG_VALUE_GET_INTEGER (nn);
		if (hg_array_length(array) <= i) {
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			retval = libretto_stack_push(estack, nself); /* dummy */
			/* it must be true */
			break;
		}
		HG_VALUE_SET_INTEGER (nn, i + 1);
		node = hg_array_index(array, i);
		copy_proc = hg_object_copy((HgObject *)nproc);
		if (copy_proc == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, node);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
			break;
		}
		libretto_stack_push(estack, copy_proc);
		retval = libretto_stack_push(estack, nself); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_forall_dict_continue)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nval, *nn, *copy_proc, *key, *val;
	HgDict *dict;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);
		nval = libretto_stack_index(estack, 2);
		nn = libretto_stack_index(estack, 3);

		dict = HG_VALUE_GET_DICT (nval);
		if (!hg_dict_first(dict, &key, &val)) {
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			retval = libretto_stack_push(estack, nself); /* dummy */
			/* it must be true */
			break;
		}
		hg_dict_remove(dict, key);
		copy_proc = hg_object_copy((HgObject *)nproc);
		if (copy_proc == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(ostack, key);
		retval = libretto_stack_push(ostack, val);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
			break;
		}
		libretto_stack_push(estack, copy_proc);
		retval = libretto_stack_push(estack, nself); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_forall_string_continue)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nval, *nn, *copy_proc, *node;
	HgString *string;
	gint32 i;
	gchar c;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);
		nval = libretto_stack_index(estack, 2);
		nn = libretto_stack_index(estack, 3);

		string = HG_VALUE_GET_STRING (nval);
		i = HG_VALUE_GET_INTEGER (nn);
		if (hg_string_maxlength(string) <= i) {
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			libretto_stack_pop(estack);
			retval = libretto_stack_push(estack, nself); /* dummy */
			/* it must be true */
			break;
		}
		HG_VALUE_SET_INTEGER (nn, i + 1);
		c = hg_string_index(string, i);
		HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm), node, c);
		copy_proc = hg_object_copy((HgObject *)nproc);
		if (copy_proc == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, node);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
			break;
		}
		libretto_stack_push(estack, copy_proc);
		retval = libretto_stack_push(estack, nself); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_loop_continue)
G_STMT_START
{
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *node;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);

		node = hg_object_copy((HgObject *)nproc);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, nself); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_repeat_continue)
G_STMT_START
{
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *nself, *nproc, *nn, *node;
	gint32 n;

	while (1) {
		nself = libretto_stack_index(estack, 0);
		nproc = libretto_stack_index(estack, 1);
		nn = libretto_stack_index(estack, 2);

		n = HG_VALUE_GET_INTEGER (nn);
		if (n > 0) {
			HG_VALUE_SET_INTEGER (nn, n - 1);
			node = hg_object_copy((HgObject *)nproc);
			if (node == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_push(estack, node);
			retval = libretto_stack_push(estack, nself); /* dummy */
			if (!retval)
				_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
			break;
		}
		libretto_stack_pop(estack);
		libretto_stack_pop(estack);
		libretto_stack_pop(estack);
		retval = libretto_stack_push(estack, nself);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_stopped_continue)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgDict *error = libretto_vm_get_dict_error(vm);
	HgValueNode *node, *node2, *name;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	name = libretto_vm_get_name_node(vm, ".isstop");
	node = hg_dict_lookup(error, name);
	if (node != NULL &&
	    HG_IS_VALUE_BOOLEAN (node) &&
	    HG_VALUE_GET_BOOLEAN (node) == TRUE) {
		HG_VALUE_MAKE_BOOLEAN (pool, node2, TRUE);
		HG_VALUE_SET_BOOLEAN (node, FALSE);
		hg_dict_insert(pool, error, name, node);
		libretto_vm_clear_error(vm);
	} else {
		HG_VALUE_MAKE_BOOLEAN (pool, node2, FALSE);
	}
	retval = libretto_stack_push(ostack, node2);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (private_findfont);
DEFUNC_UNIMPLEMENTED_OP (private_definefont);

DEFUNC_OP (private_stringcvs)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgValueNode *node;
	HgString *hs;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		switch (node->type) {
		    case HG_TYPE_VALUE_BOOLEAN:
		    case HG_TYPE_VALUE_INTEGER:
		    case HG_TYPE_VALUE_REAL:
			    hs = hg_object_to_string((HgObject *)node);
			    break;
		    case HG_TYPE_VALUE_NAME:
			    hs = hg_string_new(pool, -1);
			    if (hs == NULL) {
				    _libretto_operator_set_error(vm, op, LB_e_VMerror);
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
				    _libretto_operator_set_error(vm, op, LB_e_VMerror);
				    return retval;
			    }
			    hg_string_append(hs,
					     libretto_operator_get_name(HG_VALUE_GET_POINTER (node)),
					     -1);
			    break;
		    default:
			    hs = hg_string_new(pool, 16);
			    if (hs == NULL) {
				    _libretto_operator_set_error(vm, op, LB_e_VMerror);
				    return retval;
			    }
			    hg_string_append(hs, "--nostringval--", -1);
			    break;
		}
		if (hs == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		hg_string_fix_string_size(hs);
		HG_VALUE_MAKE_STRING (node, hs);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (private_undefinefont);

DEFUNC_OP (private_write_eqeq_only)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *hs;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		hs = hg_object_to_string((HgObject *)n2);
		if (hs == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		} else {
			hg_file_object_write(file,
					     hg_string_get_string(hs),
					     sizeof (gchar),
					     hg_string_maxlength(hs));
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (abs)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n, *node;
	guint depth = libretto_stack_depth(ostack);
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			HG_VALUE_MAKE_INTEGER (pool, node, abs(HG_VALUE_GET_INTEGER (n)));
		} else if (HG_IS_VALUE_REAL (n)) {
			HG_VALUE_MAKE_REAL (pool, node, fabs(HG_VALUE_GET_REAL (n)));
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (add)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	guint depth = libretto_stack_depth(ostack);
	gboolean integer = TRUE;
	gdouble d1, d2;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
			integer = FALSE;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
			integer = FALSE;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (integer) {
			HG_VALUE_MAKE_INTEGER (pool, n1, (gint32)(d1 + d2));
		} else {
			HG_VALUE_MAKE_REAL (pool, n1, d1 + d2);
		}
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (aload)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), len, i;
	HgValueNode *narray;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		narray = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (narray)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)narray)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		libretto_stack_pop(ostack);
		array = HG_VALUE_GET_ARRAY (narray);
		len = hg_array_length(array);
		for (i = 0; i < len; i++) {
			libretto_stack_push(ostack, hg_array_index(array, i));
		}
		retval = libretto_stack_push(ostack, narray);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (anchorsearch);

DEFUNC_OP (and)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_BOOLEAN (n1) &&
		    HG_IS_VALUE_BOOLEAN (n2)) {
			gboolean result = HG_VALUE_GET_BOOLEAN (n1) & HG_VALUE_GET_BOOLEAN (n2);

			HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
			if (n1 == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
		} else if (HG_IS_VALUE_INTEGER (n1) &&
			   HG_IS_VALUE_INTEGER (n2)) {
			gint32 result = HG_VALUE_GET_INTEGER (n1) & HG_VALUE_GET_INTEGER (n2);

			HG_VALUE_MAKE_INTEGER (pool, n1, result);
			if (n1 == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (arc)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx, *ny, *nr, *nangle1, *nangle2;
	gdouble dx, dy, dr, dangle1, dangle2;

	while (1) {
		if (depth < 5) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		nangle2 = libretto_stack_index(ostack, 0);
		nangle1 = libretto_stack_index(ostack, 1);
		nr = libretto_stack_index(ostack, 2);
		ny = libretto_stack_index(ostack, 3);
		nx = libretto_stack_index(ostack, 4);
		if (HG_IS_VALUE_INTEGER (nx)) {
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		} else if (HG_IS_VALUE_REAL (nx)) {
			dx = HG_VALUE_GET_REAL (nx);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny)) {
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		} else if (HG_IS_VALUE_REAL (ny)) {
			dy = HG_VALUE_GET_REAL (ny);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nr)) {
			dr = HG_VALUE_GET_REAL_FROM_INTEGER (nr);
		} else if (HG_IS_VALUE_REAL (nr)) {
			dr = HG_VALUE_GET_REAL (nr);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nangle1)) {
			dangle1 = HG_VALUE_GET_REAL_FROM_INTEGER (nangle1);
		} else if (HG_IS_VALUE_REAL (nangle1)) {
			dangle1 = HG_VALUE_GET_REAL (nangle1);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nangle2)) {
			dangle2 = HG_VALUE_GET_REAL_FROM_INTEGER (nangle2);
		} else if (HG_IS_VALUE_REAL (nangle2)) {
			dangle2 = HG_VALUE_GET_REAL (nangle2);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		retval = libretto_graphic_state_path_arc(libretto_graphics_get_state(libretto_vm_get_graphics(vm)),
							 dx, dy, dr,
							 dangle1 * (2 * M_PI / 360),
							 dangle2 * (2 * M_PI / 360));
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (arcn);
DEFUNC_UNIMPLEMENTED_OP (arcto);

DEFUNC_OP (array)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node, *n;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gint32 size, i;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		size = HG_VALUE_GET_INTEGER (node);
		if (size < 0 || size > 65535) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		array = hg_array_new(pool, size);
		if (array == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		for (i = 0; i < size; i++) {
			HG_VALUE_MAKE_NULL (pool, n, NULL);
			hg_array_append(array, n);
		}
		HG_VALUE_MAKE_ARRAY (node, array);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (ashow);

DEFUNC_OP (astore)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), len, i;
	HgValueNode *node, *narray;
	HgArray *array;
	HgMemObject *obj;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		narray = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (narray)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)narray)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (narray);
		len = hg_array_length(array);
		if (depth < (len + 1)) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		hg_mem_get_object__inline(array, obj);
		for (i = 0; i < len; i++) {
			node = libretto_stack_index(ostack, len - i);
			if (!hg_mem_pool_is_own_object(obj->pool, node)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				return FALSE;
			}
			hg_array_replace(array, node, i);
		}
		for (i = 0; i <= len; i++)
			libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, narray);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (atan)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2, *node;
	gdouble d1, d2, result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1)) {
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		} else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2)) {
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		} else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_VALUE_REAL_SIMILAR (d1, 0) && HG_VALUE_REAL_SIMILAR (d2, 0)) {
			_libretto_operator_set_error(vm, op, LB_e_undefinedresult);
			break;
		}
		result = atan2(d1, d2) / (2 * M_PI / 360);
		if (result < 0)
			result = 360.0 + result;
		HG_VALUE_MAKE_REAL (libretto_vm_get_current_pool(vm),
				    node, result);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (awidthshow);

DEFUNC_OP (begin)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgDict *dict;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_DICT (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (node);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		retval = libretto_stack_push(dstack, node);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_dictstackoverflow);
			break;
		}
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (bind)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint length = libretto_stack_depth(ostack), array_len, i;
	HgValueNode *n, *node;
	HgArray *array;

	while (1) {
		if (length < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (n)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
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
					if (!libretto_stack_push(ostack, n)) {
						_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
					} else {
						_libretto_operator_op_bind(op, vm);
						libretto_stack_pop(ostack);
					}
				} else if (HG_IS_VALUE_NAME (n)) {
					node = libretto_vm_lookup(vm, n);
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

DEFUNC_UNIMPLEMENTED_OP (bitshift);
DEFUNC_UNIMPLEMENTED_OP (bytesavailable);
DEFUNC_UNIMPLEMENTED_OP (cachestatus);
DEFUNC_UNIMPLEMENTED_OP (ceiling);
DEFUNC_UNIMPLEMENTED_OP (charpath);

DEFUNC_OP (clear)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);

	libretto_stack_clear(ostack);
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cleardictstack)
G_STMT_START
{
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint depth = libretto_stack_depth(dstack);
	gint i = 0;
	LibrettoEmulationType type = libretto_vm_get_emulation_level(vm);

	switch (type) {
	    case LB_EMULATION_LEVEL_1:
		    i = depth - 2;
		    break;
	    case LB_EMULATION_LEVEL_2:
		    i = depth - 3;
		    break;
	    default:
		    g_warning("Unknown emulation level %d\n", type);
		    break;
	}
	if (i < 0) {
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
	} else {
		for (; i > 0; i--)
			libretto_stack_pop(dstack);
		retval = TRUE;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cleartomark)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), i, j;
	HgValueNode *node;

	for (i = 0; i < depth; i++) {
		node = libretto_stack_index(ostack, i);
		if (HG_IS_VALUE_MARK (node)) {
			for (j = 0; j <= i; j++) {
				libretto_stack_pop(ostack);
			}
			retval = TRUE;
			break;
		}
	}
	if (i == depth)
		_libretto_operator_set_error(vm, op, LB_e_unmatchedmark);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (clip);

DEFUNC_OP (clippath)
G_STMT_START
{
	retval = libretto_graphic_state_path_from_clip(libretto_graphics_get_state(libretto_vm_get_graphics(vm)));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (closefile);

DEFUNC_OP (closepath)
G_STMT_START
{
	retval = libretto_graphic_state_path_close(libretto_graphics_get_state(libretto_vm_get_graphics(vm)));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (concat)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), i;
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgArray *matrix;
	gdouble dmatrix[6];
	HgMatrix *mtx, *new_ctm;
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		matrix = HG_VALUE_GET_ARRAY (node);
		if (hg_array_length(matrix) != 6) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		for (i = 0; i < 6; i++) {
			node = hg_array_index(matrix, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL (node);
			} else {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
		}
		if (i == 6) {
			mtx = hg_matrix_new(pool,
					    dmatrix[0], dmatrix[1], dmatrix[2],
					    dmatrix[3], dmatrix[4], dmatrix[5]);
			if (mtx == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			new_ctm = hg_matrix_multiply(pool, mtx, &gstate->ctm);
			if (new_ctm == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
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
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (concatmatrix)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), i;
	HgValueNode *n1, *n2, *n3, *node;
	HgArray *m1, *m2, *m3;
	gdouble d1[6], d2[6];
	HgMatrix *mtx1, *mtx2, *mtx3;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n3 = libretto_stack_index(ostack, 0);
		n2 = libretto_stack_index(ostack, 1);
		n1 = libretto_stack_index(ostack, 2);
		if (!HG_IS_VALUE_ARRAY (n1) ||
		    !HG_IS_VALUE_ARRAY (n2) ||
		    !HG_IS_VALUE_ARRAY (n3)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2) ||
		    !hg_object_is_writable((HgObject *)n3)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		m1 = HG_VALUE_GET_ARRAY (n1);
		m2 = HG_VALUE_GET_ARRAY (n2);
		m3 = HG_VALUE_GET_ARRAY (n3);
		if (hg_array_length(m1) != 6 ||
		    hg_array_length(m2) != 6 ||
		    hg_array_length(m3) != 6) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		for (i = 0; i < 6; i++) {
			node = hg_array_index(m1, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				d1[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				d1[i] = HG_VALUE_GET_REAL (node);
			} else {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			node = hg_array_index(m2, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				d2[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				d2[i] = HG_VALUE_GET_REAL (node);
			} else {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
		}
		mtx1 = hg_matrix_new(pool, d1[0], d1[1], d1[2], d1[3], d1[4], d1[5]);
		mtx2 = hg_matrix_new(pool, d2[0], d2[1], d2[2], d2[3], d2[4], d2[5]);
		if (mtx1 == NULL || mtx2 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
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
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n3);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

static void
_libretto_operator_copy__traverse_dict(gpointer key,
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
}

DEFUNC_OP (copy)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node, *n2, *dup_node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			/* stack copy */
			guint32 i, n = HG_VALUE_GET_INTEGER (node);

			if (n < 0 || n >= depth) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			libretto_stack_pop(ostack);
			for (i = 0; i < n; i++) {
				node = libretto_stack_index(ostack, n - 1);
				dup_node = hg_object_dup((HgObject *)node);
				retval = libretto_stack_push(ostack, dup_node);
				if (!retval) {
					_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
					break;
				}
			}
			if (n == 0)
				retval = TRUE;
		} else {
			if (depth < 2) {
				_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
				break;
			}
			n2 = libretto_stack_index(ostack, 1);
			if (!hg_object_is_readable((HgObject *)node) ||
			    !hg_object_is_writable((HgObject *)n2)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
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
					_libretto_operator_set_error(vm, op, LB_e_rangecheck);
					break;
				}
				for (i = 0; i < len1; i++) {
					node = hg_array_index(a1, i);
					dup_node = hg_object_dup((HgObject *)node);
					if (dup_node == NULL) {
						_libretto_operator_set_error(vm, op, LB_e_VMerror);
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
				LibrettoEmulationType type = libretto_vm_get_emulation_level(vm);

				d2 = HG_VALUE_GET_DICT (node);
				d1 = HG_VALUE_GET_DICT (n2);
				if (!hg_object_is_readable((HgObject *)d1) ||
				    !hg_object_is_writable((HgObject *)d2)) {
					_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
					break;
				}
				if (type == LB_EMULATION_LEVEL_1) {
					if (hg_dict_length(d2) != 0 ||
					    hg_dict_maxlength(d1) != hg_dict_maxlength(d2)) {
						_libretto_operator_set_error(vm, op, LB_e_rangecheck);
						break;
					}
				}
				hg_dict_traverse(d1, _libretto_operator_copy__traverse_dict, d2);
				HG_VALUE_MAKE_DICT (dup_node, d2);
			} else if (HG_IS_VALUE_STRING (node) &&
				   HG_IS_VALUE_STRING (n2)) {
				HgString *s1, *s2;
				guint len1, len2, mlen1, mlen2, i;
				gchar c;

				s2 = HG_VALUE_GET_STRING (node);
				s1 = HG_VALUE_GET_STRING (n2);
				len1 = hg_string_maxlength(s1);
				len2 = hg_string_maxlength(s2);
				mlen1 = hg_string_maxlength(s1);
				mlen2 = hg_string_maxlength(s2);
				if (mlen1 > mlen2) {
					_libretto_operator_set_error(vm, op, LB_e_rangecheck);
					break;
				}
				for (i = 0; i < len1; i++) {
					c = hg_string_index(s1, i);
					hg_string_insert_c(s2, c, i);
				}
				if (mlen2 > mlen1) {
					HgMemObject *obj;

					hg_mem_get_object__inline(s2, obj);
					/* need to make a subarray for that */
					s2 = hg_string_make_substring(obj->pool, s2, 0, len1 - 1);
				}
				HG_VALUE_MAKE_STRING (dup_node, s2);
			} else {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, dup_node);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (libretto_vm_get_current_pool(vm),
				    node,
				    cos(d * (2 * M_PI / 360)));
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (count)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm), node, depth);
	if (node == NULL) {
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
	} else {
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (countdictstack)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint ddepth = libretto_stack_depth(dstack);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm), node, ddepth);
	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (countexecstack)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint edepth = libretto_stack_depth(estack);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm), node, edepth);
	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (counttomark)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node = NULL;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gint32 i;

	for (i = 0; i < depth; i++) {
		node = libretto_stack_index(ostack, i);
		if (HG_IS_VALUE_MARK (node)) {
			HG_VALUE_MAKE_INTEGER (pool, node, i);
			break;
		}
	}
	if (i == depth || node == NULL) {
		_libretto_operator_set_error(vm, op, LB_e_unmatchedmark);
	} else {
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (currentdash);

DEFUNC_OP (currentdict)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	HgValueNode *node, *dup_node;

	node = libretto_stack_index(dstack, 0);
	dup_node = hg_object_dup((HgObject *)node);
	retval = libretto_stack_push(ostack, dup_node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (currentfile);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgArray *array;
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node) ||
		    !hg_object_is_writable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (node);
		if (hg_array_length(array) != 6) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgValueNode *node;
	gdouble dx, dy;

	while (1) {
		if (!hg_path_compute_current_point(gstate->path, &dx, &dy)) {
			_libretto_operator_set_error(vm, op, LB_e_nocurrentpoint);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, node, dx);
		libretto_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, dy);
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx1, *ny1, *nx2, *ny2, *nx3, *ny3;
	gdouble dx1, dy1, dx2, dy2, dx3, dy3, dx, dy;
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));

	while (1) {
		if (depth < 6) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		ny3 = libretto_stack_index(ostack, 0);
		nx3 = libretto_stack_index(ostack, 1);
		ny2 = libretto_stack_index(ostack, 2);
		nx2 = libretto_stack_index(ostack, 3);
		ny1 = libretto_stack_index(ostack, 4);
		nx1 = libretto_stack_index(ostack, 5);
		if (HG_IS_VALUE_INTEGER (nx1)) {
			dx1 = HG_VALUE_GET_REAL_FROM_INTEGER (nx1);
		} else if (HG_IS_VALUE_REAL (nx1)) {
			dx1 = HG_VALUE_GET_REAL (nx1);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny1)) {
			dy1 = HG_VALUE_GET_REAL_FROM_INTEGER (ny1);
		} else if (HG_IS_VALUE_REAL (ny1)) {
			dy1 = HG_VALUE_GET_REAL (ny1);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nx2)) {
			dx2 = HG_VALUE_GET_REAL_FROM_INTEGER (nx2);
		} else if (HG_IS_VALUE_REAL (nx2)) {
			dx2 = HG_VALUE_GET_REAL (nx2);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny2)) {
			dy2 = HG_VALUE_GET_REAL_FROM_INTEGER (ny2);
		} else if (HG_IS_VALUE_REAL (ny2)) {
			dy2 = HG_VALUE_GET_REAL (ny2);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nx3)) {
			dx3 = HG_VALUE_GET_REAL_FROM_INTEGER (nx3);
		} else if (HG_IS_VALUE_REAL (nx3)) {
			dx3 = HG_VALUE_GET_REAL (nx3);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny3)) {
			dy3 = HG_VALUE_GET_REAL_FROM_INTEGER (ny3);
		} else if (HG_IS_VALUE_REAL (ny3)) {
			dy3 = HG_VALUE_GET_REAL (ny3);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_path_compute_current_point(gstate->path, &dx, &dy)) {
			_libretto_operator_set_error(vm, op, LB_e_nocurrentpoint);
			break;
		}
		retval = libretto_graphic_state_path_curveto(gstate,
							     dx1, dy1,
							     dx2, dy2,
							     dx3, dy3);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}

		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvi)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			/* nothing to do here */
		} else if (HG_IS_VALUE_REAL (node)) {
			gdouble d = HG_VALUE_GET_REAL (node);

			if (d >= G_MAXINT) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			HG_VALUE_MAKE_INTEGER (pool, node, (gint32)d);
		} else if (HG_IS_VALUE_STRING (node)) {
			HgString *str;
			HgValueNode *n;
			HgFileObject *file;

			if (!hg_object_is_readable((HgObject *)node)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			str = HG_VALUE_GET_STRING (node);
			file = hg_file_object_new(pool,
						  HG_FILE_TYPE_BUFFER,
						  HG_FILE_MODE_READ,
						  "--%cvi--",
						  hg_string_get_string(str),
						  hg_string_maxlength(str));
			n = libretto_scanner_get_object(vm, file);
			if (n == NULL) {
				if (!libretto_vm_has_error(vm))
					_libretto_operator_set_error(vm, op, LB_e_syntaxerror);
				break;
			} else if (HG_IS_VALUE_INTEGER (n)) {
				HG_VALUE_MAKE_INTEGER (pool, node, HG_VALUE_GET_INTEGER (n));
			} else if (HG_IS_VALUE_REAL (n)) {
				gdouble d = HG_VALUE_GET_REAL (n);

				if (d >= G_MAXINT) {
					_libretto_operator_set_error(vm, op, LB_e_rangecheck);
					break;
				}
				HG_VALUE_MAKE_INTEGER (pool, node, (gint32)d);
			} else {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			hg_mem_free(n);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvlit)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		hg_object_unexecutable((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (cvn)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgString *hs;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n1 = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (n1)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)n1)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		hs = HG_VALUE_GET_STRING (n1);
		HG_VALUE_MAKE_NAME (n2, hg_string_get_string(hs));
		if (n2 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		if (hg_object_is_executable((HgObject *)n1))
			hg_object_executable((HgObject *)n2);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n2);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (cvr);
DEFUNC_UNIMPLEMENTED_OP (cvrs);

DEFUNC_OP (cvx)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		hg_object_executable((HgObject *)node);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (def)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint odepth = libretto_stack_depth(ostack);
	HgValueNode *nd, *nk, *nv;
	HgDict *dict;
	HgMemObject *obj;

	while (1) {
		if (odepth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nv = libretto_stack_index(ostack, 0);
		nk = libretto_stack_index(ostack, 1);
		nd = libretto_stack_index(dstack, 0);
		dict = HG_VALUE_GET_DICT (nd);
		hg_mem_get_object__inline(dict, obj);
		if (!hg_object_is_writable((HgObject *)dict) ||
		    !hg_mem_pool_is_own_object(obj->pool, nk) ||
		    !hg_mem_pool_is_own_object(obj->pool, nv)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		retval = hg_dict_insert(libretto_vm_get_current_pool(vm),
					dict, nk, nv);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (defaultmatrix);

DEFUNC_OP (dict)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgDict *dict;
	gint32 n_dict;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		n_dict = HG_VALUE_GET_INTEGER (node);
		if (n_dict < 0) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		if (n_dict > 65535) {
			_libretto_operator_set_error(vm, op, LB_e_limitcheck);
			break;
		}
		dict = hg_dict_new(pool, n_dict);
		if (dict == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_DICT (node, dict);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (dictstack)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint depth = libretto_stack_depth(ostack);
	guint ddepth = libretto_stack_depth(dstack);
	guint len, i;
	HgValueNode *node, *dup_node;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (node);
		len = hg_array_length(array);
		if (ddepth > len) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		for (i = 0; i < ddepth; i++) {
			node = libretto_stack_index(dstack, ddepth - i - 1);
			dup_node = hg_object_dup((HgObject *)node);
			hg_array_replace(array, dup_node, i);
		}
		if (ddepth != len) {
			HgMemObject *obj;

			hg_mem_get_object__inline(array, obj);
			if (obj == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			array = hg_array_make_subarray(obj->pool, array, 0, ddepth - 1);
			HG_VALUE_MAKE_ARRAY (node, array);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, node);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gdouble d1, d2;
	guint depth = libretto_stack_depth(ostack);

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_VALUE_REAL_SIMILAR(d2, 0)) {
			_libretto_operator_set_error(vm, op, LB_e_undefinedresult);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, n1, d1 / d2);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (dtransform);

DEFUNC_OP (dup)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node, *dup_node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		dup_node = hg_object_dup((HgObject *)node);
		retval = libretto_stack_push(ostack, dup_node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (echo);
DEFUNC_UNIMPLEMENTED_OP (eexec);

DEFUNC_OP (end)
G_STMT_START
{
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint depth = libretto_stack_depth(dstack);
	LibrettoEmulationType type = libretto_vm_get_emulation_level(vm);

	while (1) {
		if ((type >= LB_EMULATION_LEVEL_2 && depth <= 3) ||
		    (type == LB_EMULATION_LEVEL_1 && depth <= 2)) {
			_libretto_operator_set_error(vm, op, LB_e_dictstackunderflow);
			break;
		}
		libretto_stack_pop(dstack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (eoclip);

DEFUNC_OP (eofill)
G_STMT_START
{
	retval = libretto_graphics_render_eofill(libretto_vm_get_graphics(vm));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (eq)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
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
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (erasepage);

DEFUNC_OP (exch)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_pop(ostack);
		n1 = libretto_stack_pop(ostack);
		libretto_stack_push(ostack, n2);
		libretto_stack_push(ostack, n1);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (exec)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node, *self, *tmp;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (hg_object_is_executable((HgObject *)node)) {
			if (!hg_object_is_readable((HgObject *)node)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			self = libretto_stack_pop(estack);
			tmp = hg_object_copy((HgObject *)node);
			if (tmp)
				libretto_stack_push(estack, tmp);
			retval = libretto_stack_push(estack, self);
			if (!tmp)
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
			else if (!retval)
				_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
			else
				libretto_stack_pop(ostack);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	guint edepth = libretto_stack_depth(estack);
	guint len, i;
	HgValueNode *node, *dup_node;
	HgArray *array;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		array = HG_VALUE_GET_ARRAY (node);
		len = hg_array_length(array);
		if (edepth > len) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		/* don't include the last node. it's a node for this call */
		for (i = 0; i < edepth - 1; i++) {
			node = libretto_stack_index(estack, edepth - i - 1);
			dup_node = hg_object_dup((HgObject *)node);
			hg_array_replace(array, dup_node, i);
		}
		if (edepth != (len + 1)) {
			HgMemObject *obj;

			hg_mem_get_object__inline(array, obj);
			if (obj == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			array = hg_array_make_subarray(obj->pool, array, 0, edepth - 2);
			HG_VALUE_MAKE_ARRAY (node, array);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, node);
			/* it must be true */
		} else {
			retval = TRUE;
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (executeonly);

DEFUNC_OP (exit)
G_STMT_START
{
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(estack), i, j;
	const gchar *name;
	HgValueNode *node;

	for (i = 0; i < depth; i++) {
		node = libretto_stack_index(estack, i);
		if (HG_IS_VALUE_POINTER (node)) {
			/* target operators are:
			 * cshow filenameforall for forall kshow loop pathforall
			 * repeat resourceforall
			 */
			name = libretto_operator_get_name(HG_VALUE_GET_POINTER (node));
			if (strcmp(name, "%for_pos_int_continue") == 0 ||
			    strcmp(name, "%for_pos_real_continue") == 0) {
				/* drop down ini inc limit proc in estack */
				libretto_stack_pop(estack);
				libretto_stack_pop(estack);
				libretto_stack_pop(estack);
				libretto_stack_pop(estack);
			} else if (strcmp(name, "%loop_continue") == 0) {
				/* drop down proc in estack */
				libretto_stack_pop(estack);
			} else if (strcmp(name, "%repeat_continue") == 0) {
				/* drop down n proc in estack */
				libretto_stack_pop(estack);
				libretto_stack_pop(estack);
			} else if (strcmp(name, "%forall_array_continue") == 0 ||
				   strcmp(name, "%forall_dict_continue") == 0 ||
				   strcmp(name, "%forall_string_continue") == 0) {
				/* drop down n val proc in estack */
				libretto_stack_pop(estack);
				libretto_stack_pop(estack);
				libretto_stack_pop(estack);
			} else {
				continue;
			}
			for (j = 0; j < i; j++)
				libretto_stack_pop(estack);
			retval = TRUE;
			break;
		} else if (HG_IS_VALUE_FILE (node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidexit);
			break;
		}
	}
	if (i == depth)
		_libretto_operator_set_error(vm, op, LB_e_invalidexit);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (exp)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gdouble base, exponent, result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1)) {
			base = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		} else if (HG_IS_VALUE_REAL (n1)) {
			base = HG_VALUE_GET_REAL (n1);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2)) {
			exponent = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		} else if (HG_IS_VALUE_REAL (n2)) {
			exponent = HG_VALUE_GET_REAL (n2);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_VALUE_REAL_SIMILAR (base, 0.0) &&
		    HG_VALUE_REAL_SIMILAR (exponent, 0.0)) {
			_libretto_operator_set_error(vm, op, LB_e_undefinedresult);
			break;
		}
		result = pow(base, exponent);
		HG_VALUE_MAKE_REAL (pool, n1, result);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

static HgFileType
_libretto_operator_get_file_type(const gchar *p)
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
_libretto_operator_get_file_mode(const gchar *p)
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), file_mode;
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gchar *s1 = NULL, *s2 = NULL;
	HgFileObject *file = NULL;
	HgFileType file_type;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_STRING (n1) || !HG_IS_VALUE_STRING (n2)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}

		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		s1 = hg_string_get_string(HG_VALUE_GET_STRING (n1));
		s2 = hg_string_get_string(HG_VALUE_GET_STRING (n2));
		file_type = _libretto_operator_get_file_type(s1);
		file_mode = _libretto_operator_get_file_mode(s2);

		if (file_type == HG_FILE_TYPE_FILE) {
			file = hg_file_object_new(pool, file_type, file_mode, s1);
			if (hg_file_object_has_error(file)) {
				_libretto_operator_set_error_from_file(vm, op, file);
				break;
			}
		} else if (file_type == HG_FILE_TYPE_STDIN) {
			if (file_mode != HG_FILE_MODE_READ) {
				_libretto_operator_set_error(vm, op, LB_e_invalidfileaccess);
				break;
			}
			file = libretto_vm_get_io(vm, LB_IO_STDIN);
		} else if (file_type == HG_FILE_TYPE_STDOUT) {
			if (file_mode != HG_FILE_MODE_WRITE) {
				_libretto_operator_set_error(vm, op, LB_e_invalidfileaccess);
				break;
			}
			file = libretto_vm_get_io(vm, LB_IO_STDOUT);
		} else if (file_type == HG_FILE_TYPE_STDERR) {
			if (file_mode != HG_FILE_MODE_WRITE) {
				_libretto_operator_set_error(vm, op, LB_e_invalidfileaccess);
				break;
			}
			file = libretto_vm_get_io(vm, LB_IO_STDERR);
		} else if (file_type == HG_FILE_TYPE_STATEMENT_EDIT) {
			if (file_mode != HG_FILE_MODE_READ) {
				_libretto_operator_set_error(vm, op, LB_e_invalidfileaccess);
				break;
			}
			file = hg_file_object_new(pool, file_type);
			if (file == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_undefinedfilename);
				break;
			}
			if (hg_file_object_has_error(file)) {
				_libretto_operator_set_error_from_file(vm, op, file);
				break;
			}
		} else if (file_type == HG_FILE_TYPE_LINE_EDIT) {
			if (file_mode != HG_FILE_MODE_READ) {
				_libretto_operator_set_error(vm, op, LB_e_invalidfileaccess);
				break;
			}
			file = hg_file_object_new(pool, file_type);
			if (file == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_undefinedfilename);
				break;
			}
			if (hg_file_object_has_error(file)) {
				_libretto_operator_set_error_from_file(vm, op, file);
				break;
			}
		} else {
			g_warning("unknown open type: %d", file_type);
		}
		if (file == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		HG_VALUE_MAKE_FILE (n1, file);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (fill)
G_STMT_START
{
	retval = libretto_graphics_render_fill(libretto_vm_get_graphics(vm));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
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

DEFUNC_UNIMPLEMENTED_OP (flushfile);
DEFUNC_UNIMPLEMENTED_OP (fontdirectory);

DEFUNC_OP (for)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint odepth = libretto_stack_depth(ostack);
	HgValueNode *nini, *ninc, *nlimit, *nproc, *node, *self;
	gboolean fint = TRUE;

	while (1) {
		if (odepth < 4) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nproc = libretto_stack_index(ostack, 0);
		nlimit = libretto_stack_index(ostack, 1);
		ninc = libretto_stack_index(ostack, 2);
		nini = libretto_stack_index(ostack, 3);
		if (!HG_IS_VALUE_ARRAY (nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    !hg_object_is_readable((HgObject *)nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
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
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_REAL (ninc)) {
			/* nothing to do */
		} else if (HG_IS_VALUE_INTEGER (ninc)) {
			if (!fint)
				HG_VALUE_SET_REAL (ninc, HG_VALUE_GET_INTEGER (ninc));
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_REAL (nini)) {
			/* nothing to do */
		} else if (HG_IS_VALUE_INTEGER (nini)) {
			if (!fint)
				HG_VALUE_SET_REAL (nini, HG_VALUE_GET_INTEGER (nini));
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}

		self = libretto_stack_pop(estack);
		libretto_stack_push(estack, nini);
		libretto_stack_push(estack, ninc);
		libretto_stack_push(estack, nlimit);
		libretto_stack_push(estack, nproc);
		if (fint)
			node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
							  "%for_pos_int_continue");
		else
			node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
							  "%for_pos_real_continue");
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, self); /* dummy */
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
			break;
		}

		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (forall)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node, *nval, *nproc, *nn, *self;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nproc = libretto_stack_index(ostack, 0);
		nval = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_ARRAY (nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    !hg_object_is_readable((HgObject *)nproc) ||
		    !hg_object_is_readable((HgObject *)nval)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (nval)) {
			node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
							  "%forall_array_continue");
			HG_VALUE_MAKE_INTEGER (pool, nn, 0);
		} else if (HG_IS_VALUE_DICT (nval)) {
			HgDict *dict = HG_VALUE_GET_DICT (nval);

			if (!hg_object_is_readable((HgObject *)dict)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
							  "%forall_dict_continue");
			HG_VALUE_SET_DICT (nval, hg_object_dup((HgObject *)dict));
			HG_VALUE_MAKE_INTEGER (pool, nn, 0);
		} else if (HG_IS_VALUE_STRING (nval)) {
			node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
							  "%forall_string_continue");
			HG_VALUE_MAKE_INTEGER (pool, nn, 0);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		self = libretto_stack_pop(estack);
		libretto_stack_push(estack, nn);
		libretto_stack_push(estack, nval);
		libretto_stack_push(estack, nproc);
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, self); /* dummy */
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ge)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
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
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) >= 0);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (get)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), len;
	HgValueNode *n1, *n2, *node = NULL;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgArray *array;
	HgDict *dict;
	HgString *string;
	gint32 index;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_ARRAY (n1)) {
			if (!hg_object_is_readable((HgObject *)n1)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			if (!HG_IS_VALUE_INTEGER (n2)) {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			array = HG_VALUE_GET_ARRAY (n1);
			len = hg_array_length(array);
			if (index < 0 || index >= len) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			node = hg_array_index(array, index);
		} else if (HG_IS_VALUE_DICT (n1)) {
			dict = HG_VALUE_GET_DICT (n1);
			if (!hg_object_is_readable((HgObject *)dict)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			node = hg_dict_lookup(dict, n2);
			if (node == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_undefined);
				break;
			}
		} else if (HG_IS_VALUE_STRING (n1)) {
			if (!hg_object_is_readable((HgObject *)n1)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			if (!HG_IS_VALUE_INTEGER (n2)) {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			string = HG_VALUE_GET_STRING (n1);
			len = hg_string_maxlength(string);
			if (index < 0 || index >= len) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			HG_VALUE_MAKE_INTEGER (pool, n1, hg_string_index(string, index));
			if (n1 == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			node = n1;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (getinterval)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2, *n3;
	HgMemObject *obj;
	gint32 index, count;

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n3 = libretto_stack_index(ostack, 0);
		n2 = libretto_stack_index(ostack, 1);
		n1 = libretto_stack_index(ostack, 2);
		if (!HG_IS_VALUE_INTEGER (n2) ||
		    !HG_IS_VALUE_INTEGER (n3)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		index = HG_VALUE_GET_INTEGER (n2);
		count = HG_VALUE_GET_INTEGER (n3);
		if (!hg_object_is_readable((HgObject *)n1)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (n1)) {
			HgArray *array = HG_VALUE_GET_ARRAY (n1), *subarray;
			guint len = hg_array_length(array);

			if (index >= len ||
			    (len - index) < count) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
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
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			hg_mem_get_object__inline(string, obj);
			substring = hg_string_make_substring(obj->pool, string, index, index + count - 1);
			HG_VALUE_MAKE_STRING (n1, substring);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (grestore)
G_STMT_START
{
	retval = libretto_graphics_restore(libretto_vm_get_graphics(vm));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (grestoreall);

DEFUNC_OP (gsave)
G_STMT_START
{
	retval = libretto_graphics_save(libretto_vm_get_graphics(vm));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (gt)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
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
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) > 0);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (identmatrix);

DEFUNC_OP (idiv)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gint32 i1, i2;
	guint depth = libretto_stack_depth(ostack);

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n1) ||
		    !HG_IS_VALUE_INTEGER (n2)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		i1 = HG_VALUE_GET_INTEGER (n1);
		i2 = HG_VALUE_GET_INTEGER (n2);
		if (i2 == 0) {
			_libretto_operator_set_error(vm, op, LB_e_undefinedresult);
			break;
		}
		HG_VALUE_MAKE_INTEGER (pool, n1, i1 / i2);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (idtransform);

DEFUNC_OP (if)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nflag, *nif, *self, *node;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nif = libretto_stack_index(ostack, 0);
		nflag = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_ARRAY (nif) ||
		    !HG_IS_VALUE_BOOLEAN (nflag)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nif) ||
		    !hg_object_is_readable((HgObject *)nif)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		self = libretto_stack_pop(estack);
		if (HG_VALUE_GET_BOOLEAN (nflag)) {
			node = hg_object_copy((HgObject *)nif);
			retval = libretto_stack_push(estack, node);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
				break;
			}
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(estack, self); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ifelse)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nflag, *nif, *nelse, *self, *node;

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nelse = libretto_stack_index(ostack, 0);
		nif = libretto_stack_index(ostack, 1);
		nflag = libretto_stack_index(ostack, 2);
		if (!HG_IS_VALUE_ARRAY (nelse) ||
		    !HG_IS_VALUE_ARRAY (nif) ||
		    !HG_IS_VALUE_BOOLEAN (nflag)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nif) ||
		    !hg_object_is_executable((HgObject *)nelse) ||
		    !hg_object_is_readable((HgObject *)nif) ||
		    !hg_object_is_readable((HgObject *)nelse)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		self = libretto_stack_pop(estack);
		if (HG_VALUE_GET_BOOLEAN (nflag)) {
			node = hg_object_copy((HgObject *)nif);
			retval = libretto_stack_push(estack, node);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
				break;
			}
		} else {
			node = hg_object_copy((HgObject *)nelse);
			retval = libretto_stack_push(estack, node);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
				break;
			}
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(estack, self); /* dummy */
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (image);
DEFUNC_UNIMPLEMENTED_OP (imagemask);

DEFUNC_OP (index)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *node;
	gint32 i;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n1 = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (n1)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		i = HG_VALUE_GET_INTEGER (n1);
		if (i < 0 || (i + 1) >= depth) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		node = libretto_stack_index(ostack, i + 1);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (initclip)
G_STMT_START
{
	retval = libretto_graphics_initclip(libretto_vm_get_graphics(vm));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (initgraphics)
G_STMT_START
{
	retval = libretto_graphics_init(libretto_vm_get_graphics(vm));
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (initmatrix);
DEFUNC_UNIMPLEMENTED_OP (internaldict);
DEFUNC_UNIMPLEMENTED_OP (invertmatrix);
DEFUNC_UNIMPLEMENTED_OP (itransform);

DEFUNC_OP (known)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *ndict, *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgDict *dict;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		ndict = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_DICT (ndict)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (ndict);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (hg_dict_lookup(dict, node) != NULL) {
			HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
		} else {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (kshow);

DEFUNC_OP (le)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
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
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) <= 0);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (length)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!hg_object_is_readable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
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
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (lineto)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx, *ny;
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));
	gdouble dx, dy, d;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		ny = libretto_stack_index(ostack, 0);
		nx = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_path_compute_current_point(gstate->path, &d, &d)) {
			_libretto_operator_set_error(vm, op, LB_e_nocurrentpoint);
			break;
		}
		retval = libretto_graphic_state_path_lineto(gstate, dx, dy);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (ln);

DEFUNC_OP (load)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nkey, *nval;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nkey = libretto_stack_index(ostack, 0);
		nval = libretto_vm_lookup(vm, nkey);
		if (nval == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_undefined);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, nval);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (log);

DEFUNC_OP (loop)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *self, *nproc, *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nproc = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    !hg_object_is_readable((HgObject *)nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		self = libretto_stack_pop(estack);
		libretto_stack_push(estack, nproc);
		node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
						  "%loop_continue");
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, self); /* dummy */
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
			break;
		}

		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (lt)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
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
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			s1 = HG_VALUE_GET_STRING (n1);
			s2 = HG_VALUE_GET_STRING (n2);
			result = (strcmp(hg_string_get_string(s1), hg_string_get_string(s2)) < 0);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (makefont);

DEFUNC_OP (matrix)
G_STMT_START {
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;
	HgArray *array;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	guint i;
	gdouble d[6] = {1.0, .0, .0, 1.0, .0, .0};

	while (1) {
		array = hg_array_new(pool, 6);
		if (array == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		for (i = 0; i < 6; i++) {
			HG_VALUE_MAKE_REAL (pool, node, d[i]);
			if (node == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			hg_array_append(array, node);
		}
		HG_VALUE_MAKE_ARRAY (node, array);
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (maxlength)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgDict *dict;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_DICT (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (node);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		HG_VALUE_MAKE_INTEGER (pool, node, hg_dict_maxlength(dict));
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (mod)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	guint depth = libretto_stack_depth(ostack);
	gint32 i2, result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n1) || !HG_IS_VALUE_INTEGER (n2)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		i2 = HG_VALUE_GET_INTEGER (n2);
		if (i2 == 0) {
			_libretto_operator_set_error(vm, op, LB_e_undefinedresult);
			break;
		}
		result = HG_VALUE_GET_INTEGER (n1) % i2;
		HG_VALUE_MAKE_INTEGER (pool, n1, result);
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (moveto)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx, *ny;
	gdouble dx, dy;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		ny = libretto_stack_index(ostack, 0);
		nx = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		retval = libretto_graphic_state_path_moveto(libretto_graphics_get_state(libretto_vm_get_graphics(vm)),
							    dx, dy);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (mul)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gdouble d1, d2;
	guint depth = libretto_stack_depth(ostack);
	gdouble integer = TRUE;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
			integer = FALSE;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
			integer = FALSE;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		d1 *= d2;
		if (integer && d1 <= G_MAXINT) {
			HG_VALUE_MAKE_INTEGER (pool, n1, d1);
		} else {
			HG_VALUE_MAKE_REAL (pool, n1, d1);
		}
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (ne)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gboolean result;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!hg_object_is_readable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
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
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (neg)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n, *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	guint depth = libretto_stack_depth(ostack);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (n)) {
			HG_VALUE_MAKE_INTEGER (pool, node, -HG_VALUE_GET_INTEGER (n));
		} else if (HG_IS_VALUE_REAL (n)) {
			HG_VALUE_MAKE_REAL (pool, node, -HG_VALUE_GET_REAL (n));
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (newpath)
G_STMT_START
{
	retval = libretto_graphic_state_path_new(libretto_graphics_get_state(libretto_vm_get_graphics(vm)));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (noaccess)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node) &&
		    !HG_IS_VALUE_DICT (node) &&
		    !HG_IS_VALUE_FILE (node) &&
		    !HG_IS_VALUE_STRING (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_DICT (node)) {
			HgDict *dict = HG_VALUE_GET_DICT (node);

			if (!hg_object_is_writable((HgObject *)dict)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			hg_object_set_state((HgObject *)dict,
					    hg_object_get_state((HgObject *)dict) & ~(HG_ST_READABLE | HG_ST_WRITABLE));
		}
		hg_object_set_state((HgObject *)node,
				    hg_object_get_state((HgObject *)node) & ~(HG_ST_READABLE | HG_ST_WRITABLE));
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (not)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_BOOLEAN (node)) {
			gboolean bool = HG_VALUE_GET_BOOLEAN (node);

			HG_VALUE_MAKE_BOOLEAN (pool, node, !bool);
		} else if (HG_IS_VALUE_INTEGER (node)) {
			gint32 i = HG_VALUE_GET_INTEGER (node);

			HG_VALUE_MAKE_INTEGER (pool, node, ~i);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (nulldevice);

DEFUNC_OP (or)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_BOOLEAN (n1) &&
		    HG_IS_VALUE_BOOLEAN (n2)) {
			gboolean result = HG_VALUE_GET_BOOLEAN (n1) | HG_VALUE_GET_BOOLEAN (n2);

			HG_VALUE_MAKE_BOOLEAN (pool, n1, result);
			if (n1 == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
		} else if (HG_IS_VALUE_INTEGER (n1) &&
			   HG_IS_VALUE_INTEGER (n2)) {
			gint32 result = HG_VALUE_GET_INTEGER (n1) | HG_VALUE_GET_INTEGER (n2);

			HG_VALUE_MAKE_INTEGER (pool, n1, result);
			if (n1 == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (pathbbox)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgValueNode *node;
	HgPathBBox bbox;

	while (1) {
		if (libretto_vm_get_emulation_level(vm) == LB_EMULATION_LEVEL_1) {
			retval = libretto_graphic_state_get_bbox_from_path(libretto_graphics_get_state(libretto_vm_get_graphics(vm)),
									   FALSE, &bbox);
		} else {
			retval = libretto_graphic_state_get_bbox_from_path(libretto_graphics_get_state(libretto_vm_get_graphics(vm)),
									   TRUE, &bbox);
		}
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_nocurrentpoint);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, node, bbox.llx);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, bbox.lly);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, bbox.urx);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(ostack, node);
		HG_VALUE_MAKE_REAL (pool, node, bbox.ury);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (pathforall);

DEFUNC_OP (pop)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		libretto_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (print)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgString *hs;
	HgFileObject *stdout;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		hs = HG_VALUE_GET_STRING (node);
		stdout = libretto_vm_get_io(vm, LB_IO_STDOUT);
		hg_file_object_write(stdout,
				     hg_string_get_string(hs),
				     sizeof (gchar),
				     hg_string_maxlength(hs));
		if (hg_file_object_has_error(stdout)) {
			_libretto_operator_set_error_from_file(vm, op, stdout);
			break;
		}
		libretto_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

/* almost code is shared to _libretto_operator_op_private_hg_forceput */
DEFUNC_OP (put)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), len;
	gint32 index;
	HgValueNode *n1, *n2, *n3;
	HgMemObject *obj;

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n3 = libretto_stack_index(ostack, 0);
		n2 = libretto_stack_index(ostack, 1);
		n1 = libretto_stack_index(ostack, 2);
		if (!hg_object_is_writable((HgObject *)n1)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (n1)) {
			HgArray *array = HG_VALUE_GET_ARRAY (n1);

			hg_mem_get_object__inline(array, obj);
			if (!HG_IS_VALUE_INTEGER (n2)) {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			len = hg_array_length(array);
			index = HG_VALUE_GET_INTEGER (n2);
			if (index > len || index > 65535) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			if (!hg_mem_pool_is_own_object(obj->pool, n3)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			retval = hg_array_replace(array, n3, index);
		} else if (HG_IS_VALUE_DICT (n1)) {
			HgDict *dict = HG_VALUE_GET_DICT (n1);

			hg_mem_get_object__inline(dict, obj);
			if (!hg_object_is_writable((HgObject *)dict)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			if (!hg_mem_pool_is_own_object(obj->pool, n2) ||
			    !hg_mem_pool_is_own_object(obj->pool, n3)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			retval = hg_dict_insert(obj->pool, dict, n2, n3);
		} else if (HG_IS_VALUE_STRING (n1)) {
			HgString *string = HG_VALUE_GET_STRING (n1);
			gint32 c;

			if (!HG_IS_VALUE_INTEGER (n2) || !HG_IS_VALUE_INTEGER (n3)) {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			c = HG_VALUE_GET_INTEGER (n3);
			retval = hg_string_insert_c(string, c, index);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (quit)
G_STMT_START
{
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	HgValueNode *self = libretto_stack_index(estack, 0);

	libretto_stack_clear(estack);
	retval = libretto_stack_push(estack, self);
	/* FIXME */
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (rand)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;
	GRand *rand_ = libretto_vm_get_random_generator(vm);

	HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm),
			       node, g_rand_int_range(rand_, 0, RAND_MAX));
	if (node == NULL) {
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
	} else {
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (rcheck)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
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
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (rcurveto);

DEFUNC_OP (read)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nfile, *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgFileObject *file, *stdin;
	gchar c;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nfile = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_FILE (nfile)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)nfile)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (nfile);
		stdin = libretto_vm_get_io(vm, LB_IO_STDIN);
		node = libretto_vm_lookup_with_string(vm, "JOBSERVER");
		if (file == stdin &&
		    node != NULL &&
		    HG_IS_VALUE_BOOLEAN (node) &&
		    HG_VALUE_GET_BOOLEAN (node) == TRUE) {
			/* it's under the job server mode now.
			 * this means stdin is being used to get PostScript.
			 * just returns false to not breaks it.
			 */
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, node);
			/* it must be true */
			break;
		}			
		c = hg_file_object_getc(file);
		if (hg_file_object_is_eof(file)) {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, node);
			/* it must be true */
			break;
		} else if (hg_file_object_has_error(file)) {
			_libretto_operator_set_error_from_file(vm, op, file);
			break;
		} else {
			libretto_stack_pop(ostack);
			HG_VALUE_MAKE_INTEGER (pool, node, c);
			libretto_stack_push(ostack, node);
			HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
			retval = libretto_stack_push(ostack, node);
			if (!retval)
				_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (readhexstring);
DEFUNC_UNIMPLEMENTED_OP (readline);
DEFUNC_UNIMPLEMENTED_OP (readonly);
DEFUNC_UNIMPLEMENTED_OP (readstring);

DEFUNC_OP (repeat)
G_STMT_START {
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n, *nproc, *node, *self;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nproc = libretto_stack_index(ostack, 0);
		n = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (n) || !HG_IS_VALUE_ARRAY (nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_executable((HgObject *)nproc) ||
		    !hg_object_is_readable((HgObject *)nproc)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (HG_VALUE_GET_INTEGER (n) < 0) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		self = libretto_stack_pop(estack);
		/* only n must be copied. */
		node = hg_object_copy((HgObject *)n);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(estack, node);
		libretto_stack_push(estack, nproc);
		node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
						  "%repeat_continue");
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, self); /* dummy */
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
			break;
		}

		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (resetfile);

DEFUNC_OP (restore)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemSnapshot *snapshot;
	HgMemPool *pool;
	gboolean global_mode;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_SNAPSHOT (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		snapshot = HG_VALUE_GET_SNAPSHOT (node);
		global_mode = libretto_vm_is_global_pool_used(vm);
		libretto_vm_use_global_pool(vm, FALSE);
		pool = libretto_vm_get_current_pool(vm);
		libretto_vm_use_global_pool(vm, global_mode);
		retval = hg_mem_pool_restore_snapshot(pool, snapshot);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_invalidrestore);
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
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx, *ny;
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));
	gdouble dx, dy, d;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		ny = libretto_stack_index(ostack, 0);
		nx = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_path_compute_current_point(gstate->path, &d, &d)) {
			_libretto_operator_set_error(vm, op, LB_e_nocurrentpoint);
			break;
		}
		retval = libretto_graphic_state_path_rlineto(gstate, dx, dy);
		if (!retval) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (rmoveto);

DEFUNC_OP (roll)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nd, *ni;
	gint32 index, d;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nd = libretto_stack_index(ostack, 0);
		ni = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_INTEGER (nd) || !HG_IS_VALUE_INTEGER (ni)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		index = HG_VALUE_GET_INTEGER (ni);
		if (index < 0) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		if ((depth - 2) < index) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		d = HG_VALUE_GET_INTEGER (nd);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_roll(ostack, index, d);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (rotate)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), len;
	HgValueNode *node, *nmatrix;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgArray *matrix = NULL;
	HgMatrix *mtx;
	gdouble angle;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nmatrix = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (nmatrix)) {
			if (depth < 2) {
				_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
				break;
			}
			if (!hg_object_is_writable((HgObject *)nmatrix)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			len = hg_array_length(matrix);
			if (len != 6) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			node = libretto_stack_index(ostack, 1);
		} else {
			node = nmatrix;
			nmatrix = NULL;
		}
		if (HG_IS_VALUE_INTEGER (node))
			angle = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		else if (HG_IS_VALUE_REAL (node))
			angle = HG_VALUE_GET_REAL (node);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		angle *= 2 * M_PI / 360;
		if (nmatrix != NULL) {
			mtx = hg_matrix_rotate(pool, angle);
			if (mtx == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			HG_VALUE_SET_REAL (hg_array_index(matrix, 0), mtx->xx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 1), mtx->yx);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 2), mtx->xy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 3), mtx->yy);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 4), mtx->x0);
			HG_VALUE_SET_REAL (hg_array_index(matrix, 5), mtx->y0);

			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, nmatrix);
		} else {
			retval = libretto_graphics_matrix_rotate(libretto_vm_get_graphics(vm),
								 angle);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (round);
DEFUNC_UNIMPLEMENTED_OP (rrand);

DEFUNC_OP (save)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgMemSnapshot *snapshot;
	HgMemPool *pool;
	gboolean global_mode = libretto_vm_is_global_pool_used(vm);
	HgValueNode *node;

	while (1) {
		libretto_vm_use_global_pool(vm, FALSE);
		pool = libretto_vm_get_current_pool(vm);
		snapshot = hg_mem_pool_save_snapshot(pool);
		HG_VALUE_MAKE_SNAPSHOT (node, snapshot);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_vm_use_global_pool(vm, global_mode);
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (scale)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx, *ny, *nmatrix;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgArray *matrix;
	HgMatrix *mtx;
	gdouble dx, dy;
	guint len, offset;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nmatrix = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (nmatrix)) {
			if (depth < 3) {
				_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
				break;
			}
			if (!hg_object_is_writable((HgObject *)nmatrix)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			len = hg_array_length(matrix);
			if (len != 6) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			ny = libretto_stack_index(ostack, 1);
			offset = 2;
		} else {
			ny = nmatrix;
			nmatrix = NULL;
			offset = 1;
		}
		nx = libretto_stack_index(ostack, offset);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
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

			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, nmatrix);
		} else {
			retval = libretto_graphics_matrix_scale(libretto_vm_get_graphics(vm),
								dx, dy);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (scalefont);
DEFUNC_UNIMPLEMENTED_OP (search);
DEFUNC_UNIMPLEMENTED_OP (setcachedevice);
DEFUNC_UNIMPLEMENTED_OP (setcachelimit);
DEFUNC_UNIMPLEMENTED_OP (setcharwidth);
DEFUNC_UNIMPLEMENTED_OP (setdash);
DEFUNC_UNIMPLEMENTED_OP (setflat);
DEFUNC_UNIMPLEMENTED_OP (setfont);

DEFUNC_OP (setgray)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node))
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		else if (HG_IS_VALUE_REAL (node))
			d = HG_VALUE_GET_REAL (node);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_graphic_state_color_set_rgb(libretto_vm_get_graphics(vm)->current_gstate,
							      d, d, d);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (sethsbcolor)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nh, *ns, *nb;
	gdouble dh, ds, db;

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nh = libretto_stack_index(ostack, 0);
		ns = libretto_stack_index(ostack, 1);
		nb = libretto_stack_index(ostack, 2);
		if (HG_IS_VALUE_INTEGER (nh))
			dh = HG_VALUE_GET_REAL_FROM_INTEGER (nh);
		else if (HG_IS_VALUE_REAL (nh))
			dh = HG_VALUE_GET_REAL (nh);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ns))
			ds = HG_VALUE_GET_REAL_FROM_INTEGER (ns);
		else if (HG_IS_VALUE_REAL (ns))
			ds = HG_VALUE_GET_REAL (ns);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nb))
			db = HG_VALUE_GET_REAL_FROM_INTEGER (nb);
		else if (HG_IS_VALUE_REAL (nb))
			db = HG_VALUE_GET_REAL (nb);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		retval = libretto_graphic_state_color_set_hsv(libretto_vm_get_graphics(vm)->current_gstate,
							      dh, ds, db);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (setlinecap);
DEFUNC_UNIMPLEMENTED_OP (setlinejoin);

DEFUNC_OP (setlinewidth)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node))
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		else if (HG_IS_VALUE_REAL (node))
			d = HG_VALUE_GET_REAL (node);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_graphic_state_set_linewidth(libretto_vm_get_graphics(vm)->current_gstate, d);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (setmatrix)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), i;
	HgValueNode *node;
	HgArray *matrix;
	LibrettoGraphicState *gstate = libretto_graphics_get_state(libretto_vm_get_graphics(vm));
	HgMatrix *mtx;
	gdouble dmatrix[6];

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_ARRAY (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_readable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		matrix = HG_VALUE_GET_ARRAY (node);
		if (hg_array_length(matrix) != 6) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		for (i = 0; i < 6; i++) {
			node = hg_array_index(matrix, i);
			if (HG_IS_VALUE_INTEGER (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL_FROM_INTEGER (node);
			} else if (HG_IS_VALUE_REAL (node)) {
				dmatrix[i] = HG_VALUE_GET_REAL (node);
			} else {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
		}
		if (i == 6) {
			mtx = hg_matrix_new(libretto_vm_get_current_pool(vm),
					    dmatrix[0], dmatrix[1], dmatrix[2],
					    dmatrix[3], dmatrix[4], dmatrix[5]);
			if (mtx == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			memcpy(&gstate->ctm, mtx, sizeof (HgMatrix));
			hg_mem_free(mtx);
			retval = hg_path_matrix(gstate->path,
						gstate->ctm.xx, gstate->ctm.yx,
						gstate->ctm.xy, gstate->ctm.yy,
						gstate->ctm.x0, gstate->ctm.y0);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (setmiterlimit);

DEFUNC_OP (setrgbcolor)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nr, *ng, *nb;
	gdouble dr, dg, db;

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nb = libretto_stack_index(ostack, 0);
		ng = libretto_stack_index(ostack, 1);
		nr = libretto_stack_index(ostack, 2);
		if (HG_IS_VALUE_INTEGER (nr))
			dr = HG_VALUE_GET_REAL_FROM_INTEGER (nr);
		else if (HG_IS_VALUE_REAL (nr))
			dr = HG_VALUE_GET_REAL (nr);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ng))
			dg = HG_VALUE_GET_REAL_FROM_INTEGER (ng);
		else if (HG_IS_VALUE_REAL (ng))
			dg = HG_VALUE_GET_REAL (ng);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (nb))
			db = HG_VALUE_GET_REAL_FROM_INTEGER (nb);
		else if (HG_IS_VALUE_REAL (nb))
			db = HG_VALUE_GET_REAL (nb);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		retval = libretto_graphic_state_color_set_rgb(libretto_vm_get_graphics(vm)->current_gstate,
							      dr, dg, db);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
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
	retval = libretto_graphics_show_page(libretto_vm_get_graphics(vm));
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (sin)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	gdouble d;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (libretto_vm_get_current_pool(vm),
				    node,
				    sin(d * (2 * M_PI / 360)));
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (sqrt)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gdouble d;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			d = HG_VALUE_GET_REAL_FROM_INTEGER (node);
		} else if (HG_IS_VALUE_REAL (node)) {
			d = HG_VALUE_GET_REAL (node);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (d < 0) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		HG_VALUE_MAKE_REAL (pool, node, sqrt(d));
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (srand)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	GRand *rand_;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		rand_ = libretto_vm_get_random_generator(vm);
		g_rand_set_seed(rand_, (guint32)HG_VALUE_GET_INTEGER (node));
		libretto_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (standardencoding);
DEFUNC_UNIMPLEMENTED_OP (start);
DEFUNC_UNIMPLEMENTED_OP (status);

DEFUNC_OP (stop)
G_STMT_START
{
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint edepth = libretto_stack_depth(estack), i;
	HgValueNode *node, *self, *key;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	for (i = 0; i < edepth; i++) {
		node = libretto_stack_index(estack, i);
		if (HG_IS_VALUE_POINTER (node) &&
		    strcmp("%stopped_continue", libretto_operator_get_name(HG_VALUE_GET_POINTER (node))) == 0) {
			break;
		}
	}
	if (i == edepth) {
		hg_stderr_printf("No /stopped operator found. exiting...\n");
		node = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm), ".abort");
		self = libretto_stack_pop(estack);
		libretto_stack_push(estack, node);
		retval = libretto_stack_push(estack, self);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
	} else {
		/* this operator itself is still in the estack. */
		for (; i > 1; i--)
			libretto_stack_pop(estack);
		key = libretto_vm_get_name_node(vm, ".isstop");
		HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
		hg_dict_insert(pool, libretto_vm_get_dict_error(vm), key, node);
		retval = TRUE;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (stopped)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node, *self, *proc;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!hg_object_is_readable((HgObject *)node)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (hg_object_is_executable((HgObject *)node)) {
			libretto_stack_pop(ostack);
			self = libretto_stack_pop(estack);
			proc = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
							  "%stopped_continue");
			libretto_stack_push(estack, proc);
			libretto_stack_push(estack, node);
			retval = libretto_stack_push(estack, self);
			if (!retval)
				_libretto_operator_set_error(vm, op, LB_e_execstackoverflow);
		} else {
			HG_VALUE_MAKE_BOOLEAN (libretto_vm_get_current_pool(vm),
					       node, FALSE);
			retval = libretto_stack_push(ostack, node);
			if (!retval)
				_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (store);

DEFUNC_OP (string)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	gint32 size;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgString *hs;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		size = HG_VALUE_GET_INTEGER (node);
		if (size < 0 || size > 65535) {
			_libretto_operator_set_error(vm, op, LB_e_rangecheck);
			break;
		}
		hs = hg_string_new(pool, size);
		if (hs == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_STRING (node, hs);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (stringwidth);

DEFUNC_OP (stroke)
G_STMT_START
{
	retval = libretto_graphics_render_stroke(libretto_vm_get_graphics(vm));
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (strokepath);

DEFUNC_OP (sub)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gdouble d1, d2;
	guint depth = libretto_stack_depth(ostack);
	gdouble integer = TRUE;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (HG_IS_VALUE_INTEGER (n1))
			d1 = HG_VALUE_GET_REAL_FROM_INTEGER (n1);
		else if (HG_IS_VALUE_REAL (n1)) {
			d1 = HG_VALUE_GET_REAL (n1);
			integer = FALSE;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (n2))
			d2 = HG_VALUE_GET_REAL_FROM_INTEGER (n2);
		else if (HG_IS_VALUE_REAL (n2)) {
			d2 = HG_VALUE_GET_REAL (n2);
			integer = FALSE;
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (integer) {
			HG_VALUE_MAKE_INTEGER (pool, n1, (gint32)(d1 - d2));
		} else {
			HG_VALUE_MAKE_REAL (pool, n1, d1 - d2);
		}
		if (n1 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, n1);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (token);
DEFUNC_UNIMPLEMENTED_OP (transform);

DEFUNC_OP (translate)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nx, *ny, *nmatrix;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgArray *matrix;
	HgMatrix *mtx;
	gdouble dx, dy;
	guint len, offset;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nmatrix = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_ARRAY (nmatrix)) {
			if (depth < 3) {
				_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
				break;
			}
			if (!hg_object_is_writable((HgObject *)nmatrix)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			matrix = HG_VALUE_GET_ARRAY (nmatrix);
			len = hg_array_length(matrix);
			if (len != 6) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			ny = libretto_stack_index(ostack, 1);
			offset = 2;
		} else {
			ny = nmatrix;
			nmatrix = NULL;
			offset = 1;
		}
		nx = libretto_stack_index(ostack, offset);
		if (HG_IS_VALUE_INTEGER (nx))
			dx = HG_VALUE_GET_REAL_FROM_INTEGER (nx);
		else if (HG_IS_VALUE_REAL (nx))
			dx = HG_VALUE_GET_REAL (nx);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (HG_IS_VALUE_INTEGER (ny))
			dy = HG_VALUE_GET_REAL_FROM_INTEGER (ny);
		else if (HG_IS_VALUE_REAL (ny))
			dy = HG_VALUE_GET_REAL (ny);
		else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
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

			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, nmatrix);
		} else {
			retval = libretto_graphics_matrix_translate(libretto_vm_get_graphics(vm),
								    dx, dy);
			if (!retval) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_pop(ostack);
			libretto_stack_pop(ostack);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (truncate)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (HG_IS_VALUE_INTEGER (node)) {
			/* nothing to do here */
			retval = TRUE;
		} else if (HG_IS_VALUE_REAL (node)) {
			gdouble result = floor(HG_VALUE_GET_REAL (node));

			HG_VALUE_MAKE_REAL (pool, node, result);
			if (node == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_pop(ostack);
			retval = libretto_stack_push(ostack, node);
			/* it must be true */
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (type)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		switch (node->type) {
		    case HG_TYPE_VALUE_BOOLEAN:
			    node = libretto_vm_get_name_node(vm, "booleantype");
			    break;
		    case HG_TYPE_VALUE_INTEGER:
			    node = libretto_vm_get_name_node(vm, "integertype");
			    break;
		    case HG_TYPE_VALUE_REAL:
			    node = libretto_vm_get_name_node(vm, "realtype");
			    break;
		    case HG_TYPE_VALUE_NAME:
			    node = libretto_vm_get_name_node(vm, "nametype");
			    break;
		    case HG_TYPE_VALUE_ARRAY:
			    node = libretto_vm_get_name_node(vm, "arraytype");
			    break;
		    case HG_TYPE_VALUE_STRING:
			    node = libretto_vm_get_name_node(vm, "stringtype");
			    break;
		    case HG_TYPE_VALUE_DICT:
			    node = libretto_vm_get_name_node(vm, "dicttype");
			    break;
		    case HG_TYPE_VALUE_NULL:
			    node = libretto_vm_get_name_node(vm, "nulltype");
			    break;
		    case HG_TYPE_VALUE_POINTER:
			    node = libretto_vm_get_name_node(vm, "operatortype");
			    break;
		    case HG_TYPE_VALUE_MARK:
			    node = libretto_vm_get_name_node(vm, "marktype");
			    break;
		    case HG_TYPE_VALUE_FILE:
			    node = libretto_vm_get_name_node(vm, "filetype");
			    break;
		    case HG_TYPE_VALUE_SNAPSHOT:
			    node = libretto_vm_get_name_node(vm, "savetype");
			    break;
		    case HG_TYPE_VALUE_END:
		    default:
			    g_warning("[BUG] Unknown node type %d was given.", node->type);
			    _libretto_operator_set_error(vm, op, LB_e_typecheck);

			    return retval;
		}
		hg_object_executable((HgObject *)node);
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (usertime)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;

	HG_VALUE_MAKE_INTEGER (libretto_vm_get_current_pool(vm),
			       node, libretto_vm_get_current_time(vm));
	if (node == NULL) {
		_libretto_operator_set_error(vm, op, LB_e_VMerror);
	} else {
		retval = libretto_stack_push(ostack, node);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (vmstatus)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *n1, *n2, *n3;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gint32 level, used, maximum;

	while (1) {
		level = libretto_vm_get_save_level(vm);
		used = hg_mem_pool_get_used_heap_size(pool);
		maximum = hg_mem_pool_get_free_heap_size(pool);
		HG_VALUE_MAKE_INTEGER (pool, n1, level);
		HG_VALUE_MAKE_INTEGER (pool, n2, used);
		HG_VALUE_MAKE_INTEGER (pool, n3, maximum);
		if (n1 == NULL ||
		    n2 == NULL ||
		    n3 == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_push(ostack, n1);
		libretto_stack_push(ostack, n2);
		retval = libretto_stack_push(ostack, n3);
		if (!retval)
			_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (wcheck)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
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
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (where)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	guint depth = libretto_stack_depth(ostack);
	guint ddepth = libretto_stack_depth(dstack), i;
	HgValueNode *node, *ndict;
	HgDict *dict = NULL;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		for (i = 0; i < ddepth; i++) {
			ndict = libretto_stack_index(dstack, i);
			if (!HG_IS_VALUE_DICT (ndict)) {
				g_warning("[BUG] Invalid dictionary was in the dictionary stack.");
				return retval;
			}
			dict = HG_VALUE_GET_DICT (ndict);
			if (hg_dict_lookup(dict, node) != NULL)
				break;
		}
		libretto_stack_pop(ostack);
		if (i == ddepth || dict == NULL) {
			HG_VALUE_MAKE_BOOLEAN (pool, node, FALSE);
			retval = libretto_stack_push(ostack, node);
			/* it must be true */
		} else {
			HG_VALUE_MAKE_DICT (node, dict);
			libretto_stack_push(ostack, node);
			HG_VALUE_MAKE_BOOLEAN (pool, node, TRUE);
			retval = libretto_stack_push(ostack, node);
			if (!retval)
				_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
		}
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (widthshow);
DEFUNC_UNIMPLEMENTED_OP (write);
DEFUNC_UNIMPLEMENTED_OP (writehexstring);

DEFUNC_OP (writestring)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *n1, *n2;
	HgFileObject *file;
	HgString *hs;

	while (1) {
		if (depth < 2) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n2 = libretto_stack_index(ostack, 0);
		n1 = libretto_stack_index(ostack, 1);
		if (!HG_IS_VALUE_FILE (n1) ||
		    !HG_IS_VALUE_STRING (n2)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		if (!hg_object_is_writable((HgObject *)n1) ||
		    !hg_object_is_readable((HgObject *)n2)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		file = HG_VALUE_GET_FILE (n1);
		hs = HG_VALUE_GET_STRING (n2);
		hg_file_object_write(file,
				     hg_string_get_string(hs),
				     sizeof (gchar),
				     hg_string_maxlength(hs));
		if (hg_file_object_has_error(file)) {
			_libretto_operator_set_error_from_file(vm, op, file);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (xcheck)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
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
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_UNIMPLEMENTED_OP (xor);

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
	HgFileObject *file = libretto_vm_get_io(vm, LB_IO_STDERR);
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	LibrettoStack *dstack = libretto_vm_get_dstack(vm);
	HgMemPool *local_pool, *global_pool;
	gboolean flag = libretto_vm_is_global_pool_used(vm);

	libretto_vm_use_global_pool(vm, TRUE);
	global_pool = libretto_vm_get_current_pool(vm);
	libretto_vm_use_global_pool(vm, FALSE);
	local_pool = libretto_vm_get_current_pool(vm);
	libretto_vm_use_global_pool(vm, flag);
	/* allow resizing to avoid /VMerror during dumping */
	hg_mem_pool_allow_resize(global_pool, TRUE);
	hg_mem_pool_allow_resize(local_pool, TRUE);

	hg_file_object_printf(file, "\nOperand stack:\n");
	libretto_stack_dump(ostack, file);
	hg_file_object_printf(file, "\nExecution stack:\n");
	libretto_stack_dump(estack, file);
	hg_file_object_printf(file, "\nDictionary stack:\n");
	libretto_stack_dump(dstack, file);
	abort();
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_clearerror)
G_STMT_START
{
	libretto_vm_reset_error(vm);
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_currentglobal)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;

	HG_VALUE_MAKE_BOOLEAN (libretto_vm_get_current_pool(vm), node, libretto_vm_is_global_pool_used(vm));
	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_execn)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	LibrettoStack *estack = libretto_vm_get_estack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *nn, *node, *copy_node, *self;
	gint32 i, n;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		nn = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (nn)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		n = HG_VALUE_GET_INTEGER (nn);
		if (depth < (n + 1)) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		self = libretto_stack_pop(estack);
		for (i = 0; i < n; i++) {
			node = libretto_stack_index(ostack, i + 1);
			copy_node = hg_object_copy((HgObject *)node);
			if (copy_node == NULL) {
				_libretto_operator_set_error(vm, op, LB_e_VMerror);
				break;
			}
			libretto_stack_push(estack, copy_node);
		}
		if (!libretto_vm_has_error(vm)) {
			libretto_stack_pop(ostack);
			for (i = 0; i < n; i++)
				libretto_stack_pop(ostack);
		}
		retval = libretto_stack_push(estack, self);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_hgrevision)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gint32 revision = 0;

	sscanf(__hg_rcsid, "$Rev: %d $", &revision);
	HG_VALUE_MAKE_INTEGER (pool, node, revision);
	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_loadhistory)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgString *hs;
	gchar *filename, *histfile;
	const gchar *env;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
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
		retval = hg_line_edit_load_history(histfile);
		g_free(histfile);
		g_free(filename);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_product)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgString *hs = hg_string_new(pool, -1);

	hg_string_append(hs, "Hieroglyph PostScript Interpreter", -1);
	hg_string_fix_string_size(hs);
	HG_VALUE_MAKE_STRING (node, hs);
	hg_object_unwritable((HgObject *)node);
	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

/* almost code is shared to _libretto_operator_op_put */
DEFUNC_OP (private_hg_forceput)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack), len;
	gint32 index;
	HgValueNode *n1, *n2, *n3;
	HgMemObject *obj, *robj;

	while (1) {
		if (depth < 3) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		n3 = libretto_stack_index(ostack, 0);
		n2 = libretto_stack_index(ostack, 1);
		n1 = libretto_stack_index(ostack, 2);
		if (!hg_object_is_writable((HgObject *)n1)) {
			_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
			break;
		}
		if (HG_IS_VALUE_ARRAY (n1)) {
			HgArray *array = HG_VALUE_GET_ARRAY (n1);

			if (!HG_IS_VALUE_INTEGER (n2)) {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			len = hg_array_length(array);
			index = HG_VALUE_GET_INTEGER (n2);
			if (index > len || index > 65535) {
				_libretto_operator_set_error(vm, op, LB_e_rangecheck);
				break;
			}
			hg_mem_get_object__inline(array, robj);
			hg_mem_get_object__inline(n3, obj);
			retval = hg_array_replace_forcibly(array, n3, index, TRUE);
			hg_mem_add_pool_reference(obj->pool, robj->pool, n3);
		} else if (HG_IS_VALUE_DICT (n1)) {
			HgDict *dict = HG_VALUE_GET_DICT (n1);
			HgMemObject *obj2;

			if (!hg_object_is_writable((HgObject *)dict)) {
				_libretto_operator_set_error(vm, op, LB_e_invalidaccess);
				break;
			}
			hg_mem_get_object__inline(dict, robj);
			hg_mem_get_object__inline(n2, obj);
			hg_mem_get_object__inline(n3, obj2);
			retval = hg_dict_insert_forcibly(obj->pool, dict, n2, n3, TRUE);
			if (obj->pool != robj->pool)
				hg_mem_add_pool_reference(obj->pool, robj->pool, n2);
			hg_mem_add_pool_reference(obj2->pool, robj->pool, n3);
		} else if (HG_IS_VALUE_STRING (n1)) {
			HgString *string = HG_VALUE_GET_STRING (n1);
			gint32 c;

			if (!HG_IS_VALUE_INTEGER (n2) || !HG_IS_VALUE_INTEGER (n3)) {
				_libretto_operator_set_error(vm, op, LB_e_typecheck);
				break;
			}
			index = HG_VALUE_GET_INTEGER (n2);
			c = HG_VALUE_GET_INTEGER (n3);
			retval = hg_string_insert_c(string, c, index);
		} else {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_revision)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	gint32 major, minor, release;

	sscanf(HIEROGLYPH_VERSION, "%d.%d.%d", &major, &minor, &release);
	HG_VALUE_MAKE_INTEGER (pool, node, major * 1000000 + minor * 1000 + release);
	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_savehistory)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgString *hs;
	gchar *filename, *histfile;
	const gchar *env;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
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
		retval = hg_line_edit_save_history(histfile);
		g_free(histfile);
		g_free(filename);
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_setglobal)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_BOOLEAN (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		libretto_vm_use_global_pool(vm, HG_VALUE_GET_BOOLEAN (node));
		libretto_stack_pop(ostack);
		retval = TRUE;
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_sleep)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_INTEGER (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		retval = libretto_graphics_debug(libretto_vm_get_graphics(vm),
						 &hg_debug_sleep,
						 GINT_TO_POINTER(HG_VALUE_GET_INTEGER (node)));
		libretto_stack_pop(ostack);
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_startgc)
G_STMT_START
{
	HgMemPool *pool;
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	gboolean result, flag = libretto_vm_is_global_pool_used(vm);
	HgValueNode *node;

	/* do GC for only local pool */
	libretto_vm_use_global_pool(vm, FALSE);
	pool = libretto_vm_get_current_pool(vm);
	result = hg_mem_garbage_collection(pool);
	HG_VALUE_MAKE_BOOLEAN (pool, node, result);
	libretto_vm_use_global_pool(vm, flag);

	retval = libretto_stack_push(ostack, node);
	if (!retval)
		_libretto_operator_set_error(vm, op, LB_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_startjobserver)
G_STMT_START
{
	HgValueNode *key, *val;

	key = libretto_vm_get_name_node(vm, "%initialized");
	val = hg_dict_lookup_with_string(libretto_vm_get_dict_systemdict(vm),
					 "true");
	hg_dict_insert(libretto_vm_get_current_pool(vm),
		       libretto_vm_get_dict_statusdict(vm),
		       key, val);

	/* set read-only attribute to systemdict */
	hg_object_unwritable((HgObject *)libretto_vm_get_dict_systemdict(vm));
	retval = TRUE;
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (private_hg_statementedit)
G_STMT_START
{
	LibrettoStack *ostack = libretto_vm_get_ostack(vm);
	guint depth = libretto_stack_depth(ostack);
	HgValueNode *node;
	HgMemPool *pool = libretto_vm_get_current_pool(vm);
	HgString *hs;
	HgFileObject *file;
	gchar *line, *prompt;

	while (1) {
		if (depth < 1) {
			_libretto_operator_set_error(vm, op, LB_e_stackunderflow);
			break;
		}
		node = libretto_stack_index(ostack, 0);
		if (!HG_IS_VALUE_STRING (node)) {
			_libretto_operator_set_error(vm, op, LB_e_typecheck);
			break;
		}
		hs = HG_VALUE_GET_STRING (node);
		prompt = g_strndup(hg_string_get_string(hs), hg_string_maxlength(hs));
		line = hg_line_edit_get_statement(libretto_vm_get_io(vm, LB_IO_STDIN),
						  prompt);
		g_free(prompt);
		if (line == NULL || line[0] == 0) {
			_libretto_operator_set_error(vm, op, LB_e_undefinedfilename);
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
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_FILE (node, file);
		if (node == NULL) {
			_libretto_operator_set_error(vm, op, LB_e_VMerror);
			break;
		}
		libretto_stack_pop(ostack);
		retval = libretto_stack_push(ostack, node);
		/* it must be true */
		break;
	}
} G_STMT_END;
DEFUNC_OP_END

#undef DEFUNC_UNIMPLEMENTED_OP
#undef DEFUNC_OP
#undef DEFUNC_OP_END

/*
 * Private Functions
 */
static void
_libretto_operator_real_set_flags(gpointer data,
				  guint    flags)
{
	LibrettoOperator *op = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(op->name, obj);
	if (obj == NULL) {
		g_warning("Invalid object %p to be marked: LibrettoOperator->name", op->name);
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
_libretto_operator_real_relocate(gpointer           data,
				 HgMemRelocateInfo *info)
{
	LibrettoOperator *op = data;

	if ((gsize)op->name >= info->start &&
	    (gsize)op->name <= info->end) {
		op->name = (gchar *)((gsize)op->name + info->diff);
	}
}

static gpointer
_libretto_operator_real_to_string(gpointer data)
{
	LibrettoOperator *op = data;
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

#define _libretto_operator_build_operator(vm, pool, sdict, name, func, ret_op) \
	G_STMT_START {							\
		HgValueNode *__lb_key, *__lb_val;			\
									\
		(ret_op) = libretto_operator_new((pool), #name, _libretto_operator_op_##func); \
		if ((ret_op) == NULL) {					\
			g_warning("Failed to create an operator %s", #name); \
		} else {						\
			__lb_key = libretto_vm_get_name_node((vm), #name); \
			HG_VALUE_MAKE_POINTER (__lb_val, (ret_op));	\
			if (__lb_val == NULL) {				\
				libretto_vm_set_error((vm), __lb_key, LB_e_VMerror, FALSE); \
			} else {					\
				hg_object_executable((HgObject *)__lb_val); \
				hg_dict_insert((pool), (sdict), __lb_key, __lb_val); \
			}						\
		}							\
	} G_STMT_END
#define BUILD_OP(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		LibrettoOperator *__lb_op;				\
									\
		_libretto_operator_build_operator(vm, pool, sdict, name, func, __lb_op); \
		if (__lb_op != NULL) {					\
			__lb_operator_list[LB_op_##name] = __lb_op;	\
		}							\
	} G_STMT_END
#define BUILD_OP_(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		LibrettoOperator *__lb_op;				\
									\
		_libretto_operator_build_operator(vm, pool, sdict, name, func, __lb_op); \
	} G_STMT_END

static void
libretto_operator_level1_init(LibrettoVM *vm,
			      HgMemPool  *pool,
			      HgDict     *dict)
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
	BUILD_OP (vm, pool, dict, matrix, matrix);
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
	BUILD_OP (vm, pool, dict, store, store);
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
libretto_operator_level2_init(LibrettoVM *vm,
			      HgMemPool  *pool,
			      HgDict     *dict)
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
libretto_operator_level3_init(LibrettoVM *vm,
			      HgMemPool  *pool,
			      HgDict     *dict)
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
libretto_operator_hieroglyph_init(LibrettoVM *vm,
				  HgMemPool  *pool,
				  HgDict     *dict)
{
	BUILD_OP_ (vm, pool, dict, .abort, private_hg_abort);
	BUILD_OP_ (vm, pool, dict, .clearerror, private_hg_clearerror);
	BUILD_OP_ (vm, pool, dict, .currentglobal, private_hg_currentglobal);
	BUILD_OP_ (vm, pool, dict, .execn, private_hg_execn);
	BUILD_OP_ (vm, pool, dict, .hgrevision, private_hg_hgrevision);
	BUILD_OP_ (vm, pool, dict, .loadhistory, private_hg_loadhistory);
	BUILD_OP_ (vm, pool, dict, .product, private_hg_product);
	BUILD_OP_ (vm, pool, dict, .forceput, private_hg_forceput);
	BUILD_OP_ (vm, pool, dict, .revision, private_hg_revision);
	BUILD_OP_ (vm, pool, dict, .savehistory, private_hg_savehistory);
	BUILD_OP_ (vm, pool, dict, .setglobal, private_hg_setglobal);
	BUILD_OP_ (vm, pool, dict, .sleep, private_hg_sleep);
	BUILD_OP_ (vm, pool, dict, .startgc, private_hg_startgc);
	BUILD_OP_ (vm, pool, dict, .startjobserver, private_hg_startjobserver);
	BUILD_OP_ (vm, pool, dict, .statementedit, private_hg_statementedit);
}

#undef BUILD_OP

/*
 * Public Functions
 */
LibrettoOperator *
libretto_operator_new(HgMemPool            *pool,
		      const gchar          *name,
		      LibrettoOperatorFunc  func)
{
	LibrettoOperator *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (func != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (LibrettoOperator));
	if (retval == NULL) {
		g_warning("Failed to create an operator.");
		return NULL;
	}
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__lb_operator_vtable;

	retval->name = hg_mem_alloc(pool, strlen(name) + 1);
	if (retval->name == NULL) {
		g_warning("Failed to create an operator.");
		return NULL;
	}
	strcpy(retval->name, name);
	retval->operator = func;

	return retval;
}

gboolean
libretto_operator_init(LibrettoVM *vm)
{
	LibrettoEmulationType type;
	LibrettoStack *ostack;
	gboolean pool_mode;
	HgMemPool *pool;
	HgDict *systemdict, *dict;
	HgValueNode *key, *val;
	HgMemObject *obj;

	g_return_val_if_fail (vm != NULL, FALSE);

	type = libretto_vm_get_emulation_level(vm);
	if (type < LB_EMULATION_LEVEL_1 ||
	    type > LB_EMULATION_LEVEL_3) {
		g_warning("[BUG] Unknown emulation level %d", type);

		return FALSE;
	}

	pool_mode = libretto_vm_is_global_pool_used(vm);
	libretto_vm_use_global_pool(vm, TRUE);
	pool = libretto_vm_get_current_pool(vm);
	systemdict = libretto_vm_get_dict_systemdict(vm);

	/* systemdict is a read-only dictionary.
	 * it needs to be changed to the writable once.
	 */
	hg_object_writable((HgObject *)systemdict);

	/* true */
	key = _libretto_vm_get_name_node(vm, "true");
	HG_VALUE_MAKE_BOOLEAN (pool, val, TRUE);
	hg_dict_insert(pool, systemdict, key, val);
	/* false */
	key = _libretto_vm_get_name_node(vm, "false");
	HG_VALUE_MAKE_BOOLEAN (pool, val, FALSE);
	hg_dict_insert(pool, systemdict, key, val);
	/* null */
	key = _libretto_vm_get_name_node(vm, "null");
	HG_VALUE_MAKE_NULL (pool, val, NULL);
	hg_dict_insert(pool, systemdict, key, val);
	/* mark */
	key = _libretto_vm_get_name_node(vm, "mark");
	HG_VALUE_MAKE_MARK (pool, val);
	hg_dict_insert(pool, systemdict, key, val);
	/* $error */
	key = _libretto_vm_get_name_node(vm, "$error");
	dict = libretto_vm_get_dict_error(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	hg_mem_get_object__inline(val, obj);
	hg_mem_add_pool_reference(obj->pool, pool, val);
	/* errordict */
	key = _libretto_vm_get_name_node(vm, "errordict");
	dict = libretto_vm_get_dict_errordict(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	hg_mem_get_object__inline(val, obj);
	hg_mem_add_pool_reference(obj->pool, pool, val);
	/* serverdict */
	key = _libretto_vm_get_name_node(vm, "serverdict");
	dict = libretto_vm_get_dict_serverdict(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	hg_mem_get_object__inline(val, obj);
	hg_mem_add_pool_reference(obj->pool, pool, val);
	/* statusdict */
	key = _libretto_vm_get_name_node(vm, "statusdict");
	dict = libretto_vm_get_dict_statusdict(vm);
	HG_VALUE_MAKE_DICT (val, dict);
	hg_dict_insert_forcibly(pool, systemdict, key, val, TRUE);
	hg_mem_get_object__inline(val, obj);
	hg_mem_add_pool_reference(obj->pool, pool, val);
	/* systemdict */
	key = _libretto_vm_get_name_node(vm, "systemdict");
	HG_VALUE_MAKE_DICT (val, systemdict);
	hg_dict_insert(pool, systemdict, key, val);

	libretto_operator_level1_init(vm, pool, systemdict);

	ostack = libretto_vm_get_ostack(vm);

	/* hieroglyph specific operators */
	libretto_operator_hieroglyph_init(vm, pool, systemdict);

	if (type >= LB_EMULATION_LEVEL_2)
		libretto_operator_level2_init(vm, pool, systemdict);

	if (type >= LB_EMULATION_LEVEL_3)
		libretto_operator_level3_init(vm, pool, systemdict);

	libretto_vm_use_global_pool(vm, pool_mode);

	return TRUE;
}

gboolean
libretto_operator_invoke(LibrettoOperator *op,
			 LibrettoVM       *vm)
{
	g_return_val_if_fail (op != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	return op->operator(op, vm);
}

const gchar *
libretto_operator_get_name(LibrettoOperator *op)
{
	g_return_val_if_fail (op != NULL, NULL);

	return op->name;
}
