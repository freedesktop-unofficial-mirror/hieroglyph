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
#include "hgmem.h"
#include "hgstring.h"
#include "hgfile.h"


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

/*
 * Private Functions
 */
static void
_hg_value_node_real_set_flags(gpointer data,
			      guint    flags)
{
	HgValueNode *node = data;
	HgMemObject *obj;

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
	    case HG_TYPE_VALUE_POINTER:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
		    if (node->v.pointer) {
			    hg_mem_get_object__inline(node->v.pointer, obj);
			    if (obj == NULL) {
				    g_warning("[BUG] Invalid object %p with node type %d",
					      node->v.pointer, HG_VALUE_GET_VALUE_TYPE (node));
			    } else {
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
		    break;
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    break;
	    default:
		    g_warning("Unknown node type %d to set flags", HG_VALUE_GET_VALUE_TYPE (node));
		    break;
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
		    if ((gsize)node->v.pointer >= info->start &&
			(gsize)node->v.pointer <= info->end) {
			    node->v.pointer = (gpointer)((gsize)node->v.pointer + info->diff);
		    }
		    break;
	    case HG_TYPE_VALUE_POINTER:
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

	retval = hg_value_node_new(obj->pool);
	if (retval == NULL) {
		g_warning("Failed to duplicate a node.");
		return NULL;
	}
	HG_OBJECT_SET_STATE (&retval->object, HG_OBJECT_GET_STATE (&node->object));
	retval->v.pointer = NULL;
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
	    case HG_TYPE_VALUE_ARRAY:
	    case HG_TYPE_VALUE_STRING:
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_POINTER:
	    case HG_TYPE_VALUE_SNAPSHOT:
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    HG_VALUE_SET_VALUE_TYPE (retval, HG_VALUE_GET_VALUE_TYPE (node));
		    retval->v.pointer = node->v.pointer;
		    break;
	    default:
		    g_warning("Unknown node type %d to be duplicated", HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}

	return retval;
}

static gpointer
_hg_value_node_real_copy(gpointer data)
{
	HgValueNode *node = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(node, obj);
	if (obj == NULL)
		return NULL;

	retval = hg_value_node_new(obj->pool);
	if (retval == NULL) {
		g_warning("Failed to duplicate a node.");
		return NULL;
	}
	HG_OBJECT_SET_STATE (&retval->object, HG_OBJECT_GET_STATE (&node->object));
	retval->v.pointer = NULL;
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
	    case HG_TYPE_VALUE_ARRAY:
	    case HG_TYPE_VALUE_STRING:
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_POINTER:
	    case HG_TYPE_VALUE_SNAPSHOT:
		    HG_VALUE_SET_VALUE_TYPE (retval, HG_VALUE_GET_VALUE_TYPE (node));
		    retval->v.pointer = hg_object_copy((HgObject *)node->v.pointer);
		    if (retval->v.pointer == NULL)
			    return NULL;
		    break;
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_MARK:
		    HG_VALUE_SET_VALUE_TYPE (retval, HG_VALUE_GET_VALUE_TYPE (node));
		    retval->v.pointer = node->v.pointer;
		    break;
	    default:
		    g_warning("Unknown node type %d to be duplicated", HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}

	return retval;
}

static gpointer
_hg_value_node_real_to_string(gpointer data)
{
	HgValueNode *node = data;
	HgMemObject *obj;
	HgString *retval, *str;
	gchar *tmp, start, end;

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
		    hg_string_append(retval, tmp, -1);
		    g_free(tmp);
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
			    hg_string_append(retval, "-array-", -1);
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
	    case HG_TYPE_VALUE_POINTER:
	    case HG_TYPE_VALUE_SNAPSHOT:
		    str = hg_object_to_string(node->v.pointer);
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
	hg_string_fix_string_size(retval);

	return retval;
}

/*
 * Public Functions
 */
HgValueNode *
hg_value_node_new(HgMemPool *pool)
{
	HgValueNode *retval;

	retval = hg_mem_alloc_with_flags(pool,
					 sizeof (HgValueNode),
					 HG_FL_RESTORABLE);
	if (retval == NULL) {
		g_warning("Failed to create a node.");
		return NULL;
	}
	HG_OBJECT_INIT_OBJECT (retval);
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_value_node_vtable);

	return retval;
}

gsize
hg_value_node_get_hash(const HgValueNode *node)
{
	gsize retval = 0;

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
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_NULL:
	    case HG_TYPE_VALUE_POINTER:
	    case HG_TYPE_VALUE_MARK:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
		    retval = (gsize)node->v.pointer;
		    break;
	    default:
		    g_warning("[BUG] Unknown node type %d to generate hash",
			      HG_VALUE_GET_VALUE_TYPE (node));
		    break;
	}

	return retval;
}

gboolean
hg_value_node_compare(const HgValueNode *a,
		      const HgValueNode *b)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

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
		    retval = (fabs(HG_VALUE_GET_REAL (a) - HG_VALUE_GET_REAL (b)) <= fabs(DBL_EPSILON * HG_VALUE_GET_REAL (a)));
		    break;
	    case HG_TYPE_VALUE_NAME:
		    retval = (strcmp(HG_VALUE_GET_NAME (a), HG_VALUE_GET_NAME (b)) == 0);
		    break;
	    case HG_TYPE_VALUE_STRING:
		    retval = hg_string_compare(HG_VALUE_GET_STRING (a), HG_VALUE_GET_STRING (b));
		    break;
	    case HG_TYPE_VALUE_ARRAY:
	    case HG_TYPE_VALUE_DICT:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_POINTER:
	    case HG_TYPE_VALUE_SNAPSHOT:
		    retval = (a->v.pointer == b->v.pointer);
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

	switch (vtype) {
	    case 0:
		    if (HG_VALUE_GET_VALUE_TYPE ((HgValueNode *)self) == HG_TYPE_VALUE_NAME) {
			    info = g_strdup_printf("[type: %s %p (%s)]",
						   value[HG_VALUE_GET_VALUE_TYPE ((HgValueNode *)self)],
						   ((HgValueNode *)self)->v.pointer,
						   (gchar *)((HgValueNode *)self)->v.pointer);
		    } else {
			    info = g_strdup_printf("[type: %s %p]",
						   value[HG_VALUE_GET_VALUE_TYPE ((HgValueNode *)self)],
						   ((HgValueNode *)self)->v.pointer);
		    }
		    break;
	    case HG_TYPE_VALUE_BOOLEAN:
	    case HG_TYPE_VALUE_INTEGER:
	    case HG_TYPE_VALUE_REAL:
	    case HG_TYPE_VALUE_NAME:
		    break;
	    case HG_TYPE_VALUE_ARRAY:
		    if ((guint)extrainfo == G_MAXUINT)
			    info = g_strdup("[base]");
		    else
			    info = g_strdup_printf("[%d]", (guint)extrainfo);
		    break;
	    case HG_TYPE_VALUE_STRING:
		    break;
	    case HG_TYPE_VALUE_DICT:
		    info = g_strdup_printf("[%s]", dict_info[(gint)extrainfo]);
		    break;
	    case HG_TYPE_VALUE_NULL:
		    break;
	    case HG_TYPE_VALUE_POINTER:
		    info = g_strdup_printf("[%s]", (gchar *)extrainfo);
		    break;
	    case HG_TYPE_VALUE_MARK:
	    case HG_TYPE_VALUE_FILE:
	    case HG_TYPE_VALUE_SNAPSHOT:
	    default:
		    break;
	}
	switch (type) {
	    case HG_DEBUG_GC_MARK:
		    hg_file_object_printf(file, "MARK: [type: %s] parent: %p -- %p %s\n", value[vtype], parent, self, (info ? info : ""));
		    break;
	    case HG_DEBUG_GC_ALREADYMARK:
		    hg_file_object_printf(file, "MARK[already]: [type: %s] parent: %p -- %p %s\n", value[vtype], parent, self, (info ? info : ""));
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
