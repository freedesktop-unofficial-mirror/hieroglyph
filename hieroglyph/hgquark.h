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
#include <hieroglyph/hgerror.h>


G_BEGIN_DECLS

#define HG_QUARK_TYPE_BIT_SHIFT		32

typedef hg_quark_t (* hg_quark_iterate_func_t)	(hg_quark_t   qdata,
						 gpointer     user_data,
						 gpointer    *ret,
						 GError     **error);
typedef enum _hg_quark_type_bit_t	hg_quark_type_bit_t;
typedef enum _hg_type_t			hg_type_t;

enum _hg_quark_type_bit_t {
	HG_QUARK_TYPE_BIT_BIT0 = 0,
	HG_QUARK_TYPE_BIT_TYPE = HG_QUARK_TYPE_BIT_BIT0,		/* 0 */
	HG_QUARK_TYPE_BIT_TYPE0 = HG_QUARK_TYPE_BIT_TYPE + 0,		/* 0 */
	HG_QUARK_TYPE_BIT_TYPE1 = HG_QUARK_TYPE_BIT_TYPE + 1,		/* 1 */
	HG_QUARK_TYPE_BIT_TYPE2 = HG_QUARK_TYPE_BIT_TYPE + 2,		/* 2 */
	HG_QUARK_TYPE_BIT_TYPE3 = HG_QUARK_TYPE_BIT_TYPE + 3,		/* 3 */
	HG_QUARK_TYPE_BIT_TYPE_END = HG_QUARK_TYPE_BIT_TYPE3,		/* 3 */
	HG_QUARK_TYPE_BIT_ACCESS = HG_QUARK_TYPE_BIT_TYPE_END + 1,	/* 4 */
	HG_QUARK_TYPE_BIT_ACCESS0 = HG_QUARK_TYPE_BIT_ACCESS + 0,	/* 4 */
	HG_QUARK_TYPE_BIT_ACCESS1 = HG_QUARK_TYPE_BIT_ACCESS0 + 1,	/* 5 */
	HG_QUARK_TYPE_BIT_ACCESS2 = HG_QUARK_TYPE_BIT_ACCESS0 + 2,	/* 6 */
	HG_QUARK_TYPE_BIT_ACCESS_END = HG_QUARK_TYPE_BIT_ACCESS2,	/* 6 */
	HG_QUARK_TYPE_BIT_EXECUTABLE = HG_QUARK_TYPE_BIT_ACCESS0,	/* 4 */
	HG_QUARK_TYPE_BIT_READABLE = HG_QUARK_TYPE_BIT_ACCESS1,		/* 5 */
	HG_QUARK_TYPE_BIT_WRITABLE = HG_QUARK_TYPE_BIT_ACCESS2,		/* 6 */
	HG_QUARK_TYPE_BIT_MEM_ID = HG_QUARK_TYPE_BIT_ACCESS_END + 1,	/* 7 */
	HG_QUARK_TYPE_BIT_MEM_ID0 = HG_QUARK_TYPE_BIT_MEM_ID + 0,	/* 7 */
	HG_QUARK_TYPE_BIT_MEM_ID1 = HG_QUARK_TYPE_BIT_MEM_ID + 1,	/* 8 */
	HG_QUARK_TYPE_BIT_MEM_ID_END = HG_QUARK_TYPE_BIT_MEM_ID1,	/* 8 */
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
	HG_TYPE_OPER      = 8, /* undocumented */
	HG_TYPE_ARRAY     = 9,
	HG_TYPE_MARK      = 10,
	HG_TYPE_FILE      = 11, /* undocumented */
	HG_TYPE_SAVE      = 12, /* undocumented */
	HG_TYPE_STACK     = 13, /* undocumented */
	HG_TYPE_END /* 15 */
};

G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_get          (hg_quark_t           v);
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_shift        (hg_quark_t           v);
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_get_value    (hg_quark_t           v);
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_mask_bits    (hg_quark_type_bit_t  begin,
							  hg_quark_type_bit_t  end);
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_clear_bits   (hg_quark_t           v,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end);
G_INLINE_FUNC gint       _hg_quark_type_bit_get_bits     (hg_quark_t           v,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end);
G_INLINE_FUNC hg_quark_t _hg_quark_type_bit_validate_bits(hg_quark_t           v,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end);
G_INLINE_FUNC void       _hg_quark_type_bit_set_bits     (hg_quark_t          *x,
                                                          hg_quark_type_bit_t  begin,
                                                          hg_quark_type_bit_t  end,
                                                          hg_quark_t           v);

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_get(hg_quark_t v)
{
	return (v >> HG_QUARK_TYPE_BIT_SHIFT);
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_shift(hg_quark_t v)
{
	return (v << HG_QUARK_TYPE_BIT_SHIFT);
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_get_value(hg_quark_t v)
{
	return (v & (_hg_quark_type_bit_shift(1LL) - 1));
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_mask_bits(hg_quark_type_bit_t begin,
			     hg_quark_type_bit_t end)
{
	hg_return_val_if_fail (begin <= end, Qnil);

	return (((1LL << (end - begin + 1)) - 1) << begin);
}

G_INLINE_FUNC hg_quark_t
_hg_quark_type_bit_clear_bits(hg_quark_t          v,
			      hg_quark_type_bit_t begin,
			      hg_quark_type_bit_t end)
{
	return (_hg_quark_type_bit_get(v) & ~(_hg_quark_type_bit_mask_bits(begin, end)));
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
	return v & (_hg_quark_type_bit_mask_bits(begin, end) >> begin);
}

G_INLINE_FUNC void
_hg_quark_type_bit_set_bits(hg_quark_t          *x,
			    hg_quark_type_bit_t  begin,
			    hg_quark_type_bit_t  end,
			    hg_quark_t           v)
{
	hg_quark_t _x_ = *x;

	*x = _hg_quark_type_bit_shift(_hg_quark_type_bit_clear_bits(_x_, begin, end) |
				      (_hg_quark_type_bit_validate_bits(v, begin, end) << begin)) |
		_hg_quark_type_bit_get_value(_x_);
}


G_INLINE_FUNC hg_quark_t   hg_quark_new             (hg_type_t   type,
                                                     hg_quark_t  value);
G_INLINE_FUNC hg_type_t    hg_quark_get_type        (hg_quark_t  quark);
G_INLINE_FUNC hg_quark_t   hg_quark_get_value       (hg_quark_t  quark);
G_INLINE_FUNC gboolean     hg_quark_is_simple_object(hg_quark_t  quark);
G_INLINE_FUNC void         hg_quark_set_executable  (hg_quark_t *quark,
                                                     gboolean    flag);
G_INLINE_FUNC gboolean     hg_quark_is_executable   (hg_quark_t  quark);
G_INLINE_FUNC void         hg_quark_set_readable    (hg_quark_t *quark,
                                                     gboolean    flag);
G_INLINE_FUNC gboolean     hg_quark_is_readable     (hg_quark_t  quark);
G_INLINE_FUNC void         hg_quark_set_writable    (hg_quark_t *quark,
                                                     gboolean    flag);
G_INLINE_FUNC gboolean     hg_quark_is_writable     (hg_quark_t  quark);
G_INLINE_FUNC void         hg_quark_set_access_bits (hg_quark_t *quark,
						     gboolean    readable,
						     gboolean    writable,
						     gboolean    executable);
G_INLINE_FUNC gboolean     hg_quark_has_same_mem_id (hg_quark_t  quark,
                                                     guint       id);
G_INLINE_FUNC void         hg_quark_set_mem_id      (hg_quark_t *quark,
                                                     guint       id);
G_INLINE_FUNC hg_quark_t   hg_quark_get_hash        (hg_quark_t  quark);
G_INLINE_FUNC const gchar *hg_quark_get_type_name   (hg_quark_t  qdata);

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
		type == HG_TYPE_NAME ||
		type == HG_TYPE_EVAL_NAME;
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
	hg_quark_t retval = value;

	_hg_quark_type_bit_set_bits(&retval,
				    HG_QUARK_TYPE_BIT_TYPE,
				    HG_QUARK_TYPE_BIT_TYPE_END,
				    type);

	return retval;
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

/**
 * hg_quark_set_executable:
 * @quark:
 * @flag:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_executable(hg_quark_t *quark,
			gboolean    flag)
{
	hg_return_if_fail (quark != NULL);

	_hg_quark_type_bit_set_bits(quark,
				    HG_QUARK_TYPE_BIT_EXECUTABLE,
				    HG_QUARK_TYPE_BIT_EXECUTABLE,
				    (flag == TRUE));
}

/**
 * hg_quark_is_executable:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_is_executable(hg_quark_t quark)
{
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_EXECUTABLE,
					   HG_QUARK_TYPE_BIT_EXECUTABLE) != 0;
}

/**
 * hg_quark_set_readable:
 * quark:
 * flag:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_readable(hg_quark_t *quark,
		      gboolean    flag)
{
	hg_return_if_fail (quark != NULL);

	_hg_quark_type_bit_set_bits(quark,
				    HG_QUARK_TYPE_BIT_READABLE,
				    HG_QUARK_TYPE_BIT_READABLE,
				    (flag == TRUE));
}

/**
 * hg_quark_is_readable:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_is_readable(hg_quark_t quark)
{
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_READABLE,
					   HG_QUARK_TYPE_BIT_READABLE) != 0;
}

/**
 * hg_quark_set_writable:
 * quark:
 * flag:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_writable(hg_quark_t *quark,
		      gboolean    flag)
{
	hg_return_if_fail (quark != NULL);

	_hg_quark_type_bit_set_bits(quark,
				    HG_QUARK_TYPE_BIT_WRITABLE,
				    HG_QUARK_TYPE_BIT_WRITABLE,
				    (flag == TRUE));
}

/**
 * hg_quark_is_writable:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_is_writable(hg_quark_t quark)
{
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_WRITABLE,
					   HG_QUARK_TYPE_BIT_WRITABLE) != 0;
}

/**
 * hg_quark_set_access_bits:
 * @quark:
 * @readable:
 * @writable:
 * @executable:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_access_bits(hg_quark_t *quark,
			 gboolean    readable,
			 gboolean    writable,
			 gboolean    executable)
{
	hg_return_if_fail (quark != NULL);

	_hg_quark_type_bit_set_bits(quark,
				    HG_QUARK_TYPE_BIT_ACCESS,
				    HG_QUARK_TYPE_BIT_ACCESS_END,
				    ((readable << HG_QUARK_TYPE_BIT_READABLE) |
				     (writable << HG_QUARK_TYPE_BIT_WRITABLE) |
				     (executable << HG_QUARK_TYPE_BIT_EXECUTABLE)) >> HG_QUARK_TYPE_BIT_ACCESS);
}

/**
 * hg_quark_has_same_mem_id:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_has_same_mem_id(hg_quark_t quark,
			 guint      id)
{
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_MEM_ID,
					   HG_QUARK_TYPE_BIT_MEM_ID_END) == id;
}

/**
 * hg_quark_set_mem_id:
 * @quark:
 * @id:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_mem_id(hg_quark_t *quark,
		    guint       id)
{
	_hg_quark_type_bit_set_bits(quark,
				    HG_QUARK_TYPE_BIT_MEM_ID,
				    HG_QUARK_TYPE_BIT_MEM_ID_END,
				    id);
}

/**
 * hg_quark_get_hash:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC hg_quark_t
hg_quark_get_hash(hg_quark_t quark)
{
	return _hg_quark_type_bit_shift(_hg_quark_type_bit_clear_bits(quark,
								      HG_QUARK_TYPE_BIT_ACCESS,
								      HG_QUARK_TYPE_BIT_ACCESS_END)) |
		_hg_quark_type_bit_get_value(quark);
}

/**
 * hg_quark_get_type_name:
 * @qdata:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC const gchar *
hg_quark_get_type_name(hg_quark_t  qdata)
{
	static const gchar const *types[] = {
		"nulltype",
		"integertype",
		"realtype",
		"nametype",
		"booleantype",
		"stringtype",
		"enametype",
		"dicttype",
		"operatortype",
		"arraytype",
		"marktype",
		"filetype",
		"savetype",
		"stacktype",
		NULL
	};
	static const gchar const *unknown = "unknowntype";

	hg_return_val_if_fail (hg_quark_get_type(qdata) < HG_TYPE_END, unknown);

	return types[hg_quark_get_type(qdata)];
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGQUARK_H__ */
