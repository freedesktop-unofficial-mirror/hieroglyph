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
	HG_TYPE_DICT      = 7, /* undocumented */
	HG_TYPE_OPERATOR  = 8, /* undocumented */
	HG_TYPE_ARRAY     = 9,
	HG_TYPE_MARK      = 10,
	HG_TYPE_FILE      = 11, /* undocumented */
	HG_TYPE_SAVE      = 12, /* undocumented */
	HG_TYPE_STACK     = 13, /* undocumented */
	HG_TYPE_END /* 15 */
};


G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_get          (hg_quark_t           v) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_set          (hg_quark_t           v) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_get_value    (hg_quark_t           v) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_mask_bits    (hg_quark_type_bit_t  begin,
							  hg_quark_type_bit_t  end) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_clear_bits   (hg_quark_t           v,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end) G_GNUC_CONST;
G_INLINE_FUNC gint       _hg_quark_type_bit_get_bits     (hg_quark_t           v,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_validate_bits(hg_quark_t           v,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end) G_GNUC_CONST;
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_set_bits     (hg_quark_t           x,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end,
                                                          hg_quark_t           v) G_GNUC_CONST;

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_get(hg_quark_t v)
{
	return v >> HG_QUARK_TYPE_BIT_SHIFT;
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_set(hg_quark_t v)
{
	return v << HG_QUARK_TYPE_BIT_SHIFT;
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_get_value(hg_quark_t v)
{
	return v & ((1LL << HG_QUARK_TYPE_BIT_SHIFT) - 1);
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_mask_bits(hg_quark_type_bit_t begin,
			     hg_quark_type_bit_t end)
{
	return ((1LL << (end - begin + 1)) - 1) << begin;
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_clear_bits(hg_quark_t        v,
			      hg_quark_type_bit_t begin,
			      hg_quark_type_bit_t end)
{
	return _hg_quark_type_bit_get(v) & ~(_hg_quark_type_bit_mask_bits(begin, end));
}

G_INLINE_FUNC gint
_hg_quark_type_bit_get_bits(hg_quark_t          v,
			    hg_quark_type_bit_t begin,
			    hg_quark_type_bit_t end)
{
	return (_hg_quark_type_bit_get(v) & _hg_quark_type_bit_mask_bits(begin, end)) >> begin;
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_validate_bits(hg_quark_t          v,
				 hg_quark_type_bit_t begin,
				 hg_quark_type_bit_t end)
{
	return v & _hg_quark_type_bit_mask_bits(begin, end);
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_set_bits(hg_quark_t          x,
			    hg_quark_type_bit_t begin,
			    hg_quark_type_bit_t end,
			    hg_quark_t          v)
{
	return _hg_quark_type_bit_set(_hg_quark_type_bit_clear_bits(x, begin, end) |
				      (_hg_quark_type_bit_validate_bits(v, begin, end) << begin)) |
		_hg_quark_type_bit_get_value(x);
}


G_INLINE_FUNC hg_quark_t hg_quark_new             (hg_type_t   type,
                                                   hg_quark_t  value);
G_INLINE_FUNC hg_type_t  hg_quark_get_type        (hg_quark_t  quark);
G_INLINE_FUNC hg_quark_t hg_quark_get_value       (hg_quark_t  quark);
G_INLINE_FUNC gboolean   hg_quark_is_simple_object(hg_quark_t  quark);

/**
 * hg_type_is_simple:
 * @type:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_type_is_simple(hg_type_t type)
{
	return type == HG_TYPE_NULL ||
		type == HG_TYPE_INT ||
		type == HG_TYPE_REAL ||
		type == HG_TYPE_BOOL ||
		type == HG_TYPE_MARK ||
		type == HG_TYPE_NAME;
}

/**
 * hg_quark_new:
 * @type:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC hg_quark_t
hg_quark_new(hg_type_t  type,
	     hg_quark_t value)
{
	return _hg_quark_type_bit_set_bits(_hg_quark_type_bit_get_value(value),
					   HG_QUARK_TYPE_BIT_TYPE,
					   HG_QUARK_TYPE_BIT_TYPE_END,
					   type);
}

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
 * hg_quark_get_value:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC hg_quark_t
hg_quark_get_value(hg_quark_t quark)
{
	return _hg_quark_type_bit_get_value(quark);
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

	return hg_type_is_simple(t);
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGQUARK_H__ */
