/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgvaluenode.h
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
#ifndef __HG_VALUE_NODE_H__
#define __HG_VALUE_NODE_H__

#include <math.h>
#include <string.h>
#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

#define HG_VALUE_SET_VALUE_NODE(obj, _type, sym, val)	\
	G_STMT_START {					\
		if ((obj)) {				\
			(obj)->type = (_type);		\
			(obj)->v.sym = (val);		\
		}					\
	} G_STMT_END
#define HG_VALUE_SET_BOOLEAN(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_BOOLEAN, boolean, val); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_INTEGER(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_INTEGER, integer, val); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_REAL(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_REAL, real, val);	\
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_NAME(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_NAME, pointer, val); \
	hg_value_node_unrestorable(obj)
#define HG_VALUE_SET_NAME_STATIC(pool, obj, val)			\
	G_STMT_START {							\
		size_t __hg_value_name_length = strlen(val);		\
		gchar *__hg_value_name_static = hg_mem_alloc(pool, __hg_value_name_length + 1); \
		memcpy(__hg_value_name_static, (val), __hg_value_name_length); \
		HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_NAME, pointer, __hg_value_name_static); \
		hg_value_node_unrestorable(obj);			\
	} G_STMT_END
#define HG_VALUE_SET_ARRAY(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_ARRAY, pointer, val); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_STRING(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_STRING, pointer, val); \
	hg_value_node_unrestorable(obj)
#define HG_VALUE_SET_DICT(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_DICT, pointer, val); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_NULL(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_NULL, pointer, val); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_POINTER(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_POINTER, pointer, val); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_MARK(obj)						\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_MARK, pointer, NULL); \
	hg_value_node_restorable(obj)
#define HG_VALUE_SET_FILE(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_FILE, pointer, val); \
	hg_value_node_unrestorable(obj)
#define HG_VALUE_SET_SNAPSHOT(obj, val)					\
	HG_VALUE_SET_VALUE_NODE (obj, HG_TYPE_VALUE_SNAPSHOT, pointer, val); \
	hg_value_node_unrestorable(obj)

#define HG_VALUE_MAKE_VALUE_NODE(pool, obj, _type, sym, val)	\
	G_STMT_START {						\
		(obj) = hg_value_node_new((pool));		\
		HG_VALUE_SET_VALUE_NODE (obj, _type, sym, val);	\
	} G_STMT_END
#define HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL(obj, _type, sym, val)	\
	G_STMT_START {							\
		HgMemObject *__hg_value_node_obj;			\
									\
		hg_mem_get_object__inline((val), __hg_value_node_obj);	\
		if (__hg_value_node_obj) {				\
			HG_VALUE_MAKE_VALUE_NODE (__hg_value_node_obj->pool, \
						  obj, _type,		\
						  sym, val);		\
		} else {						\
			(obj) = NULL;					\
		}							\
	} G_STMT_END
#define HG_VALUE_MAKE_BOOLEAN(pool, obj, val)				\
	HG_VALUE_MAKE_VALUE_NODE (pool, obj, HG_TYPE_VALUE_BOOLEAN, boolean, val)
#define HG_VALUE_MAKE_INTEGER(pool, obj, val)				\
	HG_VALUE_MAKE_VALUE_NODE (pool, obj, HG_TYPE_VALUE_INTEGER, integer, val)
#define HG_VALUE_MAKE_REAL(pool, obj, val)				\
	HG_VALUE_MAKE_VALUE_NODE (pool, obj, HG_TYPE_VALUE_REAL, real, val)
#define HG_VALUE_MAKE_NAME(obj, val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_NAME, pointer, val)
#define HG_VALUE_MAKE_NAME_STATIC(pool, obj, val)			\
	G_STMT_START {							\
		size_t __hg_value_name_length = strlen(val);		\
		gchar *__hg_value_name_static = hg_mem_alloc((pool), __hg_value_name_length + 1); \
		memcpy(__hg_value_name_static, (val), __hg_value_name_length); \
		__hg_value_name_static[__hg_value_name_length] = 0;	\
		HG_VALUE_MAKE_VALUE_NODE ((pool), (obj), HG_TYPE_VALUE_NAME, pointer, __hg_value_name_static); \
	} G_STMT_END
#define HG_VALUE_MAKE_ARRAY(obj, val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_ARRAY, pointer, val)
#define HG_VALUE_MAKE_STRING(obj, val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_STRING, pointer, val)
#define HG_VALUE_MAKE_DICT(obj, val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_DICT, pointer, val)
#define HG_VALUE_MAKE_NULL(pool, obj, val)				\
	HG_VALUE_MAKE_VALUE_NODE (pool, obj, HG_TYPE_VALUE_NULL, pointer, val)
#define HG_VALUE_MAKE_POINTER(obj, val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_POINTER, pointer, val)
#define HG_VALUE_MAKE_MARK(pool, obj)					\
	HG_VALUE_MAKE_VALUE_NODE (pool, obj, HG_TYPE_VALUE_MARK, pointer, NULL)
#define HG_VALUE_MAKE_FILE(obj, val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_FILE, pointer, val)
#define HG_VALUE_MAKE_SNAPSHOT(obj, val)				\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (obj, HG_TYPE_VALUE_SNAPSHOT, pointer, val)

#define HG_VALUE_GET_VALUE_NODE(obj, sym)	((obj)->v.sym)
#define HG_VALUE_GET_BOOLEAN(obj)		HG_VALUE_GET_VALUE_NODE (obj, boolean)
#define HG_VALUE_GET_INTEGER(obj)		HG_VALUE_GET_VALUE_NODE (obj, integer)
#define HG_VALUE_GET_INTEGER_FROM_REAL(obj)	(gint32)HG_VALUE_GET_VALUE_NODE (obj, real)
#define HG_VALUE_GET_REAL(obj)			HG_VALUE_GET_VALUE_NODE (obj, real)
#define HG_VALUE_GET_REAL_FROM_INTEGER(obj)	(gdouble)HG_VALUE_GET_VALUE_NODE (obj, integer)
#define HG_VALUE_GET_NAME(obj)			(gchar *)HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_ARRAY(obj)			(HgArray *)HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_STRING(obj)		(HgString *)HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_DICT(obj)			(HgDict *)HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_NULL(obj)			HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_POINTER(obj)		HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_MARK(obj)			HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_FILE(obj)			HG_VALUE_GET_VALUE_NODE (obj, pointer)
#define HG_VALUE_GET_SNAPSHOT(obj)		HG_VALUE_GET_VALUE_NODE (obj, pointer)

#define HG_VALUE_IS(obj, _type)			((obj)->type == (_type))
#define HG_IS_VALUE_BOOLEAN(obj)		HG_VALUE_IS (obj, HG_TYPE_VALUE_BOOLEAN)
#define HG_IS_VALUE_INTEGER(obj)		HG_VALUE_IS (obj, HG_TYPE_VALUE_INTEGER)
#define HG_IS_VALUE_REAL(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_REAL)
#define HG_IS_VALUE_NAME(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_NAME)
#define HG_IS_VALUE_ARRAY(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_ARRAY)
#define HG_IS_VALUE_STRING(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_STRING)
#define HG_IS_VALUE_DICT(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_DICT)
#define HG_IS_VALUE_NULL(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_NULL)
#define HG_IS_VALUE_POINTER(obj)		HG_VALUE_IS (obj, HG_TYPE_VALUE_POINTER)
#define HG_IS_VALUE_MARK(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_MARK)
#define HG_IS_VALUE_FILE(obj)			HG_VALUE_IS (obj, HG_TYPE_VALUE_FILE)
#define HG_IS_VALUE_SNAPSHOT(obj)		HG_VALUE_IS (obj, HG_TYPE_VALUE_SNAPSHOT)

#define HG_VALUE_REAL_SIMILAR(x, y)		(fabs(x - y) <= fabs(DBL_EPSILON * x))

#define hg_value_node_restorable(node)				\
	G_STMT_START {						\
		HgMemObject *__obj__;				\
		hg_mem_get_object__inline((node), __obj__);	\
		hg_mem_restorable(__obj__);			\
	} G_STMT_END
#define hg_value_node_unrestorable(node)			\
	G_STMT_START {						\
		HgMemObject *__obj__;				\
		hg_mem_get_object__inline((node), __obj__);	\
		hg_mem_unrestorable(__obj__);			\
	} G_STMT_END
#define hg_value_node_is_restorable(node)				\
	hg_mem_is_restorable(hg_mem_get_object__inline_nocheck(node))

HgValueNode *hg_value_node_new     (HgMemPool         *pool);
gsize        hg_value_node_get_hash(const HgValueNode *node);
gboolean     hg_value_node_compare (const HgValueNode *a,
				    const HgValueNode *b);

void         hg_value_node_debug_print(HgFileObject    *file,
				       HgDebugStateType type,
				       HgValueType      vtype,
				       gpointer         parent,
				       gpointer         self,
				       gpointer         extrainfo);

G_END_DECLS

#endif /* __HG_VALUE_NODE_H__ */
