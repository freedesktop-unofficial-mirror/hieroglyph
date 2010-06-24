/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgreal.h
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
#ifndef __HIEROGLYPH_HGREAL_H__
#define __HIEROGLYPH_HGREAL_H__

#include <hieroglyph/hgquark.h>

G_BEGIN_DECLS

typedef struct _hg_bs_real_t	hg_bs_real_t;

struct _hg_bs_real_t {
	hg_bs_template_t t;
	guint16          representation;
	GFloatIEEE754    v;
};


#define HG_QREAL(_v_)				\
	hg_real_convert_from_native((_v_))

G_INLINE_FUNC hg_quark_t hg_real_convert_from_native(gfloat vfloat);

/**
 * hg_real_convert_from_native:
 * @vfloat:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC hg_quark_t
hg_real_convert_from_native(gfloat vfloat)
{
	GFloatIEEE754 v;
	gpointer x = &v;
	gchar *p = x;

	v.v_float = vfloat;
	return hg_quark_new(HG_TYPE_REAL, (hg_quark_t)*(guint32 *)p);
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGREAL_H__ */
