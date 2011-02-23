/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.h
 * Copyright (C) 2005-2011 Akira TAGOH
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

#ifndef __HIEROGLYPH_HGARRAY_H__
#define __HIEROGLYPH_HGARRAY_H__

#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgmatrix.h>

HG_BEGIN_DECLS

#define HG_ARRAY_INIT						\
	(hg_object_register(HG_TYPE_ARRAY,			\
			    hg_object_array_get_vtable()))
#define HG_IS_QARRAY(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_ARRAY)

typedef struct _hg_array_t	hg_array_t;
typedef hg_cb_BOOL__QUARK_t	hg_array_traverse_func_t;

struct _hg_array_t {
	hg_object_t o;
	hg_quark_t  qcontainer;
	hg_quark_t  qname;
	hg_uint16_t offset;
	hg_uint16_t length;
	hg_usize_t  allocated_size;
	hg_bool_t   is_fixed_size:1;
	hg_bool_t   is_subarray:1;
};


hg_object_vtable_t *hg_object_array_get_vtable(void) HG_GNUC_CONST;
hg_quark_t          hg_array_new              (hg_mem_t                 *mem,
                                               hg_usize_t                size,
                                               hg_pointer_t             *ret);
hg_bool_t           hg_array_set              (hg_array_t               *array,
                                               hg_quark_t                quark,
                                               hg_usize_t                index,
                                               hg_bool_t                 force);
hg_quark_t          hg_array_get              (hg_array_t               *array,
                                               hg_usize_t                index);
hg_bool_t           hg_array_insert           (hg_array_t               *array,
                                               hg_quark_t                quark,
                                               hg_size_t                 pos);
hg_bool_t           hg_array_remove           (hg_array_t               *array,
                                               hg_usize_t                pos);
hg_usize_t          hg_array_length           (hg_array_t               *array);
hg_usize_t          hg_array_maxlength        (hg_array_t               *array);
void                hg_array_foreach          (hg_array_t               *array,
                                               hg_array_traverse_func_t  func,
                                               hg_pointer_t              data);
void                hg_array_set_name         (hg_array_t               *array,
                                               const hg_char_t          *name);
hg_quark_t          hg_array_make_subarray    (hg_array_t               *array,
                                               hg_usize_t                start_index,
                                               hg_usize_t                end_index,
                                               hg_pointer_t             *ret);
hg_bool_t           hg_array_copy_as_subarray (hg_array_t               *src,
                                               hg_array_t               *dest,
                                               hg_usize_t                start_index,
                                               hg_usize_t                end_index);
hg_bool_t           hg_array_is_matrix        (hg_array_t               *array);
hg_bool_t           hg_array_from_matrix      (hg_array_t               *array,
                                               hg_matrix_t              *matrix);
hg_bool_t           hg_array_to_matrix        (hg_array_t               *array,
                                               hg_matrix_t              *matrix);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGARRAY_H__ */
