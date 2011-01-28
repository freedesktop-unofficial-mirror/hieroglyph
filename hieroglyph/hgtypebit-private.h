/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgtypebit-private.h
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
#ifndef __HIEROGLYPH_HGTYPEBIT_H__
#define __HIEROGLYPH_HGTYPEBIT_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgerror.h>

HG_BEGIN_DECLS

#define HG_TYPEBIT_SHIFT	32

typedef enum _hg_typebit_t	hg_typebit_t;

/**
 * hg_typebit_t:
 *
 * FIXME
 */
enum _hg_typebit_t {
	HG_TYPEBIT_BIT0 = 0,
	HG_TYPEBIT_TYPE = HG_TYPEBIT_BIT0,		/*  0 */
	HG_TYPEBIT_TYPE0 = HG_TYPEBIT_TYPE + 0,		/*  0 */
	HG_TYPEBIT_TYPE1 = HG_TYPEBIT_TYPE + 1,		/*  1 */
	HG_TYPEBIT_TYPE2 = HG_TYPEBIT_TYPE + 2,		/*  2 */
	HG_TYPEBIT_TYPE3 = HG_TYPEBIT_TYPE + 3,		/*  3 */
	HG_TYPEBIT_TYPE4 = HG_TYPEBIT_TYPE + 4,		/*  4 */
	HG_TYPEBIT_TYPE_END = HG_TYPEBIT_TYPE4,		/*  4 */
	HG_TYPEBIT_ACCESS = HG_TYPEBIT_TYPE_END + 1,	/*  5 */
	HG_TYPEBIT_ACCESS0 = HG_TYPEBIT_ACCESS + 0,	/*  5 */
	HG_TYPEBIT_ACCESS1 = HG_TYPEBIT_ACCESS0 + 1,	/*  6 */
	HG_TYPEBIT_ACCESS2 = HG_TYPEBIT_ACCESS0 + 2,	/*  7 */
	HG_TYPEBIT_ACCESS3 = HG_TYPEBIT_ACCESS0 + 3,	/*  8 */
	HG_TYPEBIT_ACCESS_END = HG_TYPEBIT_ACCESS3,	/*  8 */
	HG_TYPEBIT_EXECUTABLE = HG_TYPEBIT_ACCESS0,	/*  5 */
	HG_TYPEBIT_READABLE = HG_TYPEBIT_ACCESS1,	/*  6 */
	HG_TYPEBIT_WRITABLE = HG_TYPEBIT_ACCESS2,	/*  7 */
	HG_TYPEBIT_ACCESSIBLE = HG_TYPEBIT_ACCESS3,	/*  8 */
	HG_TYPEBIT_MEM_ID = HG_TYPEBIT_ACCESS_END + 1,	/*  9 */
	HG_TYPEBIT_MEM_ID0 = HG_TYPEBIT_MEM_ID + 0,	/*  9 */
	HG_TYPEBIT_MEM_ID1 = HG_TYPEBIT_MEM_ID + 1,	/* 10 */
	HG_TYPEBIT_MEM_ID_END = HG_TYPEBIT_MEM_ID1,	/* 10 */
	HG_TYPEBIT_END /* 31 */
};

G_INLINE_FUNC hg_quark_t _hg_typebit_shift      (hg_typebit_t  typebit);
G_INLINE_FUNC hg_quark_t _hg_typebit_unshift    (hg_quark_t    q);
G_INLINE_FUNC hg_quark_t _hg_typebit_get_mask   (hg_typebit_t  begin,
						 hg_typebit_t  end);
G_INLINE_FUNC hg_quark_t _hg_typebit_clear_range(hg_quark_t    q,
						 hg_typebit_t  begin,
						 hg_typebit_t  end);
G_INLINE_FUNC hg_quark_t _hg_typebit_round_value(hg_quark_t    q,
						 hg_typebit_t  begin,
						 hg_typebit_t  end);
G_INLINE_FUNC void       _hg_typebit_set_value  (hg_quark_t   *q,
						 hg_typebit_t  begin,
						 hg_typebit_t  end,
						 hg_quark_t    v);
G_INLINE_FUNC hg_quark_t _hg_typebit_get_value_ (hg_quark_t    v,
						 hg_typebit_t  begin,
						 hg_typebit_t  end);
G_INLINE_FUNC hg_quark_t _hg_typebit_get_value  (hg_quark_t    v,
						 hg_typebit_t  begin,
						 hg_typebit_t  end);


G_INLINE_FUNC hg_quark_t
_hg_typebit_shift(hg_typebit_t typebit)
{
	return 1LL << (typebit + HG_TYPEBIT_SHIFT);
}

G_INLINE_FUNC hg_quark_t
_hg_typebit_unshift(hg_quark_t q)
{
	return q >> HG_TYPEBIT_SHIFT;
}

G_INLINE_FUNC hg_quark_t
_hg_typebit_get_mask(hg_typebit_t begin,
		     hg_typebit_t end)
{
	hg_quark_t b, e;

	hg_return_val_if_fail (begin <= end, Qnil);

	b = begin + HG_TYPEBIT_SHIFT;
	e = end + HG_TYPEBIT_SHIFT;

	return (((1LL << (e - b + 1)) - 1) << b);
}

G_INLINE_FUNC hg_quark_t
_hg_typebit_clear_range(hg_quark_t   q,
			hg_typebit_t begin,
			hg_typebit_t end)
{
	return q & ~(_hg_typebit_get_mask(begin, end));
}

G_INLINE_FUNC hg_quark_t
_hg_typebit_round_value(hg_quark_t   q,
			hg_typebit_t begin,
			hg_typebit_t end)
{
	return q & ((1LL << (end - begin + 1)) - 1);
}

G_INLINE_FUNC void
_hg_typebit_set_value(hg_quark_t   *q,
		      hg_typebit_t  begin,
		      hg_typebit_t  end,
		      hg_quark_t    v)
{
	hg_quark_t r = *q;

	*q = _hg_typebit_clear_range(r, begin, end) |
		(_hg_typebit_round_value(v, begin, end) << (begin + HG_TYPEBIT_SHIFT));
}

G_INLINE_FUNC hg_quark_t
_hg_typebit_get_value_(hg_quark_t   v,
		       hg_typebit_t begin,
		       hg_typebit_t end)
{
	return (v & _hg_typebit_get_mask(begin, end)) >> begin;
}

G_INLINE_FUNC hg_quark_t
_hg_typebit_get_value(hg_quark_t   v,
		      hg_typebit_t begin,
		      hg_typebit_t end)
{
	return _hg_typebit_unshift(_hg_typebit_get_value_(v, begin, end));
}

HG_END_DECLS

#endif /* __HIEROGLYPH_HGTYPEBIT_H__ */
