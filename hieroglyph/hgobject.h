/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.h
 * Copyright (C) 2005-2007 Akira TAGOH
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
#ifndef __HIEROGLYPH__HGOBJECT_H__
#define __HIEROGLYPH__HGOBJECT_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS

#define HG_OBJECT_OBJECT(_hg_o_)		(&((_hg_o_)->object))
#define HG_OBJECT_HEADER(_hg_o_)		(&((_hg_o_)->header))
#define HG_OBJECT_DATA(_hg_o_)			(&(_hg_o_)->data)
#define HG_OBJECT_GET_TYPE(_hg_o_)		(HG_OBJECT_OBJECT (_hg_o_)->t.v.object_type)
#define HG_OBJECT_IS(_hg_o_,_hg_t_)		(HG_OBJECT_GET_TYPE (_hg_o_) == HG_OBJECT_TYPE_ ## _hg_t_)
#define HG_OBJECT_IS_NULL(_hg_o_)		HG_OBJECT_IS (_hg_o_, NULL)
#define HG_OBJECT_IS_INTEGER(_hg_o_)		HG_OBJECT_IS (_hg_o_, INTEGER)
#define HG_OBJECT_IS_REAL(_hg_o_)		HG_OBJECT_IS (_hg_o_, REAL)
#define HG_OBJECT_IS_NAME(_hg_o_)		HG_OBJECT_IS (_hg_o_, NAME)
#define HG_OBJECT_IS_BOOLEAN(_hg_o_)		HG_OBJECT_IS (_hg_o_, BOOLEAN)
#define HG_OBJECT_IS_STRING(_hg_o_)		HG_OBJECT_IS (_hg_o_, STRING)
#define HG_OBJECT_IS_EVAL(_hg_o_)		HG_OBJECT_IS (_hg_o_, EVAL)
#define HG_OBJECT_IS_ARRAY(_hg_o_)		HG_OBJECT_IS (_hg_o_, ARRAY)
#define HG_OBJECT_IS_MARK(_hg_o_)		HG_OBJECT_IS (_hg_o_, MARK)
#define HG_OBJECT_IS_DICT(_hg_o_)		HG_OBJECT_IS (_hg_o_, DICT)
#define HG_OBJECT_IS_FILE(_hg_o_)		HG_OBJECT_IS (_hg_o_, FILE)
#define HG_OBJECT_IS_OPERATOR(_hg_o_)		HG_OBJECT_IS (_hg_o_, OPERATOR)
#define HG_OBJECT_IS_COMPLEX(_hg_o_)		(HG_OBJECT_IS_ARRAY (_hg_o_) || \
						 HG_OBJECT_IS_STRING (_hg_o_) || \
						 HG_OBJECT_IS_DICT (_hg_o_) || \
						 HG_OBJECT_IS_FILE (_hg_o_) || \
						 HG_OBJECT_IS_OPERATOR (_hg_o_))
#define HG_OBJECT_ATTR_IS(_hg_o_,_hg_a_)	((HG_OBJECT_OBJECT (_hg_o_)->attr.bit.is_ ## _hg_a_) == TRUE)
#define HG_OBJECT_ATTR_IS_ACCESSIBLE(_hg_o_)	HG_OBJECT_ATTR_IS (_hg_o_, accessible)
#define HG_OBJECT_ATTR_IS_READABLE(_hg_o_)	HG_OBJECT_ATTR_IS (_hg_o_, readable)
#define HG_OBJECT_ATTR_IS_EXECUTEONLY(_hg_o_)	HG_OBJECT_ATTR_IS (_hg_o_, executeonly)
#define HG_OBJECT_ATTR_IS_GLOBAL(_hg_o_)	HG_OBJECT_ATTR_IS (_hg_o_, global)
#define HG_OBJECT_ATTR_IS_EXECUTABLE(_hg_o_)	(HG_OBJECT_OBJECT (_hg_o_)->t.v.is_executable)
#define HG_OBJECT_GET_N_OBJECTS(_hg_o_)		HG_OBJECT_HEADER (_hg_o_)->n_objects
#define HG_OBJECT_CHECK_N_OBJECTS(_hg_o_,_hg_n_)	\
						(_hg_n_ < HG_OBJECT_GET_N_OBJECTS (_hg_o_))
#define HG_OBJECT_INTEGER(_hg_o_)		(HG_OBJECT_OBJECT (_hg_o_)->v.integer)
#define HG_OBJECT_REAL(_hg_o_)			(HG_OBJECT_OBJECT (_hg_o_)->v.real)
#define HG_OBJECT_REAL_IS_EQUAL(_hg_o_1,_hg_o_2)	\
						(fabsf(HG_OBJECT_REAL (_hg_o_1) - HG_OBJECT_REAL (_hg_o_2)) <= FLT_EPSILON)
#define HG_OBJECT_REAL_IS_ZERO(_hg_o_)		(fabsf(HG_OBJECT_REAL (_hg_o_)) <= FLT_EPSILON)
#define HG_OBJECT_NAME(_hg_o_)			(&(HG_OBJECT_OBJECT (_hg_o_)->v.name))
#define HG_OBJECT_NAME_DATA(_hg_o_)		(gchar *)(HG_OBJECT_DATA (_hg_o_))
#define HG_OBJECT_ENCODING_NAME(_hg_o_)		(&(HG_OBJECT_OBJECT (_hg_o_)->v.encoding))
#define HG_OBJECT_BOOLEAN(_hg_o_)		(HG_OBJECT_OBJECT (_hg_o_)->v.boolean)
#define HG_OBJECT_STRING(_hg_o_)		(&(HG_OBJECT_OBJECT (_hg_o_)->v.string))
#define HG_OBJECT_STRING_DATA(_hg_o_)		((hg_stringdata_t *)HG_OBJECT_DATA (_hg_o_))
#define HG_OBJECT_ARRAY(_hg_o_)			(&(HG_OBJECT_OBJECT (_hg_o_)->v.array))
#define HG_OBJECT_ARRAY_DATA(_hg_o_)		((hg_arraydata_t *)HG_OBJECT_DATA (_hg_o_))
#define HG_OBJECT_DICT(_hg_o_)			(&(HG_OBJECT_OBJECT (_hg_o_)->v.dict))
#define HG_OBJECT_DICT_DATA(_hg_o_)		((hg_dictdata_t *)HG_OBJECT_DATA (_hg_o_))
#define HG_OBJECT_FILE(_hg_o_)			(&(HG_OBJECT_OBJECT (_hg_o_)->v.file))
#define HG_OBJECT_FILE_DATA(_hg_o_)		((hg_filedata_t *)HG_OBJECT_DATA (_hg_o_))
#define HG_OBJECT_OPERATOR(_hg_o_)		(&(HG_OBJECT_OBJECT (_hg_o_)->v.operator))
#define HG_OBJECT_OPERATOR_DATA(_hg_o_)		((hg_operatordata_t *)HG_OBJECT_DATA (_hg_o_))


hg_object_t *hg_object_new                (hg_vm_t     *vm,
					   guint16      n_objects) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_sized_new          (hg_vm_t     *vm,
					   gsize        size) G_GNUC_WARN_UNUSED_RESULT;
void         hg_object_free               (hg_vm_t     *vm,
					   hg_object_t *object);
hg_object_t *hg_object_dup                (hg_vm_t     *vm,
					   hg_object_t *object) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_copy               (hg_vm_t     *vm,
					   hg_object_t *object) G_GNUC_WARN_UNUSED_RESULT;
gboolean     hg_object_compare            (hg_object_t *object1,
					   hg_object_t *object2);
gchar       *hg_object_dump               (hg_object_t *object,
					   gboolean     verbose) G_GNUC_MALLOC;
gsize        hg_object_get_hash           (hg_object_t *object);
hg_object_t *hg_object_null_new           (hg_vm_t     *vm) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_integer_new        (hg_vm_t     *vm,
					   gint32       value) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_real_new           (hg_vm_t     *vm,
					   gfloat       value) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_name_new           (hg_vm_t     *vm,
					   const gchar *value,
					   gboolean     is_evaluated) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_system_encoding_new(hg_vm_t     *vm,
					   guint32      index,
					   gboolean     is_evaluated) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_boolean_new        (hg_vm_t     *vm,
					   gboolean     value) G_GNUC_WARN_UNUSED_RESULT;
hg_object_t *hg_object_mark_new           (hg_vm_t     *vm) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* __HIEROGLYPH__HGOBJECT_H__ */
