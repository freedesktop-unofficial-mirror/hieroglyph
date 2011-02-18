/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.h
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

#ifndef __HIEROGLYPH_HGSTRING_H__
#define __HIEROGLYPH_HGSTRING_H__

#include <string.h>
#include <hieroglyph/hgobject.h>

HG_BEGIN_DECLS

#define HG_STRING_INIT						\
	(hg_object_register(HG_TYPE_STRING,			\
			    hg_object_string_get_vtable()))

#define HG_QSTRING(_m_,_s_)			\
	HG_QSTRING_LEN (_m_,_s_,strlen(_s_))
#define HG_QSTRING_LEN(_m_,_s_,_l_)					\
	(hg_string_new_with_value((_m_), (_s_), (_l_), NULL))
#define HG_IS_QSTRING(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_STRING)


typedef struct _hg_string_t	hg_string_t;

struct _hg_string_t {
	hg_object_t o;
	hg_quark_t  qstring;
	hg_uint16_t offset;
	hg_uint16_t length;
	hg_usize_t  allocated_size;
	hg_bool_t   is_fixed_size:1;
};

hg_object_vtable_t *hg_object_string_get_vtable (void) G_GNUC_CONST;
hg_quark_t          hg_string_new               (hg_mem_t          *mem,
                                                 hg_usize_t         requisition_size,
                                                 hg_pointer_t      *ret);
hg_quark_t          hg_string_new_with_value    (hg_mem_t          *mem,
                                                 const hg_char_t   *string,
                                                 hg_size_t          length,
                                                 hg_pointer_t      *ret);
void                hg_string_free              (hg_string_t       *string,
                                                 hg_bool_t          free_segment);
hg_uint_t           hg_string_length            (const hg_string_t *string);
hg_uint_t           hg_string_maxlength         (const hg_string_t *string);
hg_bool_t           hg_string_clear             (hg_string_t       *string);
hg_bool_t           hg_string_append_c          (hg_string_t       *string,
                                                 hg_char_t          c);
hg_bool_t           hg_string_append            (hg_string_t       *string,
                                                 const hg_char_t   *str,
                                                 hg_size_t          length);
hg_bool_t           hg_string_overwrite_c       (hg_string_t       *string,
                                                 hg_char_t          c,
                                                 hg_uint_t          index);
hg_bool_t           hg_string_erase             (hg_string_t       *string,
                                                 hg_size_t          pos,
                                                 hg_size_t          length);
hg_bool_t           hg_string_concat            (hg_string_t       *string1,
                                                 hg_string_t       *string2);
hg_char_t           hg_string_index             (hg_string_t       *string,
                                                 hg_uint_t          index);
hg_char_t          *hg_string_get_cstr          (hg_string_t       *string);
hg_bool_t           hg_string_fix_string_size   (hg_string_t       *string);
hg_bool_t           hg_string_compare           (hg_string_t       *a,
                                                 hg_string_t       *b);
hg_bool_t           hg_string_ncompare          (hg_string_t       *a,
                                                 hg_string_t       *b,
                                                 hg_uint_t          length);
hg_bool_t           hg_string_ncompare_with_cstr(hg_string_t       *a,
                                                 const hg_char_t   *b,
                                                 hg_size_t          length);
hg_bool_t           hg_string_append_printf     (hg_string_t       *string,
                                                 const hg_char_t   *format,
						 ...);
hg_quark_t          hg_string_make_substring    (hg_string_t       *string,
                                                 hg_size_t          start_index,
                                                 hg_size_t          end_index,
                                                 hg_pointer_t      *ret);
hg_bool_t           hg_string_copy_as_substring (hg_string_t       *src,
                                                 hg_string_t       *dest,
                                                 hg_size_t          start_index,
                                                 hg_size_t          end_index);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGSTRING_H__ */
