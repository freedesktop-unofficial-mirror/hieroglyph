/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgquark.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "hgreal.h"
#include "hgtypebit-private.h"
#include "hgquark.h"

#include "hgquark.proto.h"

/*< private >*/

/*< public >*/

/**
 * hg_quark_new:
 * @type:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_quark_new(hg_type_t  type,
	     hg_quark_t value)
{
	hg_quark_t retval = value;
	hg_quark_acl_t acl = HG_ACL_READABLE|HG_ACL_ACCESSIBLE;

	if (type != HG_TYPE_REAL) {
		_hg_typebit_set_value(&retval,
				      HG_TYPEBIT_TYPE,
				      HG_TYPEBIT_TYPE_END,
				      type);
		if (!hg_type_is_simple(type) &&
		    type != HG_TYPE_OPER)
			acl |= HG_ACL_WRITABLE;
		if (type == HG_TYPE_EVAL_NAME)
			acl |= HG_ACL_EXECUTABLE;
		hg_quark_set_acl(&retval, acl);
	}

	return retval;
}

/**
 * hg_quark_set_acl:
 * @quark:
 * @acl:
 *
 * FIXME
 */
void
hg_quark_set_acl(hg_quark_t     *quark,
		 hg_quark_acl_t  acl)
{
	hg_int_t readable, writable, executable, accessible;
	hg_quark_t v = 0;

	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    !HG_IS_QREAL (*quark)) {
		readable = (acl & HG_ACL_READABLE ? 1 : 0);
		writable = (acl & HG_ACL_WRITABLE ? 1 : 0);
		executable = (acl & HG_ACL_EXECUTABLE ? 1 : 0);
		accessible = (acl & HG_ACL_ACCESSIBLE ? 1 : 0);
		v = ((readable << (HG_TYPEBIT_READABLE - HG_TYPEBIT_ACCESS)) |
		     (writable << (HG_TYPEBIT_WRITABLE - HG_TYPEBIT_ACCESS)) |
		     (executable << (HG_TYPEBIT_EXECUTABLE - HG_TYPEBIT_ACCESS)) |
		     (accessible << (HG_TYPEBIT_ACCESSIBLE - HG_TYPEBIT_ACCESS)));

		_hg_typebit_set_value(quark,
				      HG_TYPEBIT_ACCESS,
				      HG_TYPEBIT_ACCESS_END,
				      v);
	}
}

/**
 * hg_quark_get_type:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
hg_type_t
hg_quark_get_type(hg_quark_t quark)
{
	if (quark == 0 ||
	    _hg_typebit_unshift(_hg_typebit_clear_range(quark,
							HG_TYPEBIT_BIT0,
							HG_TYPEBIT_END)))
		return HG_TYPE_REAL;

	return (hg_type_t)_hg_typebit_get_value(quark,
						HG_TYPEBIT_TYPE,
						HG_TYPEBIT_TYPE_END);
}

/**
 * hg_quark_get_value:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_quark_get_value(hg_quark_t quark)
{
	if (HG_IS_QREAL (quark))
	    return quark;

	return quark & (_hg_typebit_shift(0) - 1);
}

/**
 * hg_quark_set_readable:
 * quark:
 * flag:
 *
 * FIXME
 */
void
hg_quark_set_readable(hg_quark_t *quark,
		      hg_bool_t   flag)
{
	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    !HG_IS_QREAL (*quark)) {
		if (hg_quark_is_accessible(*quark)) {
			_hg_typebit_set_value(quark,
					      HG_TYPEBIT_READABLE,
					      HG_TYPEBIT_READABLE,
					      (flag == TRUE ? 1 : 0));
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
hg_bool_t
hg_quark_is_readable(hg_quark_t quark)
{
	if (HG_IS_QREAL (quark)) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return TRUE;
	}

	return _hg_typebit_get_value(quark,
				     HG_TYPEBIT_READABLE,
				     HG_TYPEBIT_READABLE) != 0;
}

/**
 * hg_quark_set_writable:
 * quark:
 * flag:
 *
 * FIXME
 */
void
hg_quark_set_writable(hg_quark_t *quark,
		      hg_bool_t   flag)
{
	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    !HG_IS_QREAL (*quark)) {
		if (hg_quark_is_accessible(*quark)) {
			_hg_typebit_set_value(quark,
					      HG_TYPEBIT_WRITABLE,
					      HG_TYPEBIT_WRITABLE,
					      (flag == TRUE ? 1 : 0));
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
hg_bool_t
hg_quark_is_writable(hg_quark_t quark)
{
	if (HG_IS_QREAL (quark)) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return FALSE;
	}

	return _hg_typebit_get_value(quark,
				     HG_TYPEBIT_WRITABLE,
				     HG_TYPEBIT_WRITABLE) != 0 &&
		hg_quark_is_accessible(quark);
}

/**
 * hg_quark_set_executable:
 * @quark:
 * @flag:
 *
 * FIXME
 */
void
hg_quark_set_executable(hg_quark_t *quark,
			hg_bool_t   flag)
{
	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    !HG_IS_QREAL (*quark)) {
		if (hg_quark_is_accessible(*quark)) {
			_hg_typebit_set_value(quark,
					      HG_TYPEBIT_EXECUTABLE,
					      HG_TYPEBIT_EXECUTABLE,
					      (flag == TRUE ? 1 : 0));
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
hg_bool_t
hg_quark_is_executable(hg_quark_t quark)
{
	if (HG_IS_QREAL (quark)) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return FALSE;
	}

	return _hg_typebit_get_value(quark,
				     HG_TYPEBIT_EXECUTABLE,
				     HG_TYPEBIT_EXECUTABLE) != 0;
}

/**
 * hg_quark_set_accessible:
 * @quark:
 * @flag:
 *
 * FIXME
 */
void
hg_quark_set_accessible(hg_quark_t *quark,
			hg_bool_t   flag)
{
	hg_return_if_fail (quark != NULL);

	if (*quark != Qnil &&
	    !HG_IS_QREAL (*quark)) {
		_hg_typebit_set_value(quark,
				      HG_TYPEBIT_ACCESSIBLE,
				      HG_TYPEBIT_ACCESSIBLE,
				      (flag == TRUE ? 1 : 0));
	}
}

/**
 * hg_quark_is_accessible:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_quark_is_accessible(hg_quark_t quark)
{
	if (HG_IS_QREAL (quark)) {
		/* this is the unfortunate limitation for
		 * the implementation of the double precision real number.
		 */
		return TRUE;
	}

	return _hg_typebit_get_value(quark,
				     HG_TYPEBIT_ACCESSIBLE,
				     HG_TYPEBIT_ACCESSIBLE) != 0;
}

/**
 * hg_quark_get_mem_id:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
hg_uint_t
hg_quark_get_mem_id(hg_quark_t quark)
{
	hg_uint_t xid;

	if (HG_IS_QREAL (quark)) {
		/* pseudo memory id */
		xid = 0;
	} else {
		xid = _hg_typebit_get_value(quark,
					    HG_TYPEBIT_MEM_ID,
					    HG_TYPEBIT_MEM_ID_END);
	}

	return xid;
}

/**
 * hg_quark_set_mem_id:
 * @quark:
 * @id:
 *
 * FIXME
 */
void
hg_quark_set_mem_id(hg_quark_t *quark,
		    hg_uint_t   id)
{
	if (*quark != Qnil &&
	    !HG_IS_QREAL (*quark)) {
		_hg_typebit_set_value(quark,
				      HG_TYPEBIT_MEM_ID,
				      HG_TYPEBIT_MEM_ID_END,
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
hg_quark_t
hg_quark_get_hash(hg_quark_t quark)
{
	if (HG_IS_QREAL (quark))
		return quark;

	return _hg_typebit_clear_range(quark,
				       HG_TYPEBIT_ACCESS,
				       HG_TYPEBIT_ACCESS_END);
}

/**
 * hg_quark_get_type_name:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
const hg_char_t *
hg_quark_get_type_name(hg_quark_t quark)
{
	static const hg_char_t const *types[] = {
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
	static const hg_char_t const *unknown = "unknowntype";

	if (hg_quark_get_type(quark) >= HG_TYPE_END)
		return unknown;

	return types[hg_quark_get_type(quark)];
}


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
hg_bool_t
hg_quark_compare(hg_quark_t              qdata1,
		 hg_quark_t              qdata2,
		 hg_quark_compare_func_t func,
		 hg_pointer_t            user_data)
{
	hg_return_val_if_fail (func != NULL, FALSE);

	if (hg_quark_get_type(qdata1) != hg_quark_get_type(qdata2))
		return FALSE;

	if ((hg_quark_is_simple_object(qdata1) &&
	     !HG_IS_QREAL (qdata1)) ||
	    hg_quark_get_type(qdata1) == HG_TYPE_OPER) {
		return qdata1 == qdata2;
	} else if (HG_IS_QREAL (qdata1)) {
		return HG_REAL_EQUAL (HG_REAL (qdata1), HG_REAL (qdata2));
	}

	return func(qdata1, qdata2, user_data);
}
