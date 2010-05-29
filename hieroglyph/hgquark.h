/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgquark.h
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
#ifndef __HIEROGLYPH_HGQUARK_H__
#define __HIEROGLYPH_HGQUARK_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


#define HG_QUARK_TYPE_BIT_SHIFT		32

typedef enum _hg_quark_type_bit_t	hg_quark_type_bit_t;
typedef enum _hg_type_t			hg_type_t;
typedef gint64				hg_quark_t;

enum _hg_quark_type_bit_t {
	HG_QUARK_TYPE_BIT_TYPE = 0,
	HG_QUARK_TYPE_BIT_TYPE0 = 0,
	HG_QUARK_TYPE_BIT_TYPE1 = 1,
	HG_QUARK_TYPE_BIT_TYPE2 = 2,
	HG_QUARK_TYPE_BIT_TYPE3 = 3,
	HG_QUARK_TYPE_BIT_TYPE_END = 3,
	HG_QUARK_TYPE_BIT_OBJECT_TYPE = 4, /* 0 .. simple object, 1 .. composite object */
};
enum _hg_type_t {
	HG_TYPE_NULL      = 0,
	HG_TYPE_INT       = 1,
	HG_TYPE_REAL      = 2,
	HG_TYPE_NAME      = 3,
	HG_TYPE_BOOL      = 4,
	HG_TYPE_STRING    = 5,
	HG_TYPE_EVAL_NAME = 6,
	HG_TYPE_ARRAY     = 9,
	HG_TYPE_MARK      = 10,
	HG_TYPE_END
};


#ifdef G_CAN_INLINE
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_mask     (hg_quark_t           v) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_mask_bits(hg_quark_t           v,
						      hg_quark_type_bit_t  begin,
						      hg_quark_type_bit_t  end) G_GNUC_CONST;
G_INLINE_FUNC gint       _hg_quark_type_bit_get_bits (hg_quark_t           v,
						      hg_quark_type_bit_t  begin,
						      hg_quark_type_bit_t  end) G_GNUC_CONST;

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_mask(hg_quark_t v)
{
	return v >> HG_QUARK_TYPE_BIT_SHIFT;
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_mask_bits(hg_quark_t          v,
			     hg_quark_type_bit_t begin,
			     hg_quark_type_bit_t end)
{
	return _hg_quark_type_bit_mask(v) & ((1 << (end - begin + 1)) - 1);
}

G_INLINE_FUNC gint
_hg_quark_type_bit_get_bits(hg_quark_t          v,
			    hg_quark_type_bit_t begin,
			    hg_quark_type_bit_t end)
{
	return _hg_quark_type_bit_mask_bits(v, begin, end) >> begin;
}
#else /* !G_CAN_INLINE */
#define _hg_quark_type_bit_mask(_q_)			\
	((hg_quark_t)(_q_) >> HG_QUARK_TYPE_BIT_SHIFT)
#define _hg_quark_type_bit_mask_bits(_q_,_bs_,_be_)			\
	(_hg_quark_type_bit_mask(_q_) & (1 << (((_be_) - (_bs_) + 1) - 1)))
#define _hg_quark_type_bit_get_bits(_q_,_bs_,_be_)		\
	(_hg_quark_type_bit_mask_bits(_q_,_bs_,_be_) >> (_bs_)
#endif /* G_CAN_INLINE */


G_INLINE_FUNC hg_type_t hg_quark_get_type        (hg_quark_t quark);
G_INLINE_FUNC gboolean  hg_quark_is_simple_object(hg_quark_t quark);

/**
 * hg_quark_get_type:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC hg_type_t
hg_quark_get_type(hg_quark_t quark)
{
	return (hg_type_t)_hg_quark_type_bit_get_bits(quark,
						      HG_QUARK_TYPE_BIT_TYPE,
						      HG_QUARK_TYPE_BIT_TYPE_END);
}

/**
 * hg_quark_is_simple_object:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_is_simple_object(hg_quark_t quark)
{
	hg_type_t t = hg_quark_get_type(quark);

	return t == HG_TYPE_NULL ||
		t == HG_TYPE_INT ||
		t == HG_TYPE_REAL ||
		t == HG_TYPE_BOOL ||
		t == HG_TYPE_MARK ||
		t == HG_TYPE_NAME;
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGQUARK_H__ */
