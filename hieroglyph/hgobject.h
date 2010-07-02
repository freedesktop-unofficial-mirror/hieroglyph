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
	static gsize    _hg_object_ ## _name_ ## _get_capsulated_size(void); \
	static gboolean _hg_object_ ## _name_ ## _initialize         (hg_object_t *object, \
								      va_list      args); \
	static void     _hg_object_ ## _name_ ## _free               (hg_object_t *object); \
									\
	static hg_object_vtable_t __hg_object_ ## _name_ ## _vtable = {	\
		.get_capsulated_size = _hg_object_ ## _name_ ## _get_capsulated_size, \
		.initialize          = _hg_object_ ## _name_ ## _initialize, \
		.free                = _hg_object_ ## _name_ ## _free,	\
	};								\
									\
	hg_object_vtable_t *						\
	hg_object_ ## _name_ ## _get_vtable(void)			\
	{								\
		return &__hg_object_ ## _name_ ## _vtable;		\
	}

typedef enum _hg_object_state_t			hg_object_state_t;
typedef struct _hg_object_vtable_t		hg_object_vtable_t;
typedef struct _hg_object_t			hg_object_t;


enum {
	HG_STATE_SHIFT_STATE       = 0,
	HG_STATE_SHIFT_READABLE    = 0,
	HG_STATE_SHIFT_WRITABLE    = 1,
	HG_STATE_SHIFT_EXECUTABLE  = 2,
	HG_STATE_SHIFT_EXECUTEONLY = 3,
	HG_STATE_SHIFT_ACCESSIBLE  = 4,
	HG_STATE_SHIFT_RESERVED5   = 5,
	HG_STATE_SHIFT_RESERVED6   = 6,
	HG_STATE_SHIFT_RESERVED7   = 7,
	HG_STATE_SHIFT_STATE_END   = 8,
	HG_STATE_SHIFT_PSEUDO      = 27,
	HG_STATE_SHIFT_TYPE        = 28,
	HG_STATE_SHIFT_TYPE_END    = 32,
};
enum _hg_object_state_t {
	HG_STATE_READABLE    = 1 << HG_STATE_SHIFT_READABLE,
	HG_STATE_WRITABLE    = 1 << HG_STATE_SHIFT_WRITABLE,
	HG_STATE_EXECUTABLE  = 1 << HG_STATE_SHIFT_EXECUTABLE,
	HG_STATE_EXECUTEONLY = 1 << HG_STATE_SHIFT_EXECUTEONLY,
	HG_STATE_ACCESSIBLE  = 1 << HG_STATE_SHIFT_ACCESSIBLE,
	HG_STATE_TYPE0       = 1 << HG_STATE_SHIFT_TYPE,	/* reserved area for object type */
	HG_STATE_TYPE1       = 1 << (HG_STATE_SHIFT_TYPE+1),	/* reserved area for object type */
	HG_STATE_TYPE2       = 1 << (HG_STATE_SHIFT_TYPE+2),	/* reserved area for object type */
	HG_STATE_TYPE3       = 1 << (HG_STATE_SHIFT_TYPE+3),	/* reserved area for object type */
};

struct _hg_object_vtable_t {
	gsize      (* get_capsulated_size) (void);
	gboolean   (* initialize)          (hg_object_t *object,
					    va_list      args);
	void       (* free)                (hg_object_t *object);
};
struct _hg_object_t {
	hg_mem_t *mem;
	hg_type_t type;
};


void       hg_object_init(void);
void       hg_object_tini(void);
hg_quark_t hg_object_new (hg_mem_t         *mem,
                          gpointer         *ret,
                          hg_type_t         type,
                          gsize             preallocated_size,
			  ...);
void       hg_object_free(hg_mem_t         *mem,
                          hg_quark_t        index);

G_END_DECLS

#endif /* __HIEROGLYPH_HGOBJECT_H__ */
