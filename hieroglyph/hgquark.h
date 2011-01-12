/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgquark.h
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
typedef gboolean   (* hg_quark_compare_func_t)  (hg_quark_t   q1,
						 hg_quark_t   q2,
						 gpointer     user_data);
typedef enum _hg_quark_type_bit_t	hg_quark_type_bit_t;
typedef enum _hg_type_t			hg_type_t;

enum _hg_quark_type_bit_t {
	HG_QUARK_TYPE_BIT_BIT0 = 0,
	HG_QUARK_TYPE_BIT_TYPE = HG_QUARK_TYPE_BIT_BIT0,		/*  0 */
	HG_QUARK_TYPE_BIT_TYPE0 = HG_QUARK_TYPE_BIT_TYPE + 0,		/*  0 */
	HG_QUARK_TYPE_BIT_TYPE1 = HG_QUARK_TYPE_BIT_TYPE + 1,		/*  1 */
	HG_QUARK_TYPE_BIT_TYPE2 = HG_QUARK_TYPE_BIT_TYPE + 2,		/*  2 */
	HG_QUARK_TYPE_BIT_TYPE3 = HG_QUARK_TYPE_BIT_TYPE + 3,		/*  3 */
	HG_QUARK_TYPE_BIT_TYPE4 = HG_QUARK_TYPE_BIT_TYPE + 4,		/*  4 */
	HG_QUARK_TYPE_BIT_TYPE_END = HG_QUARK_TYPE_BIT_TYPE4,		/*  4 */
	HG_QUARK_TYPE_BIT_ACCESS = HG_QUARK_TYPE_BIT_TYPE_END + 1,	/*  5 */
	HG_QUARK_TYPE_BIT_ACCESS0 = HG_QUARK_TYPE_BIT_ACCESS + 0,	/*  5 */
	HG_QUARK_TYPE_BIT_ACCESS1 = HG_QUARK_TYPE_BIT_ACCESS0 + 1,	/*  6 */
	HG_QUARK_TYPE_BIT_ACCESS2 = HG_QUARK_TYPE_BIT_ACCESS0 + 2,	/*  7 */
	HG_QUARK_TYPE_BIT_ACCESS3 = HG_QUARK_TYPE_BIT_ACCESS0 + 3,	/*  8 */
	HG_QUARK_TYPE_BIT_ACCESS_END = HG_QUARK_TYPE_BIT_ACCESS3,	/*  8 */
	HG_QUARK_TYPE_BIT_EXECUTABLE = HG_QUARK_TYPE_BIT_ACCESS0,	/*  5 */
	HG_QUARK_TYPE_BIT_READABLE = HG_QUARK_TYPE_BIT_ACCESS1,		/*  6 */
	HG_QUARK_TYPE_BIT_WRITABLE = HG_QUARK_TYPE_BIT_ACCESS2,		/*  7 */
	HG_QUARK_TYPE_BIT_EDITABLE = HG_QUARK_TYPE_BIT_ACCESS3,		/*  8 */
	HG_QUARK_TYPE_BIT_MEM_ID = HG_QUARK_TYPE_BIT_ACCESS_END + 1,	/*  9 */
	HG_QUARK_TYPE_BIT_MEM_ID0 = HG_QUARK_TYPE_BIT_MEM_ID + 0,	/*  9 */
	HG_QUARK_TYPE_BIT_MEM_ID1 = HG_QUARK_TYPE_BIT_MEM_ID + 1,	/* 10 */
	HG_QUARK_TYPE_BIT_MEM_ID_END = HG_QUARK_TYPE_BIT_MEM_ID1,	/* 10 */
	HG_QUARK_TYPE_BIT_END /* 31 */
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
	HG_TYPE_SNAPSHOT  = 12, /* undocumented */
	HG_TYPE_STACK     = 13, /* undocumented */
	HG_TYPE_DICT_NODE = 14, /* undocumented */
	HG_TYPE_PATH      = 15, /* undocumented */
	HG_TYPE_GSTATE    = 16, /* undocumented */
	HG_TYPE_END /* 31 */
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


G_INLINE_FUNC hg_quark_t   hg_quark_new             (hg_type_t                type,
                                                     hg_quark_t               value);
G_INLINE_FUNC hg_type_t    hg_quark_get_type        (hg_quark_t               quark);
G_INLINE_FUNC hg_quark_t   hg_quark_get_value       (hg_quark_t               quark);
G_INLINE_FUNC gboolean     hg_quark_is_simple_object(hg_quark_t               quark);
G_INLINE_FUNC void         hg_quark_set_executable  (hg_quark_t              *quark,
                                                     gboolean                 flag);
G_INLINE_FUNC gboolean     hg_quark_is_executable   (hg_quark_t               quark);
G_INLINE_FUNC void         hg_quark_set_readable    (hg_quark_t              *quark,
                                                     gboolean                 flag);
G_INLINE_FUNC gboolean     hg_quark_is_readable     (hg_quark_t               quark);
G_INLINE_FUNC void         hg_quark_set_writable    (hg_quark_t              *quark,
                                                     gboolean                 flag);
G_INLINE_FUNC gboolean     hg_quark_is_writable     (hg_quark_t               quark);
G_INLINE_FUNC void         hg_quark_set_editable    (hg_quark_t              *quark,
						     gboolean                 flag);
G_INLINE_FUNC gboolean     hg_quark_is_editable     (hg_quark_t               quark);
G_INLINE_FUNC void         hg_quark_set_access_bits (hg_quark_t              *quark,
                                                     gboolean                 readable,
                                                     gboolean                 writable,
                                                     gboolean                 executable,
						     gboolean                 editable);
G_INLINE_FUNC gboolean     hg_quark_has_same_mem_id (hg_quark_t               quark,
                                                     guint                    id);
G_INLINE_FUNC gint         hg_quark_get_mem_id      (hg_quark_t               quark);
G_INLINE_FUNC void         hg_quark_set_mem_id      (hg_quark_t              *quark,
                                                     guint                    id);
G_INLINE_FUNC hg_quark_t   hg_quark_get_hash        (hg_quark_t               quark);
G_INLINE_FUNC const gchar *hg_quark_get_type_name   (hg_quark_t               qdata);
G_INLINE_FUNC gboolean     hg_quark_compare         (hg_quark_t               qdata1,
                                                     hg_quark_t               qdata2,
                                                     hg_quark_compare_func_t  func,
						     gpointer                 user_data);

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

	if (type != HG_TYPE_REAL) {
		_hg_quark_type_bit_set_bits(&retval,
					    HG_QUARK_TYPE_BIT_TYPE,
					    HG_QUARK_TYPE_BIT_TYPE_END,
					    type);
		hg_quark_set_access_bits(&retval,
					 TRUE,
					 (hg_type_is_simple(type) ||
					  type == HG_TYPE_OPER) ? FALSE : TRUE,
					 type == HG_TYPE_EVAL_NAME,
					 TRUE);
	}

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
	if (quark == 0 ||
	    _hg_quark_type_bit_clear_bits(quark,
					  HG_QUARK_TYPE_BIT_BIT0,
					  HG_QUARK_TYPE_BIT_END))
		return HG_TYPE_REAL;
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
	if (hg_quark_get_type(quark) == HG_TYPE_REAL)
		return quark;
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

	if (*quark != Qnil &&
	    hg_quark_get_type(*quark) != HG_TYPE_REAL) {
		if (hg_quark_is_editable(*quark)) {
			_hg_quark_type_bit_set_bits(quark,
						    HG_QUARK_TYPE_BIT_EXECUTABLE,
						    HG_QUARK_TYPE_BIT_EXECUTABLE,
						    (flag == TRUE));
		} else {
#ifdef HG_DEBUG
			g_warning("%s: Access disallowed for modification",
				  __PRETTY_FUNCTION__);
#endif
		}
	}
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
	if (hg_quark_get_type(quark) == HG_TYPE_REAL) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return FALSE;
	}
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

	if (*quark != Qnil &&
	    hg_quark_get_type(*quark) != HG_TYPE_REAL) {
		if (hg_quark_is_editable(*quark)) {
			_hg_quark_type_bit_set_bits(quark,
						    HG_QUARK_TYPE_BIT_READABLE,
						    HG_QUARK_TYPE_BIT_READABLE,
						    (flag == TRUE));
		} else {
#ifdef HG_DEBUG
			g_warning("%s: Access disallowed for modification",
				  __PRETTY_FUNCTION__);
#endif
		}
	}
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
	if (hg_quark_get_type(quark) == HG_TYPE_REAL) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return TRUE;
	}
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

	if (*quark != Qnil &&
	    hg_quark_get_type(*quark) != HG_TYPE_REAL) {
		if (hg_quark_is_editable(*quark)) {
			_hg_quark_type_bit_set_bits(quark,
						    HG_QUARK_TYPE_BIT_WRITABLE,
						    HG_QUARK_TYPE_BIT_WRITABLE,
						    (flag == TRUE));
		} else {
#ifdef HG_DEBUG
			g_warning("%s: Access disallowed for modification",
				  __PRETTY_FUNCTION__);
#endif
		}
	}
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
	if (hg_quark_get_type(quark) == HG_TYPE_REAL) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return FALSE;
	}
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_WRITABLE,
					   HG_QUARK_TYPE_BIT_WRITABLE) != 0 &&
		hg_quark_is_editable(quark);
}

/**
 * hg_quark_set_editable:
 * @quark:
 * @flag:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_editable(hg_quark_t *quark,
		      gboolean    flag)
{
	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    hg_quark_get_type(*quark) != HG_TYPE_REAL) {
		_hg_quark_type_bit_set_bits(quark,
					    HG_QUARK_TYPE_BIT_EDITABLE,
					    HG_QUARK_TYPE_BIT_EDITABLE,
					    (flag == TRUE));
	}
}

/**
 * hg_quark_is_editable:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_is_editable(hg_quark_t quark)
{
	if (hg_quark_get_type(quark) == HG_TYPE_REAL) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return TRUE;
	}
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_EDITABLE,
					   HG_QUARK_TYPE_BIT_EDITABLE) != 0;
}

/**
 * hg_quark_set_access_bits:
 * @quark:
 * @readable:
 * @writable:
 * @executable:
 * @editable:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_quark_set_access_bits(hg_quark_t *quark,
			 gboolean    readable,
			 gboolean    writable,
			 gboolean    executable,
			 gboolean    editable)
{
	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    hg_quark_get_type(*quark) != HG_TYPE_REAL) {
		_hg_quark_type_bit_set_bits(quark,
					    HG_QUARK_TYPE_BIT_ACCESS,
					    HG_QUARK_TYPE_BIT_ACCESS_END,
					    ((readable << HG_QUARK_TYPE_BIT_READABLE) |
					     (writable << HG_QUARK_TYPE_BIT_WRITABLE) |
					     (executable << HG_QUARK_TYPE_BIT_EXECUTABLE) |
					     (editable << HG_QUARK_TYPE_BIT_EDITABLE)) >> HG_QUARK_TYPE_BIT_ACCESS);
	}
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
	guint xid;

	if (hg_quark_get_type(quark) == HG_TYPE_REAL)
		xid = 0;
	else
		xid = _hg_quark_type_bit_get_bits(quark,
						  HG_QUARK_TYPE_BIT_MEM_ID,
						  HG_QUARK_TYPE_BIT_MEM_ID_END);

	return xid == id;
}

/**
 * hg_quark_get_mem_id:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gint
hg_quark_get_mem_id(hg_quark_t quark)
{
	if (hg_quark_get_type(quark) == HG_TYPE_REAL)
		return 0;
	return _hg_quark_type_bit_get_bits(quark,
					   HG_QUARK_TYPE_BIT_MEM_ID,
					   HG_QUARK_TYPE_BIT_MEM_ID_END);
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
	if (*quark != Qnil &&
	    hg_quark_get_type(*quark) != HG_TYPE_REAL) {
		_hg_quark_type_bit_set_bits(quark,
					    HG_QUARK_TYPE_BIT_MEM_ID,
					    HG_QUARK_TYPE_BIT_MEM_ID_END,
					    id);
	}
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
	if (hg_quark_get_type(quark) == HG_TYPE_REAL)
		return quark;
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
		"dictnodetype",
		"pathtype",
		"gstatetype",
		NULL
	};
	static const gchar const *unknown = "unknowntype";

	hg_return_val_if_fail (hg_quark_get_type(qdata) < HG_TYPE_END, unknown);

	return types[hg_quark_get_type(qdata)];
}

/* bits of a hack to shut up the compiler warnings */
#include <hieroglyph/hgreal.h>

/**
 * hg_quark_compare:
 * @qdata1:
 * @qdata2:
 * @func:
 * @user_data:
 *
 * FIXME
 *
 * Returns:
 */
G_INLINE_FUNC gboolean
hg_quark_compare(hg_quark_t              qdata1,
		 hg_quark_t              qdata2,
		 hg_quark_compare_func_t func,
		 gpointer                user_data)
{
	hg_return_val_if_fail (func != NULL, FALSE);

	if (hg_quark_get_type(qdata1) != hg_quark_get_type(qdata2))
		return FALSE;

	if ((hg_quark_is_simple_object(qdata1) &&
	     hg_quark_get_type(qdata1) != HG_TYPE_REAL) ||
	    hg_quark_get_type(qdata1) == HG_TYPE_OPER)
		return qdata1 == qdata2;
	else if (hg_quark_get_type(qdata1) == HG_TYPE_REAL)
		return HG_REAL_EQUAL (HG_REAL (qdata1), HG_REAL (qdata2));

	return func(qdata1, qdata2, user_data);
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGQUARK_H__ */
