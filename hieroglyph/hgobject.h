/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGOBJECT_H__
#define __HIEROGLYPH_HGOBJECT_H__

#include <stdarg.h>
#include <hieroglyph/hgquark.h>

G_BEGIN_DECLS

#define HG_DEFINE_VTABLE(_name_)					\
	static void        _hg_object_ ## _name_ ## _free                (hg_object_t              *object); \
	HG_DEFINE_VTABLE_WITH(_name_, _hg_object_ ## _name_ ## _free, NULL, NULL)

#define HG_PROTO_VTABLE_ATTRIBUTES(_name_)				\
	static void        _hg_object_ ## _name_ ## _set_attributes      (hg_object_t              *object, \
									  gint                      readable, \
									  gint                      writable, \
									  gint                      executable,	\
									  gint                      editable); \
	static gint        _hg_object_ ## _name_ ## _get_attributes      (hg_object_t              *object)

#define HG_DEFINE_VTABLE_WITH(_name_, _free_, _set_attr_, _get_attr_)		\
	static gsize       _hg_object_ ## _name_ ## _get_capsulated_size (void); \
	static guint       _hg_object_ ## _name_ ## _get_allocation_flags(void);	\
	static gboolean    _hg_object_ ## _name_ ## _initialize          (hg_object_t              *object, \
									  va_list                   args); \
	static hg_quark_t  _hg_object_ ## _name_ ## _copy                (hg_object_t              *object, \
									  hg_quark_iterate_func_t   func, \
									  gpointer                  user_data, \
									  gpointer                 *ret, \
									  GError                  **error); \
	static gchar      *_hg_object_ ## _name_ ## _to_cstr             (hg_object_t              *object, \
									  hg_quark_iterate_func_t   func, \
									  gpointer                  user_data, \
									  GError                  **error); \
	static gboolean    _hg_object_ ## _name_ ## _gc_mark             (hg_object_t              *object, \
									  hg_gc_iterate_func_t      func, \
									  gpointer                  user_data, \
									  GError                  **error); \
	static gboolean    _hg_object_ ## _name_ ## _compare             (hg_object_t              *o1,	\
									  hg_object_t              *o2, \
									  hg_quark_compare_func_t   func, \
									  gpointer                  user_data); \
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
		.set_attributes       = _set_attr_,			\
		.get_attributes       =	_get_attr_,			\
	};								\
									\
	hg_object_vtable_t *						\
	hg_object_ ## _name_ ## _get_vtable(void)			\
	{								\
		return &__hg_object_ ## _name_ ## _vtable;		\
	}

typedef enum _hg_object_access_t		hg_object_access_t;
typedef struct _hg_object_vtable_t		hg_object_vtable_t;
typedef struct _hg_object_t			hg_object_t;


enum _hg_object_access_t {
	HG_ACCESS_EXECUTABLE = 1 << 0,
	HG_ACCESS_READABLE   = 1 << 1,
	HG_ACCESS_WRITABLE   = 1 << 2,
	HG_ACCESS_EDITABLE   = 1 << 3,
	HG_ACCESS_END
};

struct _hg_object_vtable_t {
	gsize      (* get_capsulated_size)  (void);
	guint      (* get_allocation_flags) (void);
	gboolean   (* initialize)           (hg_object_t              *object,
					     va_list                   args);
	void       (* free)                 (hg_object_t              *object);
	hg_quark_t (* copy)                 (hg_object_t              *object,
					     hg_quark_iterate_func_t   func,
					     gpointer                  user_data,
					     gpointer                 *ret,
					     GError                  **error);
	gchar    * (* to_cstr)              (hg_object_t              *object,
					     hg_quark_iterate_func_t   func,
					     gpointer                  user_data,
					     GError                  **error);
	gboolean   (* gc_mark)              (hg_object_t              *object,
					     hg_gc_iterate_func_t      func,
					     gpointer                  user_data,
					     GError                  **error);
	gboolean   (* compare)              (hg_object_t              *o1,
					     hg_object_t              *o2,
					     hg_quark_compare_func_t   func,
					     gpointer                  user_data);
	void       (* set_attributes)       (hg_object_t              *object,
					     gint                      readable,
					     gint                      writable,
					     gint                      executable,
					     gint                      editable);
	gint       (* get_attributes)       (hg_object_t              *object);
};
struct _hg_object_t {
	hg_mem_t   *mem;
	hg_quark_t  self;
	hg_type_t   type;
	hg_quark_t  on_copying;
	gboolean    on_gc:1;
	gboolean    on_to_cstr:1;
	gboolean    on_compare:1;
	guint       attributes;
};


void        hg_object_init          (void);
void        hg_object_tini          (void);
gboolean    hg_object_register      (hg_type_t                 type,
				     hg_object_vtable_t       *vtable);
hg_quark_t  hg_object_new           (hg_mem_t                 *mem,
				     gpointer                 *ret,
				     hg_type_t                 type,
				     gsize                     preallocated_size,
				     ...);
void        hg_object_free          (hg_mem_t                 *mem,
                                     hg_quark_t                index);
hg_quark_t  hg_object_copy          (hg_object_t              *object,
                                     hg_quark_iterate_func_t   func,
                                     gpointer                  user_data,
                                     gpointer                 *ret,
                                     GError                  **error);
gchar      *hg_object_to_cstr       (hg_object_t              *object,
                                     hg_quark_iterate_func_t   func,
                                     gpointer                  user_data,
                                     GError                  **error);
gboolean    hg_object_gc_mark       (hg_object_t              *object,
                                     hg_gc_iterate_func_t      func,
                                     gpointer                  user_data,
                                     GError                  **error);
gboolean    hg_object_compare       (hg_object_t              *o1,
                                     hg_object_t              *o2,
                                     hg_quark_compare_func_t   func,
                                     gpointer                  user_data);
void        hg_object_set_attributes(hg_object_t              *object,
                                     gint                      readable,
                                     gint                      writable,
                                     gint                      executable,
				     gint                      editable);
gint        hg_object_get_attributes(hg_object_t              *object);
hg_quark_t  hg_object_quark_copy    (hg_mem_t                 *mem,
				     hg_quark_t                qdata,
				     gpointer                 *ret,
				     GError                  **error);

G_END_DECLS

#endif /* __HIEROGLYPH_HGOBJECT_H__ */
