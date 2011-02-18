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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGREAL_H__
#define __HIEROGLYPH_HGREAL_H__

#include <float.h>
#include <math.h>
#include <string.h>
#include <hieroglyph/hgquark.h>

HG_BEGIN_DECLS

/**
 * HG_QREAL:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_QREAL(_v_)				\
	(hg_real_convert_from_native((_v_)))
/**
 * HG_REAL:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_REAL(_q_)				\
	(hg_real_convert_to_native((_q_)))
/**
 * HG_IS_QREAL:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_IS_QREAL(_q_)				\
	(hg_quark_get_type(_q_) == HG_TYPE_REAL)
/**
 * HG_REAL_EQUAL:
 * @real1:
 * @real2:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_REAL_EQUAL(_f1_,_f2_)		\
	(fabs((_f1_) - (_f2_)) <= DBL_EPSILON)
/**
 * HG_REAL_IS_ZERO:
 * @real:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_REAL_IS_ZERO(_f_)			\
	(fabs((_f_)) <= DBL_EPSILON)
/**
 * HG_REAL_GE:
 * @real1:
 * @real2:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_REAL_GE(_f1_,_f2_)				\
	(HG_REAL_EQUAL (_f1_,_f2_) || _f1_ > _f2_)
/**
 * HG_REAL_LE:
 * @real1:
 * @real2:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_REAL_LE(_f1_,_f2_)				\
	(HG_REAL_EQUAL (_f1_,_f2_) || _f1_ < _f2_)


typedef union _hg_real_ieee754_t	hg_real_ieee754_t;

/*
 * IEEE Standard 754 Double Precision Storage Format:
 *
 *        63 62            52 51            32   31            0
 * +--------+----------------+----------------+ +---------------+
 * | s 1bit | e[62:52] 11bit | f[51:32] 20bit | | f[31:0] 32bit |
 * +--------+----------------+----------------+ +---------------+
 * B0--------------->B1---------->B2--->B3---->  B4->B5->B6->B7->
 */
union _hg_real_ieee754_t {
	hg_real_t v_real;
	struct {
		hg_uint_t mantissa_low:32;
		hg_uint_t mantissa_high:20;
		hg_uint_t biased_exponent:11;
		hg_uint_t sign:1;
	} mpn;
};

HG_INLINE_FUNC hg_quark_t hg_real_convert_from_native(hg_real_t  vreal);
HG_INLINE_FUNC hg_real_t  hg_real_convert_to_native  (hg_quark_t qreal);

/**
 * hg_real_convert_from_native:
 * @vdouble:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_quark_t
hg_real_convert_from_native(hg_real_t vreal)
{
	hg_real_ieee754_t v;
	hg_pointer_t x = &v;
	hg_char_t *p = x;

	v.v_real = vreal;
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

	return hg_quark_new(HG_TYPE_REAL, *(hg_quark_t *)p);
}

/**
 * hg_real_convert_to_native:
 * @qreal:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_real_t
hg_real_convert_to_native(hg_quark_t qreal)
{
	hg_real_ieee754_t v;
	hg_pointer_t x = &v;
	hg_quark_t i;
	hg_char_t *p;

	i = hg_quark_get_value(qreal);
	p = (hg_char_t *)&i;
	memcpy(x, p, sizeof (hg_quark_t));

	return v.v_real;
}

HG_END_DECLS

#endif /* __HIEROGLYPH_HGREAL_H__ */
