/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-private.h
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
#ifndef __HIEROGLYPH_HGALLOCATOR_PRIVATE_H__
#define __HIEROGLYPH_HGALLOCATOR_PRIVATE_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgmessages.h>
#include <hieroglyph/hgquark.h>

HG_BEGIN_DECLS

typedef enum _hg_allocator_typebit_t		hg_allocator_typebit_t;
typedef struct _hg_allocator_bitmap_t		hg_allocator_bitmap_t;
typedef struct _hg_allocator_block_t		hg_allocator_block_t;
typedef struct _hg_allocator_private_t		hg_allocator_private_t;
typedef struct _hg_allocator_snapshot_private_t	hg_allocator_snapshot_private_t;

enum _hg_allocator_typebit_t {
	HG_ALLOC_TYPE_BIT_BIT0 = 0,
	HG_ALLOC_TYPE_BIT_INDEX = HG_ALLOC_TYPE_BIT_BIT0,
	HG_ALLOC_TYPE_BIT_INDEX00 = HG_ALLOC_TYPE_BIT_INDEX +  0,
	HG_ALLOC_TYPE_BIT_INDEX01 = HG_ALLOC_TYPE_BIT_INDEX +  1,
	HG_ALLOC_TYPE_BIT_INDEX02 = HG_ALLOC_TYPE_BIT_INDEX +  2,
	HG_ALLOC_TYPE_BIT_INDEX03 = HG_ALLOC_TYPE_BIT_INDEX +  3,
	HG_ALLOC_TYPE_BIT_INDEX04 = HG_ALLOC_TYPE_BIT_INDEX +  4,
	HG_ALLOC_TYPE_BIT_INDEX05 = HG_ALLOC_TYPE_BIT_INDEX +  5,
	HG_ALLOC_TYPE_BIT_INDEX06 = HG_ALLOC_TYPE_BIT_INDEX +  6,
	HG_ALLOC_TYPE_BIT_INDEX07 = HG_ALLOC_TYPE_BIT_INDEX +  7,
	HG_ALLOC_TYPE_BIT_INDEX08 = HG_ALLOC_TYPE_BIT_INDEX +  8,
	HG_ALLOC_TYPE_BIT_INDEX09 = HG_ALLOC_TYPE_BIT_INDEX +  9,
	HG_ALLOC_TYPE_BIT_INDEX10 = HG_ALLOC_TYPE_BIT_INDEX + 10,
	HG_ALLOC_TYPE_BIT_INDEX11 = HG_ALLOC_TYPE_BIT_INDEX + 11,
	HG_ALLOC_TYPE_BIT_INDEX12 = HG_ALLOC_TYPE_BIT_INDEX + 12,
	HG_ALLOC_TYPE_BIT_INDEX13 = HG_ALLOC_TYPE_BIT_INDEX + 13,
	HG_ALLOC_TYPE_BIT_INDEX14 = HG_ALLOC_TYPE_BIT_INDEX + 14,
	HG_ALLOC_TYPE_BIT_INDEX15 = HG_ALLOC_TYPE_BIT_INDEX + 15,
	HG_ALLOC_TYPE_BIT_INDEX16 = HG_ALLOC_TYPE_BIT_INDEX + 16,
	HG_ALLOC_TYPE_BIT_INDEX17 = HG_ALLOC_TYPE_BIT_INDEX + 17,
	HG_ALLOC_TYPE_BIT_INDEX18 = HG_ALLOC_TYPE_BIT_INDEX + 18,
	HG_ALLOC_TYPE_BIT_INDEX19 = HG_ALLOC_TYPE_BIT_INDEX + 19,
	HG_ALLOC_TYPE_BIT_INDEX20 = HG_ALLOC_TYPE_BIT_INDEX + 20,
	HG_ALLOC_TYPE_BIT_INDEX21 = HG_ALLOC_TYPE_BIT_INDEX + 21,
	HG_ALLOC_TYPE_BIT_INDEX22 = HG_ALLOC_TYPE_BIT_INDEX + 22,
	HG_ALLOC_TYPE_BIT_INDEX23 = HG_ALLOC_TYPE_BIT_INDEX + 23,
	HG_ALLOC_TYPE_BIT_INDEX_END = HG_ALLOC_TYPE_BIT_INDEX23,
	HG_ALLOC_TYPE_BIT_PAGE = HG_ALLOC_TYPE_BIT_INDEX_END + 1,
	HG_ALLOC_TYPE_BIT_PAGE0 = HG_ALLOC_TYPE_BIT_PAGE + 0,
	HG_ALLOC_TYPE_BIT_PAGE1 = HG_ALLOC_TYPE_BIT_PAGE + 1,
	HG_ALLOC_TYPE_BIT_PAGE2 = HG_ALLOC_TYPE_BIT_PAGE + 2,
	HG_ALLOC_TYPE_BIT_PAGE3 = HG_ALLOC_TYPE_BIT_PAGE + 3,
	HG_ALLOC_TYPE_BIT_PAGE4 = HG_ALLOC_TYPE_BIT_PAGE + 4,
	HG_ALLOC_TYPE_BIT_PAGE5 = HG_ALLOC_TYPE_BIT_PAGE + 5,
	HG_ALLOC_TYPE_BIT_PAGE6 = HG_ALLOC_TYPE_BIT_PAGE + 6,
	HG_ALLOC_TYPE_BIT_PAGE7 = HG_ALLOC_TYPE_BIT_PAGE + 7,
	HG_ALLOC_TYPE_BIT_PAGE_END = HG_ALLOC_TYPE_BIT_PAGE7,
	HG_ALLOC_TYPE_BIT_END
};

struct _hg_allocator_bitmap_t {
	hg_uint_t  **bitmaps;
	hg_usize_t  *size;
	hg_quark_t  *last_index;
	hg_uint_t    last_page;
};
struct _hg_allocator_block_t {
	hg_usize_t         size;
	volatile hg_uint_t lock_count;
	hg_int_t           age;
	hg_int_t           finalizer_id;
	hg_bool_t          is_restorable:1;
};
struct _hg_allocator_private_t {
	hg_allocator_data_t    parent;
	hg_allocator_data_t    slave;
	hg_allocator_bitmap_t *bitmap;
	hg_allocator_bitmap_t *slave_bitmap;
	hg_pointer_t          *heaps;
	hg_int_t               snapshot_age;
	hg_finalizer_func_t   *finalizer;
	hg_int_t               finalizer_count;
	hg_int_t               max_finalizer;
};
struct _hg_allocator_snapshot_private_t {
	hg_allocator_snapshot_data_t  parent;
	hg_allocator_bitmap_t        *bitmap;
	hg_pointer_t                 *heaps;
	hg_int_t                      age;
};


HG_INLINE_FUNC hg_quark_t _hg_allocator_typebit_get_mask   (hg_allocator_typebit_t begin,
							    hg_allocator_typebit_t end);
HG_INLINE_FUNC hg_quark_t _hg_allocator_typebit_round_value(hg_quark_t             q,
							    hg_allocator_typebit_t begin,
							    hg_allocator_typebit_t end);

HG_INLINE_FUNC hg_allocator_block_t  *_hg_allocator_get_block      (hg_pointer_t *p);
HG_INLINE_FUNC hg_quark_t             _hg_allocator_quark_build    (hg_int_t      page,
								    hg_uint_t     idx);
HG_INLINE_FUNC hg_uint_t              _hg_allocator_quark_get_page (hg_quark_t    qdata);
HG_INLINE_FUNC hg_uint_t              _hg_allocator_quark_get_index(hg_quark_t    qdata);


/** copied from hgquark.c **/
HG_INLINE_FUNC hg_quark_t
_hg_allocator_typebit_get_mask(hg_allocator_typebit_t begin,
			       hg_allocator_typebit_t end)
{
	hg_return_val_if_fail (begin <= end, Qnil, HG_e_VMerror);

	return (((1LL << (end - begin + 1)) - 1) << begin);
}

/** copied from hgquark.c **/
HG_INLINE_FUNC hg_quark_t
_hg_allocator_typebit_round_value(hg_quark_t             q,
				  hg_allocator_typebit_t begin,
				  hg_allocator_typebit_t end)
{
	return q & ((1LL << (end - begin + 1)) - 1);
}

HG_INLINE_FUNC hg_allocator_block_t *
_hg_allocator_get_block(hg_pointer_t *p)
{
	return (hg_allocator_block_t *)((hg_char_t *)(p) - HG_ALIGNED_TO_POINTER (sizeof (hg_allocator_block_t)));
}

HG_INLINE_FUNC hg_quark_t
_hg_allocator_quark_build(hg_int_t  page,
			  hg_uint_t idx)
{
	hg_quark_t retval;

	retval = (_hg_allocator_typebit_round_value(page,
						   HG_ALLOC_TYPE_BIT_PAGE,
						   HG_ALLOC_TYPE_BIT_PAGE_END) << HG_ALLOC_TYPE_BIT_PAGE) |
		(_hg_allocator_typebit_round_value(idx,
						   HG_ALLOC_TYPE_BIT_INDEX,
						   HG_ALLOC_TYPE_BIT_INDEX_END) << HG_ALLOC_TYPE_BIT_INDEX);

	return retval;
}

HG_INLINE_FUNC hg_uint_t
_hg_allocator_quark_get_page(hg_quark_t qdata)
{
	return (qdata & _hg_allocator_typebit_get_mask(HG_ALLOC_TYPE_BIT_PAGE,
						       HG_ALLOC_TYPE_BIT_PAGE_END)) >> HG_ALLOC_TYPE_BIT_PAGE;
}

HG_INLINE_FUNC hg_uint_t
_hg_allocator_quark_get_index(hg_quark_t qdata)
{
	return (qdata & _hg_allocator_typebit_get_mask(HG_ALLOC_TYPE_BIT_INDEX,
						       HG_ALLOC_TYPE_BIT_INDEX_END)) >> HG_ALLOC_TYPE_BIT_INDEX;
}

HG_END_DECLS

#endif /* __HIEROGLYPH_HGALLOCATOR_PRIVATE_H__ */
