/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgquark.h
 * Copyright (C) 2010-2011 Akira TAGOH
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGQUARK_H__
#define __HIEROGLYPH_HGQUARK_H__

#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

typedef hg_quark_t (* hg_quark_iterate_func_t)	(hg_quark_t     qdata,
						 hg_pointer_t   user_data,
						 hg_pointer_t  *ret);
typedef hg_bool_t  (* hg_quark_compare_func_t)  (hg_quark_t     q1,
						 hg_quark_t     q2,
						 hg_pointer_t   user_data);
typedef enum _hg_quark_acl_t	hg_quark_acl_t;

/**
 * hg_quark_acl_t:
 *
 * FIXME
 */
enum _hg_quark_acl_t {
	HG_ACL_READABLE   = (1 << 0),
	HG_ACL_WRITABLE   = (1 << 1),
	HG_ACL_EXECUTABLE = (1 << 2),
	HG_ACL_ACCESSIBLE = (1 << 3)
};


hg_quark_t       hg_quark_new           (hg_type_t                type,
                                         hg_quark_t               value);
void             hg_quark_set_acl       (hg_quark_t              *quark,
                                         hg_quark_acl_t           acl);
hg_type_t        hg_quark_get_type      (hg_quark_t               quark);
hg_quark_t       hg_quark_get_value     (hg_quark_t               quark);
void             hg_quark_set_readable  (hg_quark_t              *quark,
                                         hg_bool_t                flag);
hg_bool_t        hg_quark_is_readable   (hg_quark_t               quark);
void             hg_quark_set_writable  (hg_quark_t              *quark,
                                         hg_bool_t                flag);
hg_bool_t        hg_quark_is_writable   (hg_quark_t               quark);
void             hg_quark_set_executable(hg_quark_t              *quark,
                                         hg_bool_t                flag);
hg_bool_t        hg_quark_is_executable (hg_quark_t               quark);
void             hg_quark_set_accessible(hg_quark_t              *quark,
                                         hg_bool_t                flag);
hg_bool_t        hg_quark_is_accessible (hg_quark_t               quark);
hg_uint_t        hg_quark_get_mem_id    (hg_quark_t               quark);
void             hg_quark_set_mem_id    (hg_quark_t              *quark,
                                         hg_uint_t                id);
hg_quark_t       hg_quark_get_hash      (hg_quark_t               quark);
const hg_char_t *hg_quark_get_type_name (hg_quark_t               quark);
hg_bool_t        hg_quark_compare       (hg_quark_t               qdata1,
                                         hg_quark_t               qdata2,
                                         hg_quark_compare_func_t  func,
                                         hg_pointer_t             user_data);


HG_INLINE_FUNC hg_bool_t hg_type_is_simple        (hg_type_t               type);
HG_INLINE_FUNC hg_bool_t hg_quark_is_simple_object(hg_quark_t              quark);
HG_INLINE_FUNC hg_bool_t hg_quark_has_mem_id      (hg_quark_t              quark,
						   hg_uint_t               id);

/**
 * hg_type_is_simple:
 * @type:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_bool_t
hg_type_is_simple(hg_type_t type)
{
	return type == HG_TYPE_NULL ||
		type == HG_TYPE_INT ||
		type == HG_TYPE_REAL ||
		type == HG_TYPE_BOOL ||
		type == HG_TYPE_MARK ||
		type == HG_TYPE_NAME ||
		type == HG_TYPE_EVAL_NAME;
}

/**
 * hg_quark_is_simple_object:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_bool_t
hg_quark_is_simple_object(hg_quark_t quark)
{
	hg_type_t t = hg_quark_get_type(quark);

	return hg_type_is_simple(t);
}

/**
 * hg_quark_has_same_mem_id:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_bool_t
hg_quark_has_mem_id(hg_quark_t quark,
		    hg_uint_t  id)
{
	return hg_quark_get_mem_id(quark) == id;
}

HG_END_DECLS

#endif /* __HIEROGLYPH_HGQUARK_H__ */
