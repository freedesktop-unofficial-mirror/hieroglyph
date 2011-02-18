/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.h
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

#ifndef __HIEROGLYPH_HGOBJECT_H__
#define __HIEROGLYPH_HGOBJECT_H__

#include <stdarg.h>
#include <hieroglyph/hgerror.h>
#include <hieroglyph/hgquark.h>

HG_BEGIN_DECLS

#define HG_DEFINE_VTABLE(_name_)					\
	static void             _hg_object_ ## _name_ ## _free                (hg_object_t            *object); \
	HG_DEFINE_VTABLE_WITH(_name_, _hg_object_ ## _name_ ## _free, NULL, NULL)

#define HG_PROTO_VTABLE_ACL(_name_)					\
	static void             _hg_object_ ## _name_ ## _set_acl             (hg_object_t            *object, \
									       hg_int_t                readable, \
									       hg_int_t                writable, \
									       hg_int_t                executable, \
									       hg_int_t                editable); \
	static hg_quark_acl_t  _hg_object_ ## _name_ ## _get_acl              (hg_object_t            *object)

#define HG_DEFINE_VTABLE_WITH(_name_, _free_, _set_acl_, _get_acl_)	\
	static hg_usize_t      _hg_object_ ## _name_ ## _get_capsulated_size (void); \
	static hg_uint_t       _hg_object_ ## _name_ ## _get_allocation_flags(void); \
	static hg_bool_t       _hg_object_ ## _name_ ## _initialize          (hg_object_t             *object, \
									      va_list                  args); \
	static hg_quark_t      _hg_object_ ## _name_ ## _copy                (hg_object_t             *object, \
									      hg_quark_iterate_func_t  func, \
									      hg_pointer_t             user_data, \
									      hg_pointer_t            *ret); \
	static hg_char_t      *_hg_object_ ## _name_ ## _to_cstr             (hg_object_t             *object, \
									      hg_quark_iterate_func_t  func, \
									      hg_pointer_t             user_data); \
	static hg_bool_t      _hg_object_ ## _name_ ## _gc_mark              (hg_object_t             *object, \
									      hg_gc_iterate_func_t     func, \
									      hg_pointer_t             user_data); \
	static hg_bool_t      _hg_object_ ## _name_ ## _compare              (hg_object_t             *o1,	\
									      hg_object_t             *o2, \
									      hg_quark_compare_func_t  func, \
									      hg_pointer_t             user_data); \
									\
	static hg_object_vtable_t __hg_object_ ## _name_ ## _vtable = {	\
		.get_capsulated_size  = _hg_object_ ## _name_ ## _get_capsulated_size, \
		.get_allocation_flags = _hg_object_ ## _name_ ## _get_allocation_flags,	\
		.initialize           = _hg_object_ ## _name_ ## _initialize, \
		.free                 = _free_,				\
		.copy                 = _hg_object_ ## _name_ ## _copy,	\
		.to_cstr              = _hg_object_ ## _name_ ## _to_cstr, \
		.gc_mark              = _hg_object_ ## _name_ ## _gc_mark, \
		.compare              = _hg_object_ ## _name_ ## _compare, \
		.set_acl              = _set_acl_,			\
		.get_acl              =	_get_acl_,			\
	};								\
									\
	hg_object_vtable_t *						\
	hg_object_ ## _name_ ## _get_vtable(void)			\
	{								\
		return &__hg_object_ ## _name_ ## _vtable;		\
	}

typedef struct _hg_object_vtable_t		hg_object_vtable_t;
typedef struct _hg_object_t			hg_object_t;
typedef hg_bool_t (* hg_gc_iterate_func_t)	(hg_quark_t    qdata,
						 hg_pointer_t  user_data);
typedef void (* hg_destroy_func_t)		(hg_mem_t     *mem,
						 hg_pointer_t  user_data);


struct _hg_object_vtable_t {
	hg_usize_t       (* get_capsulated_size)  (void);
	hg_uint_t        (* get_allocation_flags) (void);
	hg_bool_t        (* initialize)           (hg_object_t             *object,
						   va_list                  args);
	void             (* free)                 (hg_object_t             *object);
	hg_quark_t       (* copy)                 (hg_object_t             *object,
						   hg_quark_iterate_func_t  func,
						   hg_pointer_t             user_data,
						   hg_pointer_t            *ret);
	hg_char_t      * (* to_cstr)              (hg_object_t             *object,
						   hg_quark_iterate_func_t  func,
						   hg_pointer_t             user_data);
	hg_bool_t       (* gc_mark)               (hg_object_t             *object,
						   hg_gc_iterate_func_t     func,
						   hg_pointer_t             user_data);
	hg_bool_t       (* compare)               (hg_object_t             *o1,
						   hg_object_t             *o2,
						   hg_quark_compare_func_t  func,
						   hg_pointer_t             user_data);
	void             (* set_acl)              (hg_object_t             *object,
						   hg_int_t                 readable,
						   hg_int_t                 writable,
						   hg_int_t                 executable,
						   hg_int_t                 editable);
	hg_quark_acl_t   (* get_acl)              (hg_object_t             *object);
};
struct _hg_object_t {
	hg_mem_t       *mem;
	hg_quark_t      self;
	hg_type_t       type;
	hg_quark_t      on_copying;
	hg_bool_t       on_gc:1;
	hg_bool_t       on_to_cstr:1;
	hg_bool_t       on_compare:1;
	hg_quark_acl_t  acl;
};


void            hg_object_init      (void);
void            hg_object_tini      (void);
hg_bool_t       hg_object_register  (hg_type_t                type,
                                     hg_object_vtable_t      *vtable);
hg_quark_t      hg_object_new       (hg_mem_t                *mem,
                                     hg_pointer_t            *ret,
                                     hg_type_t                type,
                                     hg_usize_t               preallocated_size,
				     ...);
void            hg_object_free      (hg_mem_t                *mem,
                                     hg_quark_t               index);
hg_quark_t      hg_object_copy      (hg_object_t             *object,
                                     hg_quark_iterate_func_t  func,
                                     hg_pointer_t             user_data,
                                     hg_pointer_t            *ret);
hg_char_t      *hg_object_to_cstr   (hg_object_t             *object,
                                     hg_quark_iterate_func_t  func,
                                     hg_pointer_t             user_data);
hg_bool_t       hg_object_gc_mark   (hg_object_t             *object,
                                     hg_gc_iterate_func_t     func,
                                     hg_pointer_t             user_data);
hg_bool_t       hg_object_compare   (hg_object_t             *o1,
                                     hg_object_t             *o2,
                                     hg_quark_compare_func_t  func,
                                     hg_pointer_t             user_data);
void            hg_object_set_acl   (hg_object_t             *object,
                                     hg_int_t                 readable,
                                     hg_int_t                 writable,
                                     hg_int_t                 executable,
                                     hg_int_t                 editable);
hg_quark_acl_t  hg_object_get_acl   (hg_object_t             *object);
hg_quark_t      hg_object_quark_copy(hg_mem_t                *mem,
                                     hg_quark_t               qdata,
                                     hg_pointer_t            *ret);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGOBJECT_H__ */
