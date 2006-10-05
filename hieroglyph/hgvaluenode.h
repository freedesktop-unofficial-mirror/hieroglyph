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

#define HG_VALUE_GET_VALUE_TYPE(_obj)		((HgValueType)HG_OBJECT_GET_USER_DATA (&(_obj)->object))
#define HG_VALUE_SET_VALUE_TYPE(_obj, _type)	(HG_OBJECT_SET_USER_DATA (&(_obj)->object, (_type)))

#define HG_VALUE_SET_VALUE_NODE(_obj, _type, _klass, _val)		\
	G_STMT_START {							\
		if ((_obj)) {						\
			HG_VALUE_SET_VALUE_TYPE ((_obj), (_type));	\
			((HgValueNode ## _klass *)(_obj))->value = (_val); \
		}							\
	} G_STMT_END
#define HG_VALUE_SET_BOOLEAN(_obj, _val)				\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_BOOLEAN, Boolean, _val); \
	hg_value_node_restorable(_obj)
#define HG_VALUE_SET_INTEGER(_obj, _val)				\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_INTEGER, Integer, _val);	\
	hg_value_node_restorable(_obj)
#define HG_VALUE_SET_REAL(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_REAL, Real, _val);	\
	hg_value_node_restorable(_obj)
#define HG_VALUE_SET_NAME(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_NAME, Name, _val);	\
	hg_value_node_unrestorable(_obj)
#define HG_VALUE_SET_NAME_STATIC(_pool, _obj, _val)			\
	G_STMT_START {							\
		size_t __hg_value_name_length = strlen(_val);		\
		gchar *__hg_value_name_static = hg_mem_alloc(_pool, __hg_value_name_length + 1); \
		memcpy(__hg_value_name_static, (_val), __hg_value_name_length); \
		HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_NAME, Name, __hg_value_name_static); \
		hg_value_node_unrestorable(_obj);			\
	} G_STMT_END
#define HG_VALUE_SET_ARRAY(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_ARRAY, Array, _val); \
	hg_value_node_restorable(_obj);					\
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_SET_STRING(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_STRING, String, _val); \
	hg_value_node_unrestorable(_obj);				\
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_SET_DICT(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_DICT, Dict, _val);	\
	hg_value_node_restorable(_obj);					\
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_SET_NULL(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_NULL, Null, _val);	\
	hg_value_node_restorable(_obj)
#define HG_VALUE_SET_OPERATOR(_obj, _val)				\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_OPERATOR, Operator, _val); \
	hg_value_node_restorable(_obj);					\
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_SET_MARK(_obj)						\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_MARK, Mark, NULL);	\
	hg_value_node_restorable(_obj)
#define HG_VALUE_SET_FILE(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_FILE, File, _val);	\
	hg_value_node_unrestorable(_obj);				\
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_SET_SNAPSHOT(_obj, _val)				\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_SNAPSHOT, Snapshot, _val); \
	hg_value_node_unrestorable(_obj);				\
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_SET_PLUGIN(_obj, _val)					\
	HG_VALUE_SET_VALUE_NODE (_obj, HG_TYPE_VALUE_PLUGIN, Plugin, _val); \
	hg_value_node_unrestorable(_obj);				\
	hg_value_node_inherit_complex(_obj, _val)

#define HG_VALUE_MAKE_VALUE_NODE(_pool, _obj, _type, _sym, _val)	\
	G_STMT_START {							\
		(_obj) = hg_value_node_new((_pool), (_type));		\
		HG_VALUE_SET_VALUE_NODE (_obj, _type, _sym, _val);	\
	} G_STMT_END
#define HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL(_obj, _type, _sym, _val) \
	G_STMT_START {							\
		HgMemObject *__hg_value_node_obj;			\
									\
		hg_mem_get_object__inline((_val), __hg_value_node_obj);	\
		if (__hg_value_node_obj) {				\
			HG_VALUE_MAKE_VALUE_NODE (__hg_value_node_obj->pool, \
						  _obj, _type,		\
						  _sym, _val);		\
		} else {						\
			(_obj) = NULL;					\
		}							\
	} G_STMT_END
#define HG_VALUE_MAKE_BOOLEAN(_pool, _obj, _val)			\
	HG_VALUE_MAKE_VALUE_NODE (_pool, _obj, HG_TYPE_VALUE_BOOLEAN, Boolean, _val)
#define HG_VALUE_MAKE_INTEGER(_pool, _obj, _val)			\
	HG_VALUE_MAKE_VALUE_NODE (_pool, _obj, HG_TYPE_VALUE_INTEGER, Integer, _val)
#define HG_VALUE_MAKE_REAL(_pool, _obj, _val)				\
	HG_VALUE_MAKE_VALUE_NODE (_pool, _obj, HG_TYPE_VALUE_REAL, Real, _val)
#define HG_VALUE_MAKE_NAME(_obj, _val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_NAME, Name, _val)
#define HG_VALUE_MAKE_NAME_STATIC(_pool, _obj, _val)			\
	G_STMT_START {							\
		size_t __hg_value_name_length = strlen(_val);		\
		gchar *__hg_value_name_static = hg_mem_alloc((_pool), __hg_value_name_length + 1); \
		memcpy(__hg_value_name_static, (_val), __hg_value_name_length); \
		__hg_value_name_static[__hg_value_name_length] = 0;	\
		HG_VALUE_MAKE_VALUE_NODE ((_pool), (_obj), HG_TYPE_VALUE_NAME, Name, __hg_value_name_static); \
	} G_STMT_END
#define HG_VALUE_MAKE_ARRAY(_obj, _val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_ARRAY, Array, _val); \
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_MAKE_STRING(_obj, _val)				\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_STRING, String, _val); \
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_MAKE_DICT(_obj, _val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_DICT, Dict, _val); \
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_MAKE_NULL(_pool, _obj, _val)				\
	HG_VALUE_MAKE_VALUE_NODE (_pool, _obj, HG_TYPE_VALUE_NULL, Null, _val)
#define HG_VALUE_MAKE_OPERATOR(_obj, _val)				\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_OPERATOR, Operator, _val); \
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_MAKE_MARK(_pool, _obj)					\
	HG_VALUE_MAKE_VALUE_NODE (_pool, _obj, HG_TYPE_VALUE_MARK, Mark, NULL)
#define HG_VALUE_MAKE_FILE(_obj, _val)					\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_FILE, File, _val); \
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_MAKE_SNAPSHOT(_obj, _val)				\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_SNAPSHOT, Snapshot, _val); \
	hg_value_node_inherit_complex(_obj, _val)
#define HG_VALUE_MAKE_PLUGIN(_obj, _val)				\
	HG_VALUE_MAKE_VALUE_NODE_WITH_SAME_POOL (_obj, HG_TYPE_VALUE_PLUGIN, Plugin, _val); \
	hg_value_node_inherit_complex(_obj, _val)

#define HG_VALUE_GET_VALUE_NODE(_obj, _type)	(((HgValueNode ## _type *)(_obj))->value)
#define HG_VALUE_GET_BOOLEAN(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, Boolean)
#define HG_VALUE_GET_INTEGER(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, Integer)
#define HG_VALUE_GET_INTEGER_FROM_REAL(_obj)	(gint32)HG_VALUE_GET_VALUE_NODE (_obj, Real)
#define HG_VALUE_GET_REAL(_obj)			HG_VALUE_GET_VALUE_NODE (_obj, Real)
#define HG_VALUE_GET_REAL_FROM_INTEGER(_obj)	(gdouble)HG_VALUE_GET_VALUE_NODE (_obj, Integer)
#define HG_VALUE_GET_NAME(_obj)			HG_VALUE_GET_VALUE_NODE (_obj, Name)
#define HG_VALUE_GET_ARRAY(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, Array)
#define HG_VALUE_GET_STRING(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, String)
#define HG_VALUE_GET_DICT(_obj)			HG_VALUE_GET_VALUE_NODE (_obj, Dict)
#define HG_VALUE_GET_NULL(_obj)			HG_VALUE_GET_VALUE_NODE (_obj, Null)
#define HG_VALUE_GET_OPERATOR(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, Operator)
#define HG_VALUE_GET_MARK(_obj)			HG_VALUE_GET_VALUE_NODE (_obj, Mark)
#define HG_VALUE_GET_FILE(_obj)			HG_VALUE_GET_VALUE_NODE (_obj, File)
#define HG_VALUE_GET_SNAPSHOT(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, Snapshot)
#define HG_VALUE_GET_PLUGIN(_obj)		HG_VALUE_GET_VALUE_NODE (_obj, Plugin)

#define HG_VALUE_IS(_obj, _type)		(HG_OBJECT_GET_USER_DATA (&(_obj)->object) == (_type))
#define HG_IS_VALUE_BOOLEAN(_obj)		HG_VALUE_IS (_obj, HG_TYPE_VALUE_BOOLEAN)
#define HG_IS_VALUE_INTEGER(_obj)		HG_VALUE_IS (_obj, HG_TYPE_VALUE_INTEGER)
#define HG_IS_VALUE_REAL(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_REAL)
#define HG_IS_VALUE_NAME(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_NAME)
#define HG_IS_VALUE_ARRAY(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_ARRAY)
#define HG_IS_VALUE_STRING(_obj)		HG_VALUE_IS (_obj, HG_TYPE_VALUE_STRING)
#define HG_IS_VALUE_DICT(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_DICT)
#define HG_IS_VALUE_NULL(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_NULL)
#define HG_IS_VALUE_OPERATOR(_obj)		HG_VALUE_IS (_obj, HG_TYPE_VALUE_OPERATOR)
#define HG_IS_VALUE_MARK(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_MARK)
#define HG_IS_VALUE_FILE(_obj)			HG_VALUE_IS (_obj, HG_TYPE_VALUE_FILE)
#define HG_IS_VALUE_SNAPSHOT(_obj)		HG_VALUE_IS (_obj, HG_TYPE_VALUE_SNAPSHOT)
#define HG_IS_VALUE_PLUGIN(_obj)		HG_VALUE_IS (_obj, HG_TYPE_VALUE_PLUGIN)

/* this is not stricter checking though, we may not need that precision */
#define HG_VALUE_REAL_SIMILAR(_x, _y)		(fabs((_x) - (_y)) <= DBL_EPSILON)

#define hg_value_node_restorable(_node)				\
	G_STMT_START {						\
		HgMemObject *__obj__;				\
		hg_mem_get_object__inline((_node), __obj__);	\
		hg_mem_restorable(__obj__);			\
	} G_STMT_END
#define hg_value_node_unrestorable(_node)			\
	G_STMT_START {						\
		HgMemObject *__obj__;				\
		hg_mem_get_object__inline((_node), __obj__);	\
		hg_mem_unrestorable(__obj__);			\
	} G_STMT_END
#define hg_value_node_is_restorable(_node)				\
	hg_mem_is_restorable(hg_mem_get_object__inline_nocheck(_node))
#define hg_value_node_inherit_complex(_node, _hobj)		\
	G_STMT_START {						\
		HgMemObject *__obj__, *__obj2__;		\
		hg_mem_get_object__inline((_node), __obj__);	\
		hg_mem_get_object__inline((_hobj), __obj2__);	\
		if (hg_mem_is_complex_mark(__obj2__)) {		\
			hg_mem_complex_mark(__obj__);		\
		} else {					\
			hg_mem_complex_unmark(__obj__);		\
		}						\
	} G_STMT_END
#define hg_value_node_has_complex_object(_node)				\
	hg_mem_is_complex_mark(hg_mem_get_object__inline_nocheck(_node))

void         hg_value_node_init           (void);
void         hg_value_node_finalize       (void);
gboolean     hg_value_node_register_type  (const HgValueNodeTypeInfo *info);
const gchar *hg_value_node_get_type_name  (guint                      type_id);

HgValueNode *hg_value_node_new            (HgMemPool         *pool,
					   guint              type_id);
gsize        hg_value_node_get_hash       (const HgValueNode *node);
gboolean     hg_value_node_compare        (const HgValueNode *a,
					   const HgValueNode *b);
gboolean     hg_value_node_compare_content(const HgValueNode *a,
					   const HgValueNode *b);

void         hg_value_node_debug_print    (HgFileObject      *file,
					   HgDebugStateType   type,
					   HgValueType        vtype,
					   gpointer           parent,
					   gpointer           self,
					   gpointer           extrainfo);

G_END_DECLS

#endif /* __HG_VALUE_NODE_H__ */
