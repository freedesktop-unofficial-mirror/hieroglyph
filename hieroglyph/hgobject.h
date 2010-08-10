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
	static void       _hg_object_ ## _name_ ## _free               (hg_object_t              *object); \
	HG_DEFINE_VTABLE_WITH_FREE(_name_, _hg_object_ ## _name_ ## _free)

#define HG_DEFINE_VTABLE_WITH_FREE(_name_, _free_)			\
	static gsize      _hg_object_ ## _name_ ## _get_capsulated_size(void); \
	static gboolean   _hg_object_ ## _name_ ## _initialize         (hg_object_t              *object, \
									va_list                   args); \
	static hg_quark_t _hg_object_ ## _name_ ## _copy               (hg_object_t              *object, \
									hg_quark_iterate_func_t   func, \
									gpointer                  user_data, \
									gpointer                 *ret, \
									GError                  **error); \
	static gchar    * _hg_object_ ## _name_ ## _to_cstr            (hg_object_t              *object, \
									hg_quark_iterate_func_t   func, \
									gpointer                  user_data, \
									GError                  **error); \
	static gboolean   _hg_object_ ## _name_ ## _gc_mark            (hg_object_t              *object, \
									hg_gc_iterate_func_t      func, \
									gpointer                  user_data, \
									GError                  **error); \
									\
	static hg_object_vtable_t __hg_object_ ## _name_ ## _vtable = {	\
		.get_capsulated_size = _hg_object_ ## _name_ ## _get_capsulated_size, \
		.initialize          = _hg_object_ ## _name_ ## _initialize, \
		.free                = _free_,				\
		.copy                = _hg_object_ ## _name_ ## _copy,	\
		.to_cstr             = _hg_object_ ## _name_ ## _to_cstr, \
		.gc_mark             = _hg_object_ ## _name_ ## _gc_mark,	\
	};								\
									\
	hg_object_vtable_t *						\
	hg_object_ ## _name_ ## _get_vtable(void)			\
	{								\
		return &__hg_object_ ## _name_ ## _vtable;		\
	}

typedef struct _hg_object_vtable_t		hg_object_vtable_t;
typedef struct _hg_object_t			hg_object_t;


struct _hg_object_vtable_t {
	gsize      (* get_capsulated_size) (void);
	gboolean   (* initialize)          (hg_object_t              *object,
					    va_list                   args);
	void       (* free)                (hg_object_t              *object);
	hg_quark_t (* copy)                (hg_object_t              *object,
					    hg_quark_iterate_func_t   func,
					    gpointer                  user_data,
					    gpointer                 *ret,
					    GError                  **error);
	gchar    * (* to_cstr)             (hg_object_t              *object,
					    hg_quark_iterate_func_t   func,
					    gpointer                  user_data,
					    GError                  **error);
	gboolean   (* gc_mark)             (hg_object_t              *object,
					    hg_gc_iterate_func_t      func,
					    gpointer                  user_data,
					    GError                  **error);
};
struct _hg_object_t {
	hg_mem_t   *mem;
	hg_quark_t  self;
	hg_type_t   type;
	hg_quark_t  on_copying;
};


void        hg_object_init    (void);
void        hg_object_tini    (void);
gboolean    hg_object_register(hg_type_t                 type,
			       hg_object_vtable_t       *vtable);
hg_quark_t  hg_object_new     (hg_mem_t                 *mem,
			       gpointer                 *ret,
			       hg_type_t                 type,
			       gsize                     preallocated_size,
			       ...);
void        hg_object_free    (hg_mem_t                 *mem,
			       hg_quark_t                index);
hg_quark_t  hg_object_copy    (hg_object_t              *object,
			       hg_quark_iterate_func_t   func,
			       gpointer                  user_data,
			       gpointer                 *ret,
			       GError                  **error);
gchar      *hg_object_to_cstr (hg_object_t              *object,
			       hg_quark_iterate_func_t   func,
			       gpointer                  user_data,
			       GError                  **error);
gboolean    hg_object_gc_mark (hg_object_t              *object,
			       hg_gc_iterate_func_t      func,
			       gpointer                  user_data,
			       GError                  **error);

G_END_DECLS

#endif /* __HIEROGLYPH_HGOBJECT_H__ */
