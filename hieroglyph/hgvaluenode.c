/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgvaluenode.c
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

#include <math.h>
#include <string.h>
#include "hgvaluenode.h"
#include "hgallocator-bfit.h"
#include "hgarray.h"
#include "hgbtree.h"
#include "hgdict.h"
#include "hgfile.h"
#include "hgmem.h"
#include "hgstring.h"

#define BTREE_N_NODE	3

static void     _hg_value_node_real_set_flags(gpointer           data,
					      guint              flags);
static void     _hg_value_node_real_relocate (gpointer           data,
					      HgMemRelocateInfo *info);
static gpointer _hg_value_node_real_dup      (gpointer           data);
static gpointer _hg_value_node_real_copy     (gpointer           data);
static gpointer _hg_value_node_real_to_string(gpointer           data);


static HgObjectVTable __hg_value_node_vtable = {
	.free      = NULL,
	.set_flags = _hg_value_node_real_set_flags,
	.relocate  = _hg_value_node_real_relocate,
	.dup       = _hg_value_node_real_dup,
	.copy      = _hg_value_node_real_copy,
	.to_string = _hg_value_node_real_to_string,
};
static HgBTree *__hg_value_node_type_tree = NULL;
static HgAllocator *__hg_value_node_allocator = NULL;
static HgMemPool *__hg_value_node_mem_pool = NULL;
static gboolean __hg_value_node_is_initialized = FALSE;


/*
 * Private Functions
 */
static gboolean
_hg_value_node_register_type(const HgValueNodeTypeInfo *info)
{
	g_return_val_if_fail (__hg_value_node_is_initialized, FALSE);

	if (hg_btree_find(__hg_value_node_type_tree, GUINT_TO_POINTER (info->type_id))) {
		g_warning("type_id %d already registered.", info->type_id);
		return FALSE;
	}
	hg_btree_add(__hg_value_node_type_tree,
		     GUINT_TO_POINTER (info->type_id),
		     GSIZE_TO_POINTER (info));

	return TRUE;
}

static void
_hg_value_node_real_set_flags(gpointer data,
			      guint    flags)
{
	HgValueNode *node = data;
	HgMemObject *obj = NULL;

	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
	    case HG_TYPE_VALUE_INTEGER:
	    case HG_TYPE_VALUE_REAL:
		    /* nothing to do */
		    break;
	    case HG_TYPE_VALUE_NAME:
	    case HG_TYPE_VALUE_ARRAY:
	    case HG_TYPE_VALUE_STRING:
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_OPERATOR:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
	    case HG_TYPE_VALUE_PLUGIN:
		    if (((HgValueNodePointer *)node)->value) {
			    hg_mem_get_object__inline(((HgValueNodePointer *)node)->value, obj);
			    if (obj == NULL) {
				    g_warning("[BUG] Invalid object %p with node type %d",
					      ((HgValueNodePointer *)node)->value,
					      HG_VALUE_GET_VALUE_TYPE (node));
			    }
		    }
		    break;
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    break;
	    default:
		    g_warning("Unknown node type %d to set flags", HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}

	if (obj != NULL) {
#ifdef DEBUG_GC
		G_STMT_START {
			if ((flags & HG_MEMOBJ_MARK_AGE_MASK) != 0) {
				if (!hg_mem_is_flags__inline(obj, flags)) {
					hg_value_node_debug_print(__hg_file_stderr, HG_DEBUG_GC_MARK, 0, NULL, node, NULL);
				} else {
					hg_value_node_debug_print(__hg_file_stderr, HG_DEBUG_GC_ALREADYMARK, 0, NULL, node, NULL);
				}
			}
		} G_STMT_END;
#endif /* DEBUG_GC */
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
}

static void
_hg_value_node_real_relocate(gpointer           data,
			     HgMemRelocateInfo *info)
{
	HgValueNode *node = data;

	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
	    case HG_TYPE_VALUE_INTEGER:
	    case HG_TYPE_VALUE_REAL:
		    /* nothing to do */
		    break;
	    case HG_TYPE_VALUE_NAME:
	    case HG_TYPE_VALUE_ARRAY:
	    case HG_TYPE_VALUE_STRING:
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
	    case HG_TYPE_VALUE_PLUGIN:
		    if ((gsize)((HgValueNodePointer *)node)->value >= info->start &&
			(gsize)((HgValueNodePointer *)node)->value <= info->end) {
			    ((HgValueNodePointer *)node)->value = (gpointer)((gsize)((HgValueNodePointer *)node)->value + info->diff);
		    }
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    break;
	    default:
		    g_warning("Unknown node type %d to be relocated", HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}
}

static gpointer
_hg_value_node_real_dup(gpointer data)
{
	HgValueNode *node = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(node, obj);
	if (obj == NULL)
		return NULL;

	if (HG_VALUE_GET_VALUE_TYPE (node) == HG_TYPE_VALUE_NULL ||
	    HG_VALUE_GET_VALUE_TYPE (node) == HG_TYPE_VALUE_MARK) {
		return node;
	}
	retval = hg_value_node_new(obj->pool, HG_VALUE_GET_VALUE_TYPE (node));
	if (retval == NULL) {
		g_warning("Failed to duplicate a node.");
		return NULL;
	}
	HG_OBJECT_SET_STATE (&retval->object, HG_OBJECT_GET_STATE (&node->object));
	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
		    HG_VALUE_SET_BOOLEAN (retval, HG_VALUE_GET_BOOLEAN (node));
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    HG_VALUE_SET_INTEGER (retval, HG_VALUE_GET_INTEGER (node));
		    break;
	    case HG_TYPE_VALUE_REAL:
		    HG_VALUE_SET_REAL (retval, HG_VALUE_GET_REAL (node));
		    break;
	    case HG_TYPE_VALUE_NAME:
		    HG_VALUE_SET_NAME (retval, HG_VALUE_GET_NAME (node));
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    HG_VALUE_SET_ARRAY (retval, HG_VALUE_GET_ARRAY (node));
		    break;
	    case HG_TYPE_VALUE_STRING:
		    HG_VALUE_SET_STRING (retval, HG_VALUE_GET_STRING (node));
		    break;
	    case HG_TYPE_VALUE_DICT:
		    HG_VALUE_SET_DICT (retval, HG_VALUE_GET_DICT (node));
		    break;
	    case HG_TYPE_VALUE_FILE:
		    HG_VALUE_SET_FILE (retval, HG_VALUE_GET_FILE (node));
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
		    HG_VALUE_SET_OPERATOR (retval, HG_VALUE_GET_OPERATOR (node));
		    break;
	    case HG_TYPE_VALUE_SNAPSHOT:
		    HG_VALUE_SET_SNAPSHOT (retval, HG_VALUE_GET_SNAPSHOT (node));
		    break;
	    case HG_TYPE_VALUE_PLUGIN:
		    HG_VALUE_SET_PLUGIN (retval, HG_VALUE_GET_PLUGIN (node));
		    break;
	    default:
		    g_warning("Unknown node type %d to be duplicated", HG_VALUE_GET_VALUE_TYPE (node));
		    hg_mem_free(retval);
		    return NULL;
	}

	return retval;
}

static gpointer
_hg_value_node_real_copy(gpointer data)
{
	HgValueNode *node = data, *retval;
	HgMemObject *obj;
	gpointer p;

#define _copy_object(_node, _type, _obj)		\
	G_STMT_START {					\
		p = hg_object_copy((HgObject *)(_obj));	\
		if (p == NULL)				\
			return NULL;			\
		HG_VALUE_SET_ ## _type (_node, p);	\
	} G_STMT_END

	hg_mem_get_object__inline(node, obj);
	if (obj == NULL)
		return NULL;

	if (HG_VALUE_GET_VALUE_TYPE (node) == HG_TYPE_VALUE_NULL ||
	    HG_VALUE_GET_VALUE_TYPE (node) == HG_TYPE_VALUE_MARK) {
		return node;
	}
	retval = hg_value_node_new(obj->pool, HG_VALUE_GET_VALUE_TYPE (node));
	if (retval == NULL) {
		g_warning("Failed to duplicate a node.");
		return NULL;
	}
	HG_OBJECT_SET_STATE (&retval->object, HG_OBJECT_GET_STATE (&node->object));
	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
		    HG_VALUE_SET_BOOLEAN (retval, HG_VALUE_GET_BOOLEAN (node));
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    HG_VALUE_SET_INTEGER (retval, HG_VALUE_GET_INTEGER (node));
		    break;
	    case HG_TYPE_VALUE_REAL:
		    HG_VALUE_SET_REAL (retval, HG_VALUE_GET_REAL (node));
		    break;
	    case HG_TYPE_VALUE_NAME:
		    _copy_object (retval, NAME, HG_VALUE_GET_NAME (node));
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    _copy_object (retval, ARRAY, HG_VALUE_GET_ARRAY (node));
		    break;
	    case HG_TYPE_VALUE_STRING:
		    _copy_object (retval, STRING, HG_VALUE_GET_STRING (node));
		    break;
	    case HG_TYPE_VALUE_DICT:
		    _copy_object (retval, DICT, HG_VALUE_GET_DICT (node));
		    break;
	    case HG_TYPE_VALUE_FILE:
		    _copy_object (retval, FILE, HG_VALUE_GET_FILE (node));
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
		    _copy_object (retval, OPERATOR, HG_VALUE_GET_OPERATOR (node));
		    break;
	    case HG_TYPE_VALUE_SNAPSHOT:
		    _copy_object (retval, SNAPSHOT, HG_VALUE_GET_SNAPSHOT (node));
		    break;
	    case HG_TYPE_VALUE_PLUGIN:
		    _copy_object (retval, PLUGIN, HG_VALUE_GET_PLUGIN (node));
		    break;
	    default:
		    g_warning("Unknown node type %d to be duplicated", HG_VALUE_GET_VALUE_TYPE (node));
		    hg_mem_free(retval);
		    return NULL;
	}

#undef _copy_object

	return retval;
}

static gpointer
_hg_value_node_real_to_string(gpointer data)
{
	HgValueNode *node = data;
	HgMemObject *obj;
	HgString *retval, *str = NULL;
	HgArray *array;
	gchar *tmp, start, end;
	const gchar *name;
	size_t len;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_string_new(obj->pool, -1);
	if (retval == NULL)
		return NULL;
	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
		    if (HG_VALUE_GET_BOOLEAN (node) == TRUE)
			    hg_string_append(retval, "true", -1);
		    else
			    hg_string_append(retval, "false", -1);
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    tmp = g_strdup_printf("%d", HG_VALUE_GET_INTEGER (node));
		    hg_string_append(retval, tmp, -1);
		    g_free(tmp);
		    break;
	    case HG_TYPE_VALUE_REAL:
		    tmp = g_strdup_printf("%f", HG_VALUE_GET_REAL (node));
		    if (tmp != NULL) {
			    len = strlen(tmp);
			    while (len > 1 && tmp[len - 1] == '0')
				    len--;
			    if (tmp[len - 1] == '.')
				    len++;
			    hg_string_append(retval, tmp, len);
			    g_free(tmp);
		    }
		    break;
	    case HG_TYPE_VALUE_NAME:
		    if (!hg_object_is_executable((HgObject *)node))
			    hg_string_append_c(retval, '/');
		    hg_string_append(retval, HG_VALUE_GET_NAME (node), -1);
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    if (hg_object_is_executable((HgObject *)node)) {
			    start = '{';
			    end = '}';
		    } else {
			    start = '[';
			    end = ']';
		    }
		    if (hg_object_is_readable((HgObject *)node)) {
			    str = hg_object_to_string((HgObject *)HG_VALUE_GET_ARRAY (node));
			    if (str == NULL) {
				    hg_mem_free(retval);
				    return NULL;
			    }
			    hg_string_append_c(retval, start);
			    hg_string_concat(retval, str);
			    hg_string_append_c(retval, end);
		    } else {
			    array = HG_VALUE_GET_ARRAY (node);
			    if ((name = hg_array_get_name(array)) != NULL) {
				    hg_string_append(retval, "--", 2);
				    hg_string_append(retval, name, -1);
				    hg_string_append(retval, "--", 2);
			    } else {
				    hg_string_append(retval, "-array-", -1);
			    }
		    }
		    break;
	    case HG_TYPE_VALUE_STRING:
		    if (hg_object_is_readable((HgObject *)node)) {
			    str = hg_object_to_string((HgObject *)HG_VALUE_GET_STRING (node));
			    if (str == NULL) {
				    hg_mem_free(retval);
				    return NULL;
			    }
			    hg_string_append_c(retval, '(');
			    hg_string_concat(retval, str);
			    hg_string_append_c(retval, ')');
		    } else {
			    hg_string_append(retval, "-string-", -1);
		    }
		    break;
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_OPERATOR:
	    case HG_TYPE_VALUE_SNAPSHOT:
	    case HG_TYPE_VALUE_PLUGIN:
		    str = hg_object_to_string((HgObject *)((HgValueNodePointer *)node)->value);
		    if (str == NULL) {
			    hg_mem_free(retval);
			    return NULL;
		    }
		    hg_string_concat(retval, str);
		    break;
	    case HG_TYPE_VALUE_NULL:
		    hg_string_append(retval, "null", -1);
		    break;
	    case HG_TYPE_VALUE_MARK:
		    hg_string_append(retval, "-mark-", -1);
		    break;
	    default:
		    g_warning("Unknown node type %d to be converted to string.",
			      HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}
	if (str)
		hg_mem_free(str);
	hg_string_fix_string_size(retval);

	return retval;
}

/*
 * Public Functions
 */
void
hg_value_node_init(void)
{
	hg_mem_init();
#ifdef DEBUG
	hg_file_init();
#endif /* DEBUG */

	if (!__hg_value_node_is_initialized) {
		__hg_value_node_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
		__hg_value_node_mem_pool = hg_mem_pool_new(__hg_value_node_allocator,
							   "Memory pool for HgValueNode",
							   1024, HG_MEM_GLOBAL); // | HG_MEM_RESIZABLE);

		__hg_value_node_type_tree = hg_btree_new(__hg_value_node_mem_pool,
							 BTREE_N_NODE);
		hg_mem_add_root_node(__hg_value_node_mem_pool, __hg_value_node_type_tree);
		hg_btree_allow_marking(__hg_value_node_type_tree, FALSE);

		__hg_value_node_is_initialized = TRUE;

		/* register the reserved type IDs */
#define _register_type(_t, _s)						\
		G_STMT_START {						\
			static HgValueNodeTypeInfo type_info = {	\
				.name = # _t,				\
				.type_id = HG_TYPE_VALUE_ ## _t,	\
				.struct_size = sizeof (HgValueNode ## _s), \
			};						\
									\
			_hg_value_node_register_type(&type_info);	\
		} G_STMT_END

		_register_type(BOOLEAN, Boolean);
		_register_type(INTEGER, Integer);
		_register_type(REAL, Real);
		_register_type(NAME, Name);
		_register_type(ARRAY, Array);
		_register_type(STRING, String);
		_register_type(DICT, Dict);
		_register_type(NULL, Null);
		_register_type(OPERATOR, Operator);
		_register_type(MARK, Mark);
		_register_type(FILE, File);
		_register_type(SNAPSHOT, Snapshot);
		_register_type(PLUGIN, Plugin);
	}
}

void
hg_value_node_finalize(void)
{
	if (__hg_value_node_is_initialized) {
		hg_mem_pool_destroy(__hg_value_node_mem_pool);
		hg_allocator_destroy(__hg_value_node_allocator);
#ifdef DEBUG
		hg_file_finalize();
#endif /* DEBUG */

		__hg_value_node_is_initialized = FALSE;
	}
}

gboolean
hg_value_node_register_type(const HgValueNodeTypeInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);
	g_return_val_if_fail (info->type_id > HG_TYPE_VALUE_END, FALSE);

	return _hg_value_node_register_type(info);
}

const gchar *
hg_value_node_get_type_name(guint type_id)
{
	HgValueNodeTypeInfo *info;

	g_return_val_if_fail (__hg_value_node_is_initialized, NULL);

	if ((info = hg_btree_find(__hg_value_node_type_tree,
				  GUINT_TO_POINTER (type_id))) == NULL) {
		g_warning("Unknown type ID %d. Failed to get a type name.",
			  type_id);
		return NULL;
	}

	return info->name;
}

HgValueNode *
hg_value_node_new(HgMemPool   *pool,
		  guint        type_id)
{
	HgValueNode *retval;
	HgValueNodeTypeInfo *info;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (__hg_value_node_is_initialized, NULL);

	if ((info = hg_btree_find(__hg_value_node_type_tree,
				  GUINT_TO_POINTER (type_id))) == NULL) {
		g_warning("Unknown type ID %d. Failed to create a HgValueNode.", type_id);
		return NULL;
	}
	retval = hg_mem_alloc_with_flags(pool, info->struct_size,
					 HG_FL_HGOBJECT | HG_FL_RESTORABLE);
	if (retval == NULL) {
		g_warning("Failed to create a node.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_value_node_vtable);

	return retval;
}

gsize
hg_value_node_get_hash(const HgValueNode *node)
{
	gsize retval = 0;

#define _get_hash(_node, _type)				\
	(gsize)HG_VALUE_GET_ ## _type (_node);

	switch (HG_VALUE_GET_VALUE_TYPE (node)) {
	    case HG_TYPE_VALUE_BOOLEAN:
		    retval = HG_VALUE_GET_BOOLEAN (node);
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    retval = HG_VALUE_GET_INTEGER (node);
		    break;
	    case HG_TYPE_VALUE_REAL:
		    retval = HG_VALUE_GET_REAL (node);
		    break;
	    case HG_TYPE_VALUE_NAME:
		    G_STMT_START {
			    gchar *name = HG_VALUE_GET_NAME (node), *p;

			    for (p = name; *p != 0; p++) {
				    retval = (retval << 5) - retval + *p;
			    }
		    } G_STMT_END;
		    break;
	    case HG_TYPE_VALUE_STRING:
		    G_STMT_START {
			    HgString *string = HG_VALUE_GET_STRING (node);
			    guint len = hg_string_length(string), i;
			    gchar c;

			    for (i = 0; i < len; i++) {
				    c = hg_string_index(string, i);
				    retval = (retval << 5) - retval + c;
			    }
		    } G_STMT_END;
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    retval = _get_hash(node, ARRAY);
		    break;
	    case HG_TYPE_VALUE_DICT:
		    retval = _get_hash(node, DICT);
		    break;
	    case HG_TYPE_VALUE_NULL:
		    retval = _get_hash(node, NULL);
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
		    retval = _get_hash(node, OPERATOR);
		    break;
	    case HG_TYPE_VALUE_MARK:
		    retval = _get_hash(node, MARK);
		    break;
	    case HG_TYPE_VALUE_FILE:
		    retval = _get_hash(node, FILE);
		    break;
	    case HG_TYPE_VALUE_SNAPSHOT:
		    retval = _get_hash(node, SNAPSHOT);
		    break;
	    case HG_TYPE_VALUE_PLUGIN:
		    retval = _get_hash(node, PLUGIN);
		    break;
	    default:
		    g_warning("[BUG] Unknown node type %d to generate hash",
			      HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}

#undef _get_hash

	return retval;
}

gboolean
hg_value_node_compare(const HgValueNode *a,
		      const HgValueNode *b)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

#define _compare(_a, _b, _type)			\
	(HG_VALUE_GET_ ## _type (_a) == HG_VALUE_GET_ ## _type (_b))

	if (HG_VALUE_GET_VALUE_TYPE (a) != HG_VALUE_GET_VALUE_TYPE (b))
		return FALSE;
	switch (HG_VALUE_GET_VALUE_TYPE (a)) {
	    case HG_TYPE_VALUE_BOOLEAN:
		    retval = (HG_VALUE_GET_BOOLEAN (a) == HG_VALUE_GET_BOOLEAN (b));
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    retval = (HG_VALUE_GET_INTEGER (a) == HG_VALUE_GET_INTEGER (b));
		    break;
	    case HG_TYPE_VALUE_REAL:
		    retval = HG_VALUE_REAL_SIMILAR (HG_VALUE_GET_REAL (a), HG_VALUE_GET_REAL (b));
		    break;
	    case HG_TYPE_VALUE_NAME:
		    g_return_val_if_fail (hg_object_is_readable((HgObject *)a), FALSE);
		    g_return_val_if_fail (hg_object_is_readable((HgObject *)b), FALSE);
		    retval = (strcmp(HG_VALUE_GET_NAME (a), HG_VALUE_GET_NAME (b)) == 0);
		    break;
	    case HG_TYPE_VALUE_STRING:
		    retval = hg_string_compare(HG_VALUE_GET_STRING (a), HG_VALUE_GET_STRING (b));
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    retval = _compare (a, b, ARRAY);
		    break;
	    case HG_TYPE_VALUE_DICT:
		    retval = _compare (a, b, DICT);
		    break;
	    case HG_TYPE_VALUE_FILE:
		    retval = _compare (a, b, FILE);
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
		    retval = _compare (a, b, OPERATOR);
		    break;
	    case HG_TYPE_VALUE_SNAPSHOT:
		    retval = _compare (a, b, SNAPSHOT);
		    break;
	    case HG_TYPE_VALUE_PLUGIN:
		    retval = _compare (a, b, PLUGIN);
		    break;
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    retval = TRUE;
		    break;
	    default:
		    g_warning("[BUG] Unknown node type is given to compare: %d",
			      HG_VALUE_GET_VALUE_TYPE (a));
		    break;
	}

#undef _compare

	return retval;
}

gboolean
hg_value_node_compare_content(const HgValueNode *a,
			      const HgValueNode *b,
			      guint              attributes_mask)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

#define _compare(_a, _b, _type)			\
	(HG_VALUE_GET_ ## _type (_a) == HG_VALUE_GET_ ## _type (_b))

	if (HG_VALUE_GET_VALUE_TYPE (a) != HG_VALUE_GET_VALUE_TYPE (b))
		return FALSE;
	switch (HG_VALUE_GET_VALUE_TYPE (a)) {
	    case HG_TYPE_VALUE_BOOLEAN:
		    retval = (HG_VALUE_GET_BOOLEAN (a) == HG_VALUE_GET_BOOLEAN (b));
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    retval = (HG_VALUE_GET_INTEGER (a) == HG_VALUE_GET_INTEGER (b));
		    break;
	    case HG_TYPE_VALUE_REAL:
		    retval = HG_VALUE_REAL_SIMILAR (HG_VALUE_GET_REAL (a), HG_VALUE_GET_REAL (b));
		    break;
	    case HG_TYPE_VALUE_NAME:
		    retval = (strcmp(HG_VALUE_GET_NAME (a), HG_VALUE_GET_NAME (b)) == 0);
		    break;
	    case HG_TYPE_VALUE_STRING:
		    retval = hg_string_compare(HG_VALUE_GET_STRING (a), HG_VALUE_GET_STRING (b));
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    retval = hg_array_compare(HG_VALUE_GET_ARRAY (a),
					      HG_VALUE_GET_ARRAY (b),
					      attributes_mask);
		    break;
	    case HG_TYPE_VALUE_DICT:
		    retval = hg_dict_compare(HG_VALUE_GET_DICT (a),
					     HG_VALUE_GET_DICT (b),
					     attributes_mask);
		    break;
	    case HG_TYPE_VALUE_FILE:
		    retval = _compare (a, b, FILE);
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
		    retval = _compare (a, b, OPERATOR);
		    break;
	    case HG_TYPE_VALUE_SNAPSHOT:
		    retval = _compare (a, b, SNAPSHOT);
		    break;
	    case HG_TYPE_VALUE_PLUGIN:
		    retval = _compare (a, b, PLUGIN);
		    break;
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    retval = TRUE;
		    break;
	    default:
		    g_warning("[BUG] Unknown node type is given to compare: %d",
			      HG_VALUE_GET_VALUE_TYPE (a));
		    break;
	}

#undef _compare

#define _state_comp(_a_, _b_, _mask_)					\
	((hg_object_get_state((HgObject *)(_a_)) & (_mask_)) ==		\
	 (hg_object_get_state((HgObject *)(_b_)) & (_mask_)))

	retval &= _state_comp(a, b, attributes_mask);

#undef _state_comp

	return retval;
}

void
hg_value_node_debug_print(HgFileObject    *file,
			  HgDebugStateType type,
			  HgValueType      vtype,
			  gpointer         parent,
			  gpointer         self,
			  gpointer         extrainfo)
{
	const gchar *value[] = {
		"node",
		"boolean",
		"integer",
		"real",
		"name",
		"array",
		"string",
		"dict",
		"null",
		"pointer",
		"mark",
		"file",
		"save",
	};
	const gchar *dict_info[] = {"DictNode", "Key", "Val"};
	gchar *info = NULL;
	HgMemObject *obj;

	hg_mem_get_object__inline(self, obj);
	switch (vtype) {
	    case 0:
		    if (HG_VALUE_GET_VALUE_TYPE ((HgValueNode *)self) == HG_TYPE_VALUE_NAME) {
			    info = g_strdup_printf("[type: %s %p (%s)]",
						   value[HG_VALUE_GET_VALUE_TYPE ((HgValueNode *)self)],
						   HG_VALUE_GET_NAME (self),
						   (gchar *)HG_VALUE_GET_NAME (self));
		    } else {
			    info = g_strdup_printf("[type: %s %p]",
						   value[HG_VALUE_GET_VALUE_TYPE ((HgValueNode *)self)],
						   ((HgValueNodePointer *)self)->value);
		    }
		    break;
	    case HG_TYPE_VALUE_BOOLEAN:
		    info = g_strdup_printf("%s", (HG_VALUE_GET_BOOLEAN (self) ? "true" : "false"));
		    break;
	    case HG_TYPE_VALUE_INTEGER:
		    info = g_strdup_printf("%d", HG_VALUE_GET_INTEGER (self));
		    break;
	    case HG_TYPE_VALUE_REAL:
		    info = g_strdup_printf("%f", HG_VALUE_GET_REAL (self));
		    break;
	    case HG_TYPE_VALUE_NAME:
		    info = g_strdup((gchar *)HG_VALUE_GET_NAME (self));
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    if (GPOINTER_TO_UINT (extrainfo) == G_MAXUINT)
			    info = g_strdup("[base]");
		    else
			    info = g_strdup_printf("[%d]", GPOINTER_TO_UINT (extrainfo));
		    break;
	    case HG_TYPE_VALUE_STRING:
		    break;
	    case HG_TYPE_VALUE_DICT:
		    info = g_strdup_printf("[%s]", dict_info[GPOINTER_TO_UINT (extrainfo)]);
		    break;
	    case HG_TYPE_VALUE_NULL:
		    break;
	    case HG_TYPE_VALUE_OPERATOR:
		    info = g_strdup_printf("[%s]", (gchar *)extrainfo);
		    break;
	    case HG_TYPE_VALUE_MARK:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
	    case HG_TYPE_VALUE_PLUGIN:
	    default:
		    break;
	}
	switch (type) {
	    case HG_DEBUG_GC_MARK:
		    hg_file_object_printf(file, "MARK: age: %d [type: %s] parent: %p -- %p %s\n", HG_MEMOBJ_GET_MARK_AGE (obj), value[vtype], parent, self, (info ? info : ""));
		    break;
	    case HG_DEBUG_GC_ALREADYMARK:
		    hg_file_object_printf(file, "MARK[already]: age: %d [type: %s] parent: %p -- %p %s\n", HG_MEMOBJ_GET_MARK_AGE (obj), value[vtype], parent, self, (info ? info : ""));
		    break;
	    case HG_DEBUG_GC_UNMARK:
		    hg_file_object_printf(file, "UNMARK: [type: %s] parent: %p -- %p %s\n", value[vtype], parent, self, (info ? info : ""));
		    break;
	    case HG_DEBUG_DUMP:
		    hg_file_object_printf(file, "%8p|%7s|%s\n", self, value[vtype], (info ? info : ""));
		    break;
	    default:
		    break;
	}
	if (info)
		g_free(info);
}
