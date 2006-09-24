/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * debug-main.c
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

#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgplugins.h>
#include <hieroglyph/hgstack.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/operator.h>
#include <hieroglyph/vm.h>

#define BUILD_OP(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		hg_operator_build_operator__inline(_debug_op_, vm, pool, sdict, name, func, __hg_op); \
		if (__hg_op != NULL) {					\
			__debug_operator_list[HG_debug_op_##func] = __hg_op; \
			hg_mem_add_root_node(pool, __hg_op);		\
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
			if (__hg_op == __debug_operator_list[HG_debug_op_##func]) { \
				hg_dict_remove(sdict, __hg_key);	\
			}						\
			hg_mem_remove_root_node(pool, __hg_op);		\
			hg_mem_free(__hg_op);				\
		}							\
		hg_mem_free(__hg_key);					\
	} G_STMT_END
#define DEFUNC_OP(func)					\
	static gboolean					\
	_debug_op_##func(HgOperator *op,		\
			 gpointer    data)		\
	{						\
		HgVM *vm = data;			\
		gboolean retval = FALSE;
#define DEFUNC_OP_END				\
		return retval;			\
	}

typedef enum {
	HG_debug_op_breakpoint = 0,
	HG_debug_op_startgc,
	HG_debug_op_END
} HgOperatorDebugEncoding;


static gboolean  plugin_init                             (void);
static gboolean  plugin_finalize                         (void);
static gboolean  plugin_load                             (HgPlugin *plugin,
							  HgVM     *vm);
static gboolean  plugin_unload                           (HgPlugin *plugin,
							  HgVM     *vm);


static HgPluginVTable vtable = {
	.init     = plugin_init,
	.finalize = plugin_finalize,
	.load     = plugin_load,
	.unload   = plugin_unload,
};
HgOperator *__debug_operator_list[HG_debug_op_END];


/*
 * Private Functions
 */
DEFUNC_OP (breakpoint)
G_STMT_START
{
	(void)vm;
	retval = TRUE;
	G_BREAKPOINT();
} G_STMT_END;
DEFUNC_OP_END

DEFUNC_OP (startgc)
G_STMT_START
{
	HgMemPool *pool;
	HgStack *ostack = hg_vm_get_ostack(vm);
	gboolean result, flag = hg_vm_is_global_pool_used(vm);
	HgValueNode *node;

	/* do GC for only local pool */
	hg_vm_use_global_pool(vm, FALSE);
	pool = hg_vm_get_current_pool(vm);
	result = hg_mem_garbage_collection(pool);
	HG_VALUE_MAKE_BOOLEAN (pool, node, result);
	hg_vm_use_global_pool(vm, flag);

	retval = hg_stack_push(ostack, node);
	if (!retval)
		_hg_operator_set_error(vm, op, VM_e_stackoverflow);
} G_STMT_END;
DEFUNC_OP_END

static gboolean
plugin_init(void)
{
	gint i;

	for (i = 0; i < HG_debug_op_END; i++) {
		__debug_operator_list[i] = NULL;
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
	gboolean is_global;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (vm != NULL, FALSE);

	if (plugin->user_data != NULL) {
		g_warning("already loaded.");
		return FALSE;
	}
	systemdict = hg_vm_get_dict_systemdict(vm);
	is_global = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);

	BUILD_OP (vm, pool, systemdict, .breakpoint, breakpoint);
	BUILD_OP (vm, pool, systemdict, .startgc, startgc);

	hg_vm_use_global_pool(vm, is_global);

	plugin->user_data = systemdict;

	return TRUE;
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
		g_warning("not yet loaded.");
		return FALSE;
	}

	systemdict = hg_vm_get_dict_systemdict(vm);
	is_global = hg_vm_is_global_pool_used(vm);
	hg_vm_use_global_pool(vm, TRUE);
	pool = hg_vm_get_current_pool(vm);

	REMOVE_OP (pool, systemdict, .startgc, startgc);
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
