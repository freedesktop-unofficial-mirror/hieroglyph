/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgname.h
 * Copyright (C) 2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGNAME_H__
#define __HIEROGLYPH_HGNAME_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

typedef struct _hg_object_name_t	hg_object_name_t;
typedef enum _hg_object_name_type_t	hg_object_name_type_t;

enum _hg_object_name_type_t {
	HG_name_offset = 1,
	HG_name_DPS = 0,
	HG_name_index = -1,
};
struct _hg_object_name_t {
	hg_object_template_t t;
	guint16              representation;
	union {
		guint32      index;
		guint32      offset;
	} v;
};


#define hg_object_name_to_qname(_x_)				\
	(hg_quark_t)(hg_quark_mask_set_type (HG_TYPE_NAME)	\
		     |hg_quark_mask_set_value ((_x_)->value))
#define hg_qname_to_object_name(_m_, _x_)					\
	(hg_object_name_t *)hg_object_new(_m_, HG_TYPE_NAME, 0, (_x_), Qnil)


hg_object_vtable_t *hg_object_name_get_vtable       (void);
hg_quark_t          hg_object_name_new_with_encoding(hg_mem_t             *mem,
                                                     hg_system_encoding_t  encoding,
                                                     gpointer             *ret);
hg_quark_t          hg_object_name_new_with_string  (hg_mem_t             *mem,
                                                     const gchar          *name,
                                                     gssize                len,
                                                     gpointer             *ret);
const gchar        *hg_object_name_get_name         (hg_mem_t             *mem,
                                                     hg_quark_t            index);


G_END_DECLS

#endif /* __HIEROGLYPH_HGNAME_H__ */
