/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * test-main.c
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

#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgfile.h>
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
		hg_operator_build_operator__inline(_test_op_, vm, pool, sdict, name, func, __hg_op); \
		if (__hg_op != NULL) {					\
			__test_operator_list[HG_test_op_##func] = __hg_op; \
			hg_mem_pool_add_root_node(pool, __hg_op);	\
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
			if (__hg_op == __test_operator_list[HG_test_op_##func]) { \
				hg_dict_remove(sdict, __hg_key);	\
			}						\
			hg_mem_pool_remove_root_node(pool, __hg_op);	\
			hg_mem_free(__hg_op);				\
		}							\
		hg_mem_free(__hg_key);					\
	} G_STMT_END
#define DEFUNC_OP(func)					\
	static gboolean					\
	_test_op_##func(HgOperator *op,		\
			 gpointer    data)		\
	{						\
		HgVM *vm = data;			\
		gboolean retval = FALSE;
#define DEFUNC_OP_END				\
		return retval;			\
	}

typedef enum {
	HG_test_op_validateunittest = 0,
	HG_test_op_END
} HgOperatorTestEncoding;


static gboolean plugin_init    (void);
static gboolean plugin_finalize(void);
static gboolean plugin_load    (HgPlugin *plugin,
				HgVM     *vm);
static gboolean plugin_unload  (HgPlugin *plugin,
				HgVM     *vm);


static HgPluginVTable vtable = {
	.init     = plugin_init,
	.finalize = plugin_finalize,
	.load     = plugin_load,
	.unload   = plugin_unload,
};
HgOperator *__test_operator_list[HG_test_op_END];

/*
 * Operators
 */
DEFUNC_OP (validateunittest)
G_STMT_START
{
	HgStack *ostack = hg_vm_get_ostack(vm);
	guint depth = hg_stack_depth(ostack);
	HgValueNode *n, *ndict, *nverbose, *nexp, *nactual, *nexpected, *nattrs;
	HgDict *dict;
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	gboolean result = TRUE, verbose = FALSE;
	HgString *sexp;
	HgArray *actual, *expected;
	guint length, i;
	gint32 attrs = 0;

	while (1) {
		if (depth < 1) {
			_hg_operator_set_error(vm, op, VM_e_stackunderflow);
			break;
		}
		ndict = hg_stack_index(ostack, 0);
		if (!HG_IS_VALUE_DICT (ndict)) {
			_hg_operator_set_error(vm, op, VM_e_typecheck);
			break;
		}
		dict = HG_VALUE_GET_DICT (ndict);
		if (!hg_object_is_readable((HgObject *)dict)) {
			_hg_operator_set_error(vm, op, VM_e_invalidaccess);
			break;
		}
		nverbose = hg_dict_lookup_with_string(dict, "verbose");
		if (nverbose != NULL && HG_IS_VALUE_BOOLEAN (nverbose)) {
			verbose = HG_VALUE_GET_BOOLEAN (nverbose);
		}
		nattrs = hg_dict_lookup_with_string(dict, "attrsmask");
		if (nattrs != NULL && HG_IS_VALUE_INTEGER (nattrs)) {
			attrs = HG_VALUE_GET_INTEGER (nattrs);
		}
		nexp = hg_dict_lookup_with_string(dict, "expression");
		sexp = hg_object_to_string((HgObject *)nexp);
		nactual = hg_dict_lookup_with_string(dict, "actualerror");
		nexpected = hg_dict_lookup_with_string(dict, "expectederror");
		if (nactual == NULL || nexpected == NULL) {
			hg_file_object_printf(hg_vm_get_io(vm, VM_IO_STDERR),
					      "[BUG] Unknown result happened during testing an exception at `%s'\n",
					      hg_string_get_string(sexp));
			result = FALSE;
		} else if (!hg_value_node_compare(nactual, nexpected)) {
			if (verbose) {
				HgString *sa, *se;

				sa = hg_object_to_string((HgObject *)nactual);
				se = hg_object_to_string((HgObject *)nexpected);
				hg_file_object_printf(hg_vm_get_io(vm, VM_IO_STDERR),
						      "Expression: %s - expected error is %s, but actual error was %s\n",
						      hg_string_get_string(sexp),
						      hg_string_get_string(se),
						      hg_string_get_string(sa));
				hg_mem_free(sa);
				hg_mem_free(se);
			}
			result = FALSE;
		}
		nactual = hg_dict_lookup_with_string(dict, "actualostack");
		nexpected = hg_dict_lookup_with_string(dict, "expectedostack");
		if (nactual == NULL || nexpected == NULL ||
		    !HG_IS_VALUE_ARRAY (nactual) ||
		    !HG_IS_VALUE_ARRAY (nexpected)) {
			hg_file_object_printf(hg_vm_get_io(vm, VM_IO_STDERR),
					      "[BUG] Uknown result happened during testing the result at `%s'\n",
					      hg_string_get_string(sexp));
			result = FALSE;
		} else {
			HgString *sa, *se;

			sa = hg_object_to_string((HgObject *)nactual);
			se = hg_object_to_string((HgObject *)nexpected);
			actual = HG_VALUE_GET_ARRAY (nactual);
			expected = HG_VALUE_GET_ARRAY (nexpected);
			if ((length = hg_array_length(actual)) != hg_array_length(expected)) {
				if (verbose) {
				  ostack_mismatch:
					hg_file_object_printf(hg_vm_get_io(vm, VM_IO_STDERR),
							      "Expression: %s - expected ostack is %s, but actual ostack was %s\n",
							      hg_string_get_string(sexp),
							      hg_string_get_string(se),
							      hg_string_get_string(sa));
				}
				result = FALSE;
			} else {
				for (i = 0; i < length; i++) {
					HgValueNode *nna = hg_array_index(actual, i);
					HgValueNode *nne = hg_array_index(expected, i);

					if (!hg_value_node_compare_content(nna, nne, attrs)) {
						goto ostack_mismatch;
					}
				}
			}
			hg_mem_free(sa);
			hg_mem_free(se);
		}
		hg_mem_free(sexp);
		HG_VALUE_MAKE_BOOLEAN (pool, n, result);
		hg_stack_pop(ostack);
		retval = hg_stack_push(ostack, n);
		/* it must be true */
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

	for (i = 0; i < HG_test_op_END; i++) {
		__test_operator_list[i] = NULL;
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
	HgStack *estack;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		hg_log_warning("already loaded.");
		return FALSE;
	}
	systemdict = hg_vm_get_dict_systemdict(vm);
	is_global = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);

	BUILD_OP (vm, pool, systemdict, .validateunittest, validateunittest);

	estack = hg_stack_new(pool, 256);
	hg_vm_eval(vm, "{(hg_unittest.ps) runlibfile} stopped pop",
		   NULL, estack, NULL, &error);
	hg_mem_free(estack);
	hg_vm_use_global_pool(vm, is_global);

	plugin->user_data = systemdict;

	return error == FALSE;
}

static gboolean
plugin_unload(HgPlugin *plugin,
	      HgVM     *vm)
{
	HgMemPool *pool;
	HgDict *systemdict;
	gboolean is_global;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		hg_log_warning("not yet loaded.");
		return FALSE;
	}

	systemdict = hg_vm_get_dict_systemdict(vm);
	is_global = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);

	REMOVE_OP (pool, systemdict, .validateunittest, validateunittest);
	hg_vm_use_global_pool(vm, is_global);

	plugin->user_data = NULL;

	return TRUE;
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
