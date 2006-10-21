/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * vm.c
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "vm.h"
#include "hgallocator-bfit.h"
#include "hgmem.h"
#include "hgarray.h"
#include "hgdict.h"
#include "hgfile.h"
#include "hglineedit.h"
#include "hglist.h"
#include "hgplugins.h"
#include "hgstack.h"
#include "hgstring.h"
#include "hgvaluenode.h"
#include "hggraphics.h"
#include "operator.h"
#include "scanner.h"


#define _hg_vm_set_error(v, o, e, d)					\
	G_STMT_START {							\
		HgValueNode *__hg_op_node;				\
									\
		HG_VALUE_MAKE_OPERATOR (__hg_op_node, (o));		\
		hg_vm_set_error((v), __hg_op_node, (e), (d));		\
	} G_STMT_END
#define _hg_vm_set_error_from_file(v, o, e, d)				\
	G_STMT_START {							\
		HgValueNode *__hg_op_node;				\
									\
		HG_VALUE_MAKE_OPERATOR (__hg_op_node, (o));		\
		hg_vm_set_error_from_file((v), __hg_op_node, (e), (d)); \
	} G_STMT_END


struct _HieroGlyphVirtualMachine {
	HgObject  object;

	/* vm */
	HgVMEmulationType  emulation_type;
	gboolean           use_global_pool : 1;
	gboolean           has_error : 1;
	HgDict            *name_dict;
	GTimeVal           initialized_time;
	GRand             *random_generator;
	gint32             error_code;
	gboolean           shutdown;
	gint32             security_level;

	/* job management */
	GList             *saved_jobs;
	GList             *global_snapshot;
	GList             *local_snapshot;

	/* memory pool */
	HgAllocator       *local_allocator;
	HgAllocator       *global_allocator;
	HgAllocator       *graphic_allocator;
	HgMemPool         *local_pool;
	HgMemPool         *global_pool;
	HgMemPool         *graphic_pool;

	/* stacks */
	HgStack           *ostack;
	HgStack           *estack;
	HgStack           *dstack;

	/* local dictionaries */
	HgDict            *errordict;
	HgDict            *error;
	HgDict            *statusdict;
	HgDict            *serverdict;
	HgDict            *font;

	/* global dictionaries */
	HgDict            *systemdict;
	HgDict            *globalfont;

	/* file */
	HgFileObject      *stdin;
	HgFileObject      *stdout;
	HgFileObject      *stderr;
	HgLineEdit        *lineeditor;

	/* graphics */
	HgGraphics        *graphics;

	/* plugin */
	HgDict            *plugin_table;
	GList             *plugin_list;
};


static void _hg_vm_real_free    (gpointer           data);
static void _hg_vm_real_relocate(gpointer           data,
				 HgMemRelocateInfo *info);


static gboolean __hg_vm_is_initialized = FALSE;
static HgAllocator *__hg_vm_allocator = NULL;
static HgMemPool *__hg_vm_mem_pool = NULL;
HgValueNode *__hg_vm_errorname[VM_e_END] = { NULL };
static HgObjectVTable __hg_vm_vtable = {
	.free      = _hg_vm_real_free,
	.set_flags = NULL,
	.relocate  = _hg_vm_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * special operators
 */
static gboolean
_hg_vm_op_rollbacksecuritylevel(HgOperator *op,
				gpointer    data)
{
	HgVM *vm = data;
	gboolean retval = TRUE;
	HgStack *estack = hg_vm_get_estack(vm);
	guint edepth = hg_stack_depth(estack);
	HgValueNode *nself, *node;

	/* ignore the errors */
	if (edepth > 2) {
		nself = hg_stack_index(estack, 0);
		node = hg_stack_index(estack, 1);
		if (HG_IS_VALUE_INTEGER (node)) {
			vm->security_level = HG_VALUE_GET_INTEGER (node);
			hg_stack_pop(estack);
			hg_stack_pop(estack);
			retval = hg_stack_push(estack, nself); /* dummy */
			/* it must be true */
		}
	}

	return retval;
}

/*
 * Private Functions
 */
static void
_hg_vm_init_errorname(void)
{
#define MAKE_ERRNAME(sym)						\
	G_STMT_START {							\
		HgValueNode *__hg_vm_err_node;				\
									\
		HG_VALUE_MAKE_NAME_STATIC (__hg_vm_mem_pool,		\
					   __hg_vm_err_node,		\
					   #sym);			\
		__hg_vm_errorname[VM_e_##sym] = __hg_vm_err_node;	\
		hg_mem_add_root_node(__hg_vm_mem_pool, __hg_vm_err_node); \
	} G_STMT_END

	MAKE_ERRNAME(dictfull);
	MAKE_ERRNAME(dictstackoverflow);
	MAKE_ERRNAME(dictstackunderflow);
	MAKE_ERRNAME(execstackoverflow);
	MAKE_ERRNAME(handleerror);
	MAKE_ERRNAME(interrupt);
	MAKE_ERRNAME(invalidaccess);
	MAKE_ERRNAME(invalidexit);
	MAKE_ERRNAME(invalidfileaccess);
	MAKE_ERRNAME(invalidfont);
	MAKE_ERRNAME(invalidrestore);
	MAKE_ERRNAME(ioerror);
	MAKE_ERRNAME(limitcheck);
	MAKE_ERRNAME(nocurrentpoint);
	MAKE_ERRNAME(rangecheck);
	MAKE_ERRNAME(stackoverflow);
	MAKE_ERRNAME(stackunderflow);
	MAKE_ERRNAME(syntaxerror);
	MAKE_ERRNAME(timeout);
	MAKE_ERRNAME(typecheck);
	MAKE_ERRNAME(undefined);
	MAKE_ERRNAME(undefinedfilename);
	MAKE_ERRNAME(undefinedresult);
	MAKE_ERRNAME(unmatchedmark);
	MAKE_ERRNAME(unregistered);
	MAKE_ERRNAME(VMerror);
	MAKE_ERRNAME(configurationerror);
	MAKE_ERRNAME(undefinedresource);

#undef MAKE_ERRNAME
}

static void
_hg_vm_real_free(gpointer data)
{
	HgVM *vm = data;

	if (vm->saved_jobs)
		g_list_free(vm->saved_jobs);
	if (vm->random_generator)
		g_rand_free(vm->random_generator);
	if (vm->global_snapshot)
		g_list_free(vm->global_snapshot);
	if (vm->local_snapshot)
		g_list_free(vm->local_snapshot);
	if (vm->local_pool)
		hg_mem_pool_destroy(vm->local_pool);
#ifdef ENABLE_GLOBAL_POOL
	if (vm->global_pool)
		hg_mem_pool_destroy(vm->global_pool);
#endif
	if (vm->graphic_pool)
		hg_mem_pool_destroy(vm->graphic_pool);
	hg_allocator_destroy(vm->local_allocator);
	hg_allocator_destroy(vm->global_allocator);
	hg_allocator_destroy(vm->graphic_allocator);
}

static void
_hg_vm_real_relocate(gpointer           data,
		     HgMemRelocateInfo *info)
{
	HgVM *vm = data;

#define _hg_vm_update_addr(_name)					\
	G_STMT_START {							\
		if ((gsize)vm->_name >= info->start &&			\
		    (gsize)vm->_name <= info->end) {			\
			vm->_name = (gpointer)((gsize)vm->_name + info->diff); \
		}							\
	} G_STMT_END

	_hg_vm_update_addr(name_dict);
	_hg_vm_update_addr(ostack);
	_hg_vm_update_addr(estack);
	_hg_vm_update_addr(dstack);
	_hg_vm_update_addr(errordict);
	_hg_vm_update_addr(error);
	_hg_vm_update_addr(statusdict);
	_hg_vm_update_addr(serverdict);
	_hg_vm_update_addr(font);
	_hg_vm_update_addr(systemdict);
	_hg_vm_update_addr(globalfont);
	_hg_vm_update_addr(stdin);
	_hg_vm_update_addr(stdout);
	_hg_vm_update_addr(stderr);

#undef _hg_vm_update_addr
}

static gboolean
_hg_vm_eval_file(HgVM         *vm,
		 HgFileObject *file,
		 gboolean     *error)
{
	HgStack *estack = hg_vm_get_estack(vm);
	gboolean retval = FALSE;
	HgValueNode *node;

	while (1) {
		if (!hg_file_object_has_error(file)) {
			HG_VALUE_MAKE_FILE (node, file);
			if (node == NULL) {
				_hg_vm_set_error(vm, file, VM_e_VMerror, FALSE);
				*error = TRUE;
				break;
			}
			hg_stack_clear(estack);
			hg_vm_reset_error(vm);
			hg_object_executable((HgObject *)node);
			retval = hg_stack_push(estack, node);
			if (!retval) {
				hg_vm_set_error(vm, node, VM_e_execstackoverflow, FALSE);
				*error = TRUE;
				break;
			}
			hg_vm_main(vm);
			hg_mem_free(node);
			hg_mem_free(file);
			if (hg_vm_has_error(vm) ||
			    ((node = hg_dict_lookup_with_string(vm->error, "newerror")) != NULL &&
			     HG_IS_VALUE_BOOLEAN (node) &&
			     HG_VALUE_GET_BOOLEAN (node) == TRUE)) {
				if (error)
					*error = TRUE;
			} else {
				retval = TRUE;
				if (error)
					*error = FALSE;
			}
		}
		break;
	}

	return retval;
}

static gboolean
_hg_vm_run(HgVM        *vm,
	   const gchar *filename,
	   gboolean    *error)
{
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgFileObject *file = hg_file_object_new(pool,
						HG_FILE_TYPE_FILE,
						HG_FILE_MODE_READ,
						filename);

	return _hg_vm_eval_file(vm, file, error);
}

static void
hg_vm_set_ostack(HgVM    *vm,
		 HgStack *ostack)
{
	g_return_if_fail (vm != NULL);
	g_return_if_fail (ostack != NULL);

	vm->ostack = ostack;
}

static void
hg_vm_set_estack(HgVM    *vm,
		 HgStack *estack)
{
	g_return_if_fail (vm != NULL);
	g_return_if_fail (estack != NULL);

	vm->estack = estack;
}

static void
hg_vm_set_dstack(HgVM    *vm,
		 HgStack *dstack)
{
	g_return_if_fail (vm != NULL);
	g_return_if_fail (dstack != NULL);

	vm->dstack = dstack;
}

/*
 * Public Functions
 */

/* initializer */
void
hg_vm_init(void)
{
	if (!__hg_vm_is_initialized) {
		hg_mem_init();
		hg_value_node_init();
		hg_file_init();
		hg_list_init();

		__hg_vm_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
		/* XXX: Ordinarily this pool should be a global pool according
		 *      to this purpose. However it causes the pool check error
		 *      to store the objects to the object, such as the node to
		 *      the array when it happens during the error handling
		 *      so that hg_vm_get_current_pool() returns
		 *      __hg_vm_mem_pool to avoid another VMerror.
		 */
		__hg_vm_mem_pool = hg_mem_pool_new(__hg_vm_allocator,
						   "Hieroglyph VM Memory Pool",
						   8192,
						   HG_MEM_RESIZABLE);
		hg_mem_pool_use_garbage_collection(__hg_vm_mem_pool, FALSE);
		_hg_vm_init_errorname();
		__hg_vm_is_initialized = TRUE;
	}
}

void
hg_vm_finalize(void)
{
	if (__hg_vm_is_initialized) {
		hg_mem_pool_destroy(__hg_vm_mem_pool);
		hg_allocator_destroy(__hg_vm_allocator);

		hg_list_finalize();
		hg_file_finalize();
		hg_value_node_finalize();
		hg_mem_finalize();

		__hg_vm_is_initialized = FALSE;
	}
}

gboolean
hg_vm_is_initialized(void)
{
	return __hg_vm_is_initialized;
}

/* virtual machine */
HgVM *
hg_vm_new(HgVMEmulationType type)
{
	HgVM *retval;
	gchar *name;
	gboolean allow_resize = FALSE;

	g_return_val_if_fail (__hg_vm_is_initialized, NULL);

	retval = hg_mem_alloc_with_flags(__hg_vm_mem_pool, sizeof (HgVM),
					 HG_FL_HGOBJECT);
	if (retval == NULL) {
		g_warning("Failed to create a virtual machine.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(__hg_vm_mem_pool));
	hg_object_set_vtable(&retval->object, &__hg_vm_vtable);

	retval->emulation_type = type;
	retval->use_global_pool = FALSE;
	retval->has_error = FALSE;
	retval->shutdown = FALSE;
	retval->error_code = 0;

	if (type >= VM_EMULATION_LEVEL_2) {
		allow_resize = TRUE;
		retval->use_global_pool = TRUE;
	}

	/* initialize the memory pools */
	retval->local_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
	retval->global_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
	retval->graphic_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
	name = g_strdup_printf("VM %p:local pool", retval);
	retval->local_pool = hg_mem_pool_new(retval->local_allocator,
					     name,
					     HG_LOCAL_POOL_SIZE,
					     (allow_resize ? HG_MEM_RESIZABLE : 0));
	g_free(name);
	if (retval->local_pool == NULL) {
		g_warning("Failed to create a local memory pool");
		return NULL;
	}
#ifdef ENABLE_GLOBAL_POOL
	name = g_strdup_printf("VM %p: global pool", retval);
	retval->global_pool = hg_mem_pool_new(retval->global_allocator,
					      name,
					      HG_GLOBAL_POOL_SIZE,
					      HG_MEM_GLOBAL | (allow_resize ? HG_MEM_RESIZABLE : 0));
	hg_mem_pool_use_global_mode(retval->global_pool, TRUE);
	g_free(name);
	if (retval->global_pool == NULL) {
		g_warning("Failed to create a global memory pool");
		return NULL;
	}
#else
	retval->global_pool = retval->local_pool;
#endif
	name = g_strdup_printf("VM %p:graphic pool", retval);
	retval->graphic_pool = hg_mem_pool_new(retval->graphic_allocator,
					       name,
					       HG_GRAPHIC_POOL_SIZE,
					       (allow_resize ? HG_MEM_RESIZABLE : 0));
	g_free(name);
	if (retval->graphic_pool == NULL) {
		g_warning("Failed to create a graphic memory pool");
		return NULL;
	}

	/* internal use */
	retval->name_dict = hg_dict_new(retval->global_pool, 1024);
	if (retval->name_dict == NULL) {
		g_warning("Failed to create a name dict.");
		return NULL;
	}
	hg_mem_add_root_node(retval->global_pool, retval->name_dict);

	g_get_current_time(&retval->initialized_time);
	retval->random_generator = g_rand_new();

	/* snapshots */
	retval->saved_jobs = NULL;
	retval->global_snapshot = NULL;
	retval->local_snapshot = NULL;

	/* initialize the stacks */
	retval->ostack = hg_stack_new(retval->local_pool, 500);
	retval->estack = hg_stack_new(retval->local_pool, 250);
	retval->dstack = hg_stack_new(retval->local_pool, 20);
	if (retval->ostack == NULL ||
	    retval->estack == NULL ||
	    retval->dstack == NULL) {
		g_warning("Failed to create stacks.");
		return NULL;
	}
	hg_mem_add_root_node(retval->local_pool, retval->ostack);
	hg_mem_add_root_node(retval->local_pool, retval->estack);
	hg_mem_add_root_node(retval->local_pool, retval->dstack);

	/* initialize local dictionaries */
	retval->errordict = hg_dict_new(retval->local_pool, 128);
	retval->error = hg_dict_new(retval->local_pool, 128);
	retval->statusdict = hg_dict_new(retval->local_pool, 128);
	retval->serverdict = hg_dict_new(retval->local_pool, 128);
	retval->font = hg_dict_new(retval->local_pool, 128);
	if (retval->errordict == NULL ||
	    retval->error == NULL ||
	    retval->statusdict == NULL ||
	    retval->serverdict == NULL ||
	    retval->font == NULL) {
		g_warning("Failed to create local dictionaries.");
		return NULL;
	}
	hg_mem_add_root_node(retval->local_pool, retval->errordict);
	hg_mem_add_root_node(retval->local_pool, retval->error);
	hg_mem_add_root_node(retval->local_pool, retval->statusdict);
	hg_mem_add_root_node(retval->local_pool, retval->serverdict);
	hg_mem_add_root_node(retval->local_pool, retval->font);

	/* initialize global dictionaries */
	retval->systemdict = hg_dict_new(retval->global_pool, 384);
	retval->globalfont = hg_dict_new(retval->global_pool, 128);
	if (retval->systemdict == NULL ||
	    retval->globalfont == NULL) {
		g_warning("Failed to create global dictionaries.");
		return NULL;
	}
	hg_mem_add_root_node(retval->global_pool, retval->systemdict);
	hg_mem_add_root_node(retval->global_pool, retval->globalfont);

	/* initialize file object */
	retval->stdin = __hg_file_stdin;
	retval->stdout = __hg_file_stdout;
	retval->stderr = __hg_file_stderr;
	retval->lineeditor = hg_line_edit_new(retval->local_pool, NULL,
					      retval->stdin, retval->stdout);
	hg_mem_add_root_node(retval->local_pool, retval->lineeditor);

	/* initialize graphics */
	retval->graphics = hg_graphics_new(retval->graphic_pool);
	hg_mem_add_root_node(retval->graphic_pool, retval->graphics);

	/* plugin support */
	retval->plugin_table = hg_dict_new(retval->local_pool, 256);
	retval->plugin_list = NULL;
	hg_mem_add_root_node(retval->local_pool, retval->plugin_table);

	hg_mem_add_root_node(__hg_vm_mem_pool, retval);

	return retval;
}

void
hg_vm_set_emulation_level(HgVM              *vm,
			  HgVMEmulationType  type)
{
	gboolean allow_resize = FALSE;

	g_return_if_fail (vm != NULL);

	vm->emulation_type = type;
	if (type >= VM_EMULATION_LEVEL_2)
		allow_resize = TRUE;

	hg_mem_pool_allow_resize(vm->global_pool, allow_resize);
	hg_mem_pool_allow_resize(vm->local_pool, allow_resize);
	hg_vm_startjob(vm, NULL, FALSE);
}

HgVMEmulationType
hg_vm_get_emulation_level(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, VM_EMULATION_LEVEL_1);

	return vm->emulation_type;
}

HgStack *
hg_vm_get_ostack(HgVM *vm)
{
	return vm->ostack;
}

HgStack *
hg_vm_get_estack(HgVM *vm)
{
	return vm->estack;
}

HgStack *
hg_vm_get_dstack(HgVM *vm)
{
	return vm->dstack;
}

HgDict *
hg_vm_get_dict_errordict(HgVM *vm)
{
	return vm->errordict;
}

HgDict *
hg_vm_get_dict_error(HgVM *vm)
{
	return vm->error;
}

HgDict *
hg_vm_get_dict_statusdict(HgVM *vm)
{
	return vm->statusdict;
}

HgDict *
hg_vm_get_dict_serverdict(HgVM *vm)
{
	return vm->serverdict;
}

HgDict *
hg_vm_get_dict_font(HgVM *vm)
{
	return vm->font;
}

HgDict *
hg_vm_get_dict_systemdict(HgVM *vm)
{
	return vm->systemdict;
}

HgDict *
hg_vm_get_dict_globalfont(HgVM *vm)
{
	return vm->globalfont;
}

HgMemPool *
hg_vm_get_current_pool(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, NULL);

	if (vm->has_error) {
		/* return a special memory pool to avoid recursive VMerror */
		return __hg_vm_mem_pool;
	}
	if (vm->use_global_pool)
		return vm->global_pool;

	return vm->local_pool;
}

gboolean
hg_vm_is_global_pool_used(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, FALSE);

	return vm->use_global_pool;
}

void
hg_vm_use_global_pool(HgVM     *vm,
		      gboolean  use_global)
{
	g_return_if_fail (vm != NULL);

	vm->use_global_pool = use_global;
}

gboolean
hg_vm_is_global_object(HgVM        *vm,
		       HgValueNode *node)
{
	HgMemObject *obj;
	gboolean retval = FALSE;
	gpointer p;

	g_return_val_if_fail (vm != NULL, TRUE);
	g_return_val_if_fail (node != NULL, TRUE);

	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
	    case HG_TYPE_VALUE_INTEGER:
	    case HG_TYPE_VALUE_REAL:
	    case HG_TYPE_VALUE_NAME:
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_OPERATOR:
	    case HG_TYPE_VALUE_MARK:
		    break;
	    case HG_TYPE_VALUE_ARRAY:
	    case HG_TYPE_VALUE_STRING:
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
		    p = HG_VALUE_GET_VALUE_NODE (node, Pointer);
		    hg_mem_get_object__inline(p, obj);
		    retval = (vm->local_pool != obj->pool);
		    break;
	    default:
		    g_warning("Failed to consider if the object is a global object due to the unknown type ID: %d", HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}

	return retval;
}

HgGraphics *
hg_vm_get_graphics(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, NULL);

	return vm->graphics;
}

gint32
hg_vm_get_current_time(HgVM *vm)
{
	GTimeVal t;
	gint32 retval, i;

	g_get_current_time(&t);
	if (t.tv_sec == vm->initialized_time.tv_sec) {
		retval = (t.tv_usec - vm->initialized_time.tv_usec) / 1000;
	} else {
		i = t.tv_sec - vm->initialized_time.tv_sec;
		if (t.tv_usec > vm->initialized_time.tv_usec) {
			retval = i * 1000 + (t.tv_usec - vm->initialized_time.tv_usec) / 1000;
		} else {
			i--;
			retval = i * 1000 + ((1 * 1000000 - vm->initialized_time.tv_usec) + t.tv_usec) / 1000;
		}
	}

	return retval;
}

GRand *
hg_vm_get_random_generator(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, 0);

	return vm->random_generator;
}

HgFileObject *
hg_vm_get_io(HgVM       *vm,
	     HgVMIOType  type)
{
	g_return_val_if_fail (vm != NULL, NULL);

	switch (type) {
	    case VM_IO_STDIN:
		    return vm->stdin;
	    case VM_IO_STDOUT:
		    return vm->stdout;
	    case VM_IO_STDERR:
		    return vm->stderr;
	    default:
		    g_warning("Unknown I/O type %d", type);
		    break;
	}

	return NULL;
}

void
hg_vm_set_io(HgVM         *vm,
	     HgVMIOType    type,
	     HgFileObject *file)
{
	g_return_if_fail (vm != NULL);
	g_return_if_fail (file != NULL);

	switch (type) {
	    case VM_IO_STDIN:
		    vm->stdin = file;
		    hg_line_edit_set_stdin(vm->lineeditor, file);
		    break;
	    case VM_IO_STDOUT:
		    vm->stdout = file;
		    hg_line_edit_set_stdout(vm->lineeditor, file);
		    break;
	    case VM_IO_STDERR:
		    vm->stderr = file;
		    break;
	    default:
		    g_warning("Unknown I/O type %d", type);
		    break;
	}
}

HgLineEdit *
hg_vm_get_line_editor(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, NULL);

	return vm->lineeditor;
}

void
hg_vm_set_line_editor(HgVM       *vm,
		      HgLineEdit *editor)
{
	g_return_if_fail (vm != NULL);
	g_return_if_fail (editor != NULL);

	vm->lineeditor = editor;
}

gboolean
hg_vm_load_plugin(HgVM        *vm,
		  const gchar *filename)
{
	HgPlugin *plugin;
	HgValueNode *key, *val;

	g_return_val_if_fail (vm != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (hg_dict_lookup_with_string(vm->plugin_table, filename) != NULL) {
		g_warning("`%s' plugin is already loaded.", filename);
		return FALSE;
	}
	plugin = hg_plugin_open(vm->local_pool, filename, HG_PLUGIN_EXTENSION);
	if (plugin) {
		HG_VALUE_MAKE_NAME_STATIC (vm->local_pool, key, filename);
		HG_VALUE_MAKE_PLUGIN (val, plugin);
		hg_dict_insert(vm->local_pool, vm->plugin_table, key, val);
		vm->plugin_list = g_list_append(vm->plugin_list, plugin);
	}

	return plugin != NULL;
}

void
hg_vm_load_plugins_all(HgVM *vm)
{
	GList *l;

	/* load all the plugins after the VM initialization is finished */
	for (l = vm->plugin_list; l != NULL; l = g_list_next(l)) {
		HgPlugin *plugin = l->data;

		hg_plugin_load(plugin, vm);
	}
}

gboolean
hg_vm_unload_plugin(HgVM        *vm,
		    const gchar *filename)
{
	HgValueNode *val;

	g_return_val_if_fail (vm != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if ((val = hg_dict_lookup_with_string(vm->plugin_table, filename)) == NULL) {
		g_warning("`%s' plugin isn't yet loaded.", filename);
		return FALSE;
	}

	return hg_plugin_unload(HG_VALUE_GET_PLUGIN (val), vm);
}

guint
hg_vm_get_save_level(HgVM *vm)
{
	guint retval;

	g_return_val_if_fail (vm != NULL, 0);

	if (hg_vm_is_global_pool_used(vm)) {
		retval = g_list_length(vm->global_snapshot) + g_list_length(vm->saved_jobs);
	} else {
		retval = g_list_length(vm->local_snapshot) + g_list_length(vm->saved_jobs);
	}

	return retval;
}

HgValueNode *
_hg_vm_get_name_node(HgVM        *vm,
		     const gchar *name)
{
	HgValueNode *node;

	g_return_val_if_fail (vm != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	node = hg_dict_lookup_with_string(vm->name_dict, name);
	if (node == NULL) {
		HG_VALUE_MAKE_NAME_STATIC (vm->global_pool, node, name);
		hg_dict_insert(vm->global_pool, vm->name_dict, node, node);
	}

	return node;
}

HgValueNode *
hg_vm_get_name_node(HgVM        *vm,
		    const gchar *name)
{
	HgValueNode *retval = NULL, *node;

	node = _hg_vm_get_name_node(vm, name);
	if (node) {
		HG_VALUE_MAKE_NAME (retval, HG_VALUE_GET_NAME (node));
	}

	return retval;
}

HgValueNode *
hg_vm_lookup(HgVM        *vm,
	     HgValueNode *key)
{
	guint depth, i;
	HgValueNode *node, *retval = NULL;

	g_return_val_if_fail (vm != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	depth = hg_stack_depth(vm->dstack);
	for (i = 0; i < depth; i++) {
		HgDict *dict;

		node = hg_stack_index(vm->dstack, i);
		if (node == NULL || !HG_IS_VALUE_DICT (node)) {
			g_error("dictionary stack was broken.");
			break;
		}
		dict = HG_VALUE_GET_DICT (node);
		if ((retval = hg_dict_lookup(dict, key)) != NULL)
			break;
	}

	return retval;
}

HgValueNode *
hg_vm_lookup_with_string(HgVM        *vm,
			 const gchar *key)
{
	guint depth, i;
	HgValueNode *node, *retval = NULL;

	g_return_val_if_fail (vm != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	depth = hg_stack_depth(vm->dstack);
	for (i = 0; i < depth; i++) {
		node = hg_stack_index(vm->dstack, i);
		if (node == NULL || !HG_IS_VALUE_DICT (node)) {
			g_error("dictionary stack was broken.");
			break;
		}
		if ((retval = hg_dict_lookup_with_string(HG_VALUE_GET_DICT (node), key)) != NULL)
			break;
	}

	return retval;
}

gboolean
hg_vm_startjob(HgVM        *vm,
	       const gchar *initializer,
	       gboolean     encapsulated)
{
	HgValueNode *node, *nkey;
	GList *l;
	gboolean retval = TRUE;

	g_return_val_if_fail (vm != NULL, FALSE);

	if (vm->saved_jobs) {
		g_list_free(vm->saved_jobs);
	}
	if (g_list_length(vm->global_snapshot) > 0) {
		l = g_list_last(vm->global_snapshot);
		hg_mem_pool_restore_snapshot(vm->global_pool, l->data);
		vm->global_snapshot = g_list_delete_link(vm->global_snapshot, l);
	}
	if (g_list_length(vm->local_snapshot) > 0) {
		l = g_list_last(vm->local_snapshot);
		hg_mem_pool_restore_snapshot(vm->local_pool, l->data);
		vm->local_snapshot = g_list_delete_link(vm->local_snapshot, l);
	}

	hg_stack_clear(vm->ostack);
	hg_stack_clear(vm->dstack);

	hg_mem_garbage_collection(vm->global_pool);
	hg_mem_garbage_collection(vm->local_pool);

	if (encapsulated) {
		HgMemSnapshot *gsnapshot = hg_mem_pool_save_snapshot(vm->global_pool);
		HgMemSnapshot *lsnapshot = hg_mem_pool_save_snapshot(vm->local_pool);

		vm->global_snapshot = g_list_append(vm->global_snapshot, gsnapshot);
		vm->local_snapshot = g_list_append(vm->local_snapshot, lsnapshot);
	}

	HG_VALUE_MAKE_DICT (node, vm->systemdict);
	hg_stack_push(vm->dstack, node);

	if (!hg_operator_init(vm))
		return FALSE;

	/* initialize the special operators */
#define BUILD_OP(_vm_, _pool_, _sdict_, _name_, _func_)			\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		hg_operator_build_operator__inline(_hg_vm_op_, (_vm_), (_pool_), (_sdict_), _name_, _func_, __hg_op); \
	} G_STMT_END

	BUILD_OP (vm, vm->global_pool, vm->systemdict, %rollback_securitylevel, rollbacksecuritylevel);

#undef BUILD_OP

	/* FIXME: initialize graphics */

	hg_vm_use_global_pool(vm, FALSE);

	HG_VALUE_MAKE_NAME_STATIC (vm->local_pool, nkey, "%initialized");
	node = hg_dict_lookup(vm->statusdict, nkey);
	if (node == NULL ||
	    !HG_IS_VALUE_BOOLEAN (node) ||
	    HG_VALUE_GET_BOOLEAN (node) == FALSE) {
		HgValueNode *ntrue = hg_dict_lookup_with_string(vm->systemdict, "true");

		retval = hg_vm_run(vm, "hg_init.ps");

		/* set read-only attribute for systemdict */
		hg_object_unwritable((HgObject *)vm->systemdict);

		/* initialization is done */
		hg_dict_insert(vm->local_pool, vm->serverdict, nkey, ntrue);
	}

	if (initializer && initializer[0] == 0) {
		/* just initialize VM */
		return TRUE;
	} else if (initializer) {
		gchar *eval = g_strdup_printf("{prompt (%s)(r)file dup type /filetype eq {cvx exec} if} stopped {$error /newerror get {errordict /handleerror get exec 1 .quit} if} if", initializer);

		retval = hg_vm_eval(vm, eval, NULL, NULL, NULL, NULL);
		g_free(eval);

		return retval;
	}
	return (retval ? hg_vm_eval(vm, "systemdict /.loadhistory known {(.hghistory) .loadhistory} if start systemdict /.savehistory known {(.hghistory) .savehistory} if", NULL, NULL, NULL, NULL) : FALSE);
}

gboolean
hg_vm_has_error(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, TRUE);

	return vm->has_error;
}

void
hg_vm_clear_error(HgVM *vm)
{
	g_return_if_fail (vm != NULL);

	vm->has_error = FALSE;
}

void
hg_vm_reset_error(HgVM *vm)
{
	HgValueNode *key, *nfalse, *nnull;

	g_return_if_fail (vm != NULL);

	hg_vm_clear_error(vm);
	_hg_stack_use_stack_validator(vm->ostack, TRUE);
	_hg_stack_use_stack_validator(vm->estack, TRUE);
	_hg_stack_use_stack_validator(vm->dstack, TRUE);
	nnull = hg_dict_lookup_with_string(vm->systemdict, "null");
	nfalse = hg_dict_lookup_with_string(vm->systemdict, "false");
	key = _hg_vm_get_name_node(vm, "newerror");
	hg_dict_insert(vm->local_pool, vm->error, key, nfalse);
	key = _hg_vm_get_name_node(vm, ".isstop");
	hg_dict_insert(vm->local_pool, vm->error, key, nfalse);
	key = _hg_vm_get_name_node(vm, "errorname");
	hg_dict_insert(vm->local_pool, vm->error, key, nnull);
}

void
hg_vm_set_error(HgVM        *vm,
		HgValueNode *errnode,
		HgVMError    error,
		gboolean     need_self)
{
	HgValueNode *proc, *copy_proc, *self;

	g_return_if_fail (vm != NULL);
	g_return_if_fail (errnode != NULL);
	g_return_if_fail (error < VM_e_END);

	if (vm->has_error) {
		HgValueNode *enode = hg_dict_lookup_with_string(vm->error, "errorname");
		HgValueNode *wnode = hg_dict_lookup_with_string(vm->error, "command");
		HgString *hs = NULL, *hs2;
		gchar *name, *p = NULL;

		if (wnode == NULL) {
			p = name = g_strdup("--unknown--");
		} else {
			hs = hg_object_to_string((HgObject *)wnode);
			name = hg_string_get_string(hs);
		}
		hs2 = hg_object_to_string((HgObject *)errnode);
		if (enode == NULL) {
			g_warning("Multiple errors occurred.\n  prevoius error: unknown or this happened before set /errorname in %s\n  current error: %s in %s\n", name, HG_VALUE_GET_NAME (__hg_vm_errorname[error]), hg_string_get_string(hs2));
		} else {
			g_warning("Multiple errors occurred.\n  previous error: %s in %s\n  current error: %s in %s\n", HG_VALUE_GET_NAME (enode), name, HG_VALUE_GET_NAME (__hg_vm_errorname[error]), hg_string_get_string(hs2));
		}
		if (p)
			g_free(p);
		hg_mem_free(hs2);
		hg_mem_free(hs);
		proc = hg_dict_lookup_with_string(vm->systemdict, ".abort");
		hg_operator_invoke(HG_VALUE_GET_OPERATOR (proc), vm);
	}
	_hg_stack_use_stack_validator(vm->ostack, FALSE);
	_hg_stack_use_stack_validator(vm->estack, FALSE);
	_hg_stack_use_stack_validator(vm->dstack, FALSE);
	proc = hg_dict_lookup(vm->errordict, __hg_vm_errorname[error]);
	if (proc == NULL) {
		g_warning("[BUG] no error handler for /%s\n", HG_VALUE_GET_NAME (__hg_vm_errorname[error]));
		hg_file_object_printf(vm->stderr, "\nOperand stack:\n");
		hg_stack_dump(vm->ostack, vm->stderr);
		hg_file_object_printf(vm->stderr, "\nExecution stack:\n");
		hg_stack_dump(vm->estack, vm->stderr);
		hg_file_object_printf(vm->stderr, "\nDictionary stack:\n");
		hg_stack_dump(vm->dstack, vm->stderr);
		abort();
	}
	copy_proc = hg_object_copy((HgObject *)proc);
	if (copy_proc == NULL) {
		g_error("FATAL: failed to allocate a memory for an error handler. there are no way to recover it unfortunately.");
	}
	self = hg_stack_pop(vm->estack);
	_hg_stack_push(vm->estack, copy_proc);
	if (need_self)
		_hg_stack_push(vm->estack, self);
	_hg_stack_push(vm->ostack, errnode);
	vm->has_error = TRUE;
}

void
hg_vm_set_error_from_file(HgVM         *vm,
			  HgValueNode  *errnode,
			  HgFileObject *file,
			  gboolean      need_self)
{
	gchar buffer[4096];

	g_return_if_fail (vm != NULL);
	g_return_if_fail (file != NULL);

	switch (hg_file_object_get_error(file)) {
	    case 0:
		    /* no error */
		    break;
	    case EACCES:
	    case EBADF:
	    case EEXIST:
	    case ENOTDIR:
	    case ENOTEMPTY:
	    case EPERM:
	    case EROFS:
		    hg_vm_set_error(vm, errnode, VM_e_invalidfileaccess, need_self);
		    break;
	    case EAGAIN:
		    if (file == vm->stdin) {
			    /* probably no data from stdin found. */
			    break;
		    }
	    case EBUSY:
	    case EIO:
	    case ENOSPC:
		    hg_vm_set_error(vm, errnode, VM_e_ioerror, need_self);
		    break;
	    case EMFILE:
		    hg_vm_set_error(vm, errnode, VM_e_limitcheck, need_self);
		    break;
	    case ENAMETOOLONG:
	    case ENODEV:
	    case ENOENT:
		    hg_vm_set_error(vm, errnode, VM_e_undefinedfilename, need_self);
		    break;
	    case ENOMEM:
		    hg_vm_set_error(vm, errnode, VM_e_VMerror, need_self);
		    break;
	    default:
		    strerror_r(hg_file_object_get_error(file), buffer, 4096);
		    g_warning("%s: need to support errno %d\n  %s\n",
			      __FUNCTION__, hg_file_object_get_error(file), buffer);
		    break;
	}
	hg_file_object_clear_error(file);
}

gboolean
hg_vm_main(HgVM *vm)
{
	HgValueNode *node;
	HgOperator *op;
	HgFileObject *file;
	guint depth;
	gboolean retval = TRUE;

	g_return_val_if_fail (vm != NULL, FALSE);

	vm->shutdown = FALSE;
	file = hg_file_object_new(hg_vm_get_current_pool(vm),
				  HG_FILE_TYPE_BUFFER,
				  HG_FILE_MODE_READ,
				  "%system",
				  "", 0);
	while (!vm->shutdown) {
		depth = hg_stack_depth(vm->estack);
		if (depth == 0)
			break;
		node = hg_stack_index(vm->estack, 0);
		if (node == NULL) {
			retval = FALSE;
			break;
		}

#ifdef DEBUG_VM
		G_STMT_START {
			HgString *s;

			s = hg_object_to_string((HgObject *)node);
			hg_file_object_printf(vm->stderr, "DEBUG: [%s] %s\n",
					      hg_value_node_get_type_name(HG_VALUE_GET_VALUE_TYPE (node)),
					      hg_string_get_string(s));
			hg_mem_free(s);
		} G_STMT_END;
#endif /* DEBUG_VM */
		switch (HG_VALUE_GET_VALUE_TYPE (node)) {
		    case HG_TYPE_VALUE_BOOLEAN:
		    case HG_TYPE_VALUE_INTEGER:
		    case HG_TYPE_VALUE_REAL:
		    case HG_TYPE_VALUE_DICT:
		    case HG_TYPE_VALUE_NULL:
		    case HG_TYPE_VALUE_MARK:
			    g_warning("[BUG] %s: an object (node type %d) which isn't actually executable, was pushed into the estack.",
				      __FUNCTION__, HG_VALUE_GET_VALUE_TYPE (node));
			    hg_stack_pop(vm->estack);
			    retval = hg_stack_push(vm->ostack, node);
			    if (!retval) {
				    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
			    }
			    break;
		    case HG_TYPE_VALUE_ARRAY:
			    if (hg_object_is_executable((HgObject *)node)) {
				    HgArray *array;
				    HgValueNode *tmp;

				    array = HG_VALUE_GET_ARRAY (node);
				    if (hg_array_length(array) == 0) {
					    /* this might be an empty array */
					    hg_stack_pop(vm->estack);
					    break;
				    }
				    tmp = hg_array_index(array, 0);
				    hg_array_remove(array, 0);
				    if (hg_array_length(array) == 0)
					    hg_stack_pop(vm->estack);
				    if (hg_object_is_executable((HgObject *)tmp) &&
					(HG_IS_VALUE_NAME (tmp) || HG_IS_VALUE_OPERATOR (tmp))) {
					    retval = hg_stack_push(vm->estack, tmp);
					    if (!retval)
						    _hg_vm_set_error(vm, file, VM_e_execstackoverflow, FALSE);
				    } else {
					    retval = hg_stack_push(vm->ostack, tmp);
					    if (!retval)
						    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
				    }
			    } else {
				    g_warning("[BUG] %s: an object (node type %d) which isn't actually executable, was pushed into the estack.",
					      __FUNCTION__, HG_VALUE_GET_VALUE_TYPE (node));
				    hg_stack_pop(vm->estack);
				    retval = hg_stack_push(vm->ostack, node);
				    if (!retval) {
					    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
				    }
			    }
			    break;
		    case HG_TYPE_VALUE_STRING:
			    if (hg_object_is_executable((HgObject *)node)) {
				    HgString *hs = HG_VALUE_GET_STRING (node);
				    guint state = hg_object_get_state((HgObject *)node);

				    file = hg_file_object_new(hg_vm_get_current_pool(vm),
							      HG_FILE_TYPE_BUFFER,
							      HG_FILE_MODE_READ,
							      "%eval_string",
							      hg_string_get_string(hs),
							      hg_string_maxlength(hs));
				    hg_stack_pop(vm->estack);
				    HG_VALUE_MAKE_FILE (node, file);
				    hg_object_set_state((HgObject *)node, state);
				    retval = hg_stack_push(vm->estack, node);
			    } else {
				    g_warning("[BUG] %s: an object (node type %d) which isn't actually executable, was pushed into the estack.",
					      __FUNCTION__, HG_VALUE_GET_VALUE_TYPE (node));
				    hg_stack_pop(vm->estack);
				    retval = hg_stack_push(vm->ostack, node);
			    }
			    if (!retval) {
				    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
			    }
			    break;
		    case HG_TYPE_VALUE_NAME:
			    if (hg_object_is_executable((HgObject *)node)) {
				    HgValueNode *op_node = hg_vm_lookup(vm, node), *tmp;

				    if (op_node == NULL) {
					    hg_vm_set_error(vm, node, VM_e_undefined, FALSE);
				    } else {
					    hg_stack_pop(vm->estack);
					    if (hg_object_is_executable((HgObject *)op_node)) {
						    tmp = hg_object_copy((HgObject *)op_node);
						    if (tmp == NULL) {
							    _hg_vm_set_error(vm, file, VM_e_VMerror, FALSE);
						    } else {
							    retval = hg_stack_push(vm->estack, tmp);
							    if (!retval)
								    _hg_vm_set_error(vm, file, VM_e_execstackoverflow, FALSE);
						    }
					    } else {
						    retval = hg_stack_push(vm->ostack, op_node);
						    if (!retval)
							    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
					    }
				    }
			    } else {
				    g_warning("[BUG] %s: an object (node type %d) which isn't actually executable, was pushed into the estack.",
					      __FUNCTION__, HG_VALUE_GET_VALUE_TYPE (node));
				    hg_stack_pop(vm->estack);
				    retval = hg_stack_push(vm->ostack, node);
				    if (!retval) {
					    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
				    }
			    }
			    break;
		    case HG_TYPE_VALUE_OPERATOR:
			    op = HG_VALUE_GET_OPERATOR (node);
			    retval = hg_operator_invoke(op, vm);
			    hg_stack_pop(vm->estack);
			    break;
		    case HG_TYPE_VALUE_FILE:
			    if (hg_object_is_executable((HgObject *)node)) {
				    file = HG_VALUE_GET_FILE (node);
				    if (hg_file_object_has_error(file)) {
					    hg_stack_pop(vm->estack);
					    _hg_vm_set_error_from_file(vm, file, file, FALSE);
				    } else {
					    HgValueNode *tmp_node;

					    tmp_node = hg_scanner_get_object(vm, file);
					    if (tmp_node != NULL) {
#ifdef DEBUG_SCANNER
						    G_STMT_START {
							    HgString *__str__ = hg_object_to_string((HgObject *)tmp_node);

							    g_print("SCANNER: %s\n", hg_string_get_string(__str__));
						    } G_STMT_END;
#endif /* DEBUG_SCANNER */
						    if (hg_object_is_executable((HgObject *)tmp_node) &&
							(HG_IS_VALUE_NAME (tmp_node) ||
							 HG_IS_VALUE_OPERATOR (tmp_node))) {
							    hg_stack_push(vm->estack, tmp_node);
						    } else {
							    hg_stack_push(vm->ostack, tmp_node);
						    }
					    } else {
						    if (hg_file_object_is_eof(file)) {
							    hg_stack_pop(vm->estack);
						    }
					    }
				    }
			    } else {
				    g_warning("[BUG] %s: an object (node type %d) which isn't actually executable, was pushed into the estack.",
					      __FUNCTION__, HG_VALUE_GET_VALUE_TYPE (node));
				    hg_stack_pop(vm->estack);
				    retval = hg_stack_push(vm->ostack, node);
				    if (!retval) {
					    _hg_vm_set_error(vm, file, VM_e_stackoverflow, FALSE);
				    }
			    }
			    break;
		    default:
			    g_warning("[BUG] Unknown node type %d was given into the estack.",
				      HG_VALUE_GET_VALUE_TYPE (node));
			    hg_stack_pop(vm->estack);
			    break;
		}
		if (!retval && !hg_vm_has_error(vm)) {
			g_warning("[BUG] probably unknown error happened.");
			break;
		}
	}

	return retval;
}

gchar *
hg_vm_find_libfile(HgVM        *vm,
		   const gchar *file)
{
	const gchar *env = g_getenv("HIEROGLYPH_LIB_PATH");
	gchar **paths, *filename = NULL, *basename = g_path_get_basename(file);
	gint i = 0;
	struct stat st;

	/* for the security reason, ignore the original path identifier */
	if (env != NULL) {
		paths = g_strsplit(env, ":", 0);
		if (paths != NULL) {
			while (paths[i] != NULL) {
				filename = g_build_filename(paths[i], basename, NULL);
				if (stat(filename, &st) == 0) {
					break;
				}
				g_free(filename);
				filename = NULL;
				i++;
			}
		}
		g_strfreev(paths);
	}
	if (filename == NULL) {
		/* if the PS file couldn't load, try the default path. */
		filename = g_build_filename(HIEROGLYPH_LIBDIR, basename, NULL);
		if (stat(filename, &st) == -1) {
			g_free(filename);
			filename = NULL;
		}
	}
	if (basename)
		g_free(basename);

	return filename;
}

gboolean
hg_vm_run(HgVM        *vm,
	  const gchar *file)
{
	const gchar *env = g_getenv("HIEROGLYPH_LIB_PATH");
	gchar **paths, *filename, *path = g_path_get_dirname(file), *basename = g_path_get_basename(file);
	gint i = 0;
	gboolean retval = FALSE, error = FALSE;

	if (path != NULL && strcmp(path, ".") != 0) {
		/* file contains any path. try it first */
		retval = _hg_vm_run(vm, file, &error);
		if (error) {
			/* don't retry anymore */
			return FALSE;
		}
		if (retval)
			return retval;
	}
	if (path)
		g_free(path);
	if (env != NULL) {
		paths = g_strsplit(env, ":", 0);
		if (paths != NULL) {
			while (paths[i] != NULL) {
				filename = g_build_filename(paths[i], basename, NULL);
				retval = _hg_vm_run(vm, filename, &error);
				g_free(filename);
				if (retval ||
				    error)
					break;
				i++;
			}
		}
		g_strfreev(paths);
	}
	if (!retval && !error) {
		/* if the PS file couldn't load, try to load it from the default path. */
		filename = g_build_filename(HIEROGLYPH_LIBDIR, basename, NULL);

		retval = _hg_vm_run(vm, filename, &error);
		g_free(filename);
	}
	if (env == NULL && !retval && !error) {
		/* still can't find a file, try to read it on current directory.
		 * but someone may not prefers this way. so this behavior can be
		 * disabled with specifying HIEROGLYPH_LIB_PATH anytime.
		 */
		retval = _hg_vm_run(vm, file, &error);
	}
	if (basename)
		g_free(basename);

	return retval;
}

gboolean
hg_vm_eval(HgVM        *vm,
	   const gchar *expression,
	   HgStack     *ostack,
	   HgStack     *estack,
	   HgStack     *dstack,
	   gboolean    *error)
{
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgStack *old_ostack = NULL, *old_estack = NULL, *old_dstack = NULL;
	HgFileObject *file;
	gboolean retval = FALSE;

	g_return_val_if_fail (vm != NULL, FALSE);
	g_return_val_if_fail (expression != NULL, FALSE);

	if (ostack) {
		old_ostack = hg_vm_get_ostack(vm);
		hg_vm_set_ostack(vm, ostack);
	}
	if (estack) {
		old_estack = hg_vm_get_estack(vm);
		hg_vm_set_estack(vm, estack);
	}
	if (dstack) {
		old_dstack = hg_vm_get_dstack(vm);
		hg_vm_set_dstack(vm, dstack);
	}
	file = hg_file_object_new(pool,
				  HG_FILE_TYPE_BUFFER,
				  HG_FILE_MODE_READ,
				  "%eval",
				  expression, - 1);

	retval = _hg_vm_eval_file(vm, file, error);

	if (old_ostack)
		hg_vm_set_ostack(vm, old_ostack);
	if (old_estack)
		hg_vm_set_estack(vm, old_estack);
	if (old_dstack)
		hg_vm_set_dstack(vm, old_dstack);

	return retval;
}

gint32
hg_vm_get_error_code(HgVM *vm)
{
	g_return_val_if_fail (vm != NULL, -1);

	return vm->error_code;
}

void
hg_vm_set_error_code(HgVM   *vm,
		     gint32  error_code)
{
	g_return_if_fail (vm != NULL);

	vm->error_code = error_code;
}

void
hg_vm_shutdown(HgVM   *vm,
	       gint32  error_code)
{
	g_return_if_fail (vm != NULL);

	vm->shutdown = TRUE;
	hg_vm_set_error_code(vm, error_code);
}

gint32
hg_vm_get_security_level(HgVM *vm)
{
	g_return_val_if_fail(vm != NULL, G_MAXINT32);

	return vm->security_level;
}

gboolean
hg_vm_set_security_level(HgVM   *vm,
			 gint32  level)
{
	g_return_val_if_fail (vm != NULL, FALSE);

	if (hg_vm_get_security_level(vm) > level)
		return FALSE;

	vm->security_level = level;

	return TRUE;
}
