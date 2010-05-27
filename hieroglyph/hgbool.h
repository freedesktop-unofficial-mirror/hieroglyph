/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbool.h
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
#ifndef __HIEROGLYPH_HGBOOL_H__
#define __HIEROGLYPH_HGBOOL_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

typedef struct _hg_object_bool_t		hg_object_bool_t;

struct _hg_object_bool_t {
	hg_object_template_t t;
	guint16              unused1;
	guint32              value;
};


#define hg_object_bool_new(_m_,_v_,_r_)					\
	hg_object_new((_m_), (_r_), HG_TYPE_BOOL, 0, (_v_), Qnil)
#define hg_object_bool_to_qbool(_x_)				\
	(hg_quark_t)(hg_quark_mask_set_type (HG_TYPE_BOOL)	\
		     |hg_quark_mask_set_value ((_x_)->value))
#define hg_qbool_to_object_bool(_m_, _x_,_r_)	\
	hg_object_bool_new((_m_), (_x_), (_r_))


hg_object_vtable_t *hg_object_bool_get_vtable(void);


G_END_DECLS

#endif /* __HIEROGLYPH_HGBOOL_H__ */
