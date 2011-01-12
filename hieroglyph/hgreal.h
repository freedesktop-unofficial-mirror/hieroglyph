/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgreal.h
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
#include <hieroglyph/hgquark.h>

#ifndef __HIEROGLYPH_HGREAL_H__
#define __HIEROGLYPH_HGREAL_H__

#include <math.h>
#include <string.h>

G_BEGIN_DECLS

typedef struct _hg_bs_real_t	hg_bs_real_t;

struct _hg_bs_real_t {
	hg_bs_template_t t;
	guint16          representation;
	GFloatIEEE754    v;
};


#define HG_QREAL(_v_)				\
	hg_real_convert_from_native((_v_))
#define HG_REAL(_q_)				\
	hg_real_convert_to_native((_q_))
#define HG_IS_QREAL(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_REAL)
#define HG_REAL_EQUAL(_f1_,_f2_)		\
	(fabsf((_f1_) - (_f2_)) <= FLT_EPSILON)
#define HG_REAL_IS_ZERO(_f_)			\
	(fabsf((_f_)) <= FLT_EPSILON)
#define HG_REAL_GE(_f1_,_f2_)				\
	(HG_REAL_EQUAL (_f1_,_f2_) || _f1_ > _f2_)
#define HG_REAL_LE(_f1_,_f2_)				\
	(HG_REAL_EQUAL (_f1_,_f2_) || _f1_ < _f2_)

G_INLINE_FUNC hg_quark_t hg_real_convert_from_native(gdouble    vdouble);
G_INLINE_FUNC gdouble    hg_real_convert_to_native  (hg_quark_t qreal);

/**
 * hg_real_convert_from_native:
 * @vdouble:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC hg_quark_t
hg_real_convert_from_native(gdouble vdouble)
{
	GDoubleIEEE754 v;
	gpointer x = &v;
	gchar *p = x;

	v.v_double = vdouble;
	if (v.mpn.biased_exponent == 0 &&
	    (v.mpn.mantissa_low != 0 ||
	     v.mpn.mantissa_high != 0)) {
		/* ignore a subnormal number.
		 * it avoids to detect the real number right way
		 * since this is relying on something is there
		 * on the biased exponent bits. which isn't used
		 * for anything else on hg_quark_t.
		 */
		v.mpn.mantissa_low = 0;
		v.mpn.mantissa_high = 0;
	}

	return hg_quark_new(HG_TYPE_REAL, (hg_quark_t)*(guint64 *)p);
}

/**
 * hg_real_convert_to_native:
 * @qreal:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gdouble
hg_real_convert_to_native(hg_quark_t qreal)
{
	GDoubleIEEE754 v;
	gpointer x = &v;
	gint64 i;
	gchar *p;

	i = hg_quark_get_value(qreal);
	p = (gchar *)&i;
	memcpy(x, p, sizeof (gint64));

	return v.v_double;
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGREAL_H__ */
