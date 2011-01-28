/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgtypes.h
 * Copyright (C) 2005-2011 Akira TAGOH
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

#ifndef __HIEROGLYPH_HGTYPES_H__
#define __HIEROGLYPH_HGTYPES_H__

#include <stdint.h>
#include <sys/types.h>
#include <hieroglyph/hgmacros.h>

HG_BEGIN_DECLS

/* basic types */

/* XXX: We may get rid of GLib dependency in the future */
typedef int		hg_bool_t;
typedef int32_t		hg_int_t;
typedef uint32_t	hg_uint_t;
typedef double		hg_real_t;
typedef char		hg_char_t;
typedef unsigned char	hg_uchar_t;
typedef int8_t		hg_int8_t;
typedef uint8_t		hg_uint8_t;
typedef int16_t		hg_int16_t;
typedef uint16_t	hg_uint16_t;
typedef void *		hg_pointer_t;
typedef uint64_t	hg_quark_t;
typedef size_t		hg_usize_t;
typedef ssize_t		hg_size_t;

#define Qnil	(hg_quark_t)((hg_uint_t)-1)

/* types in hg_quark_t */
typedef enum _hg_type_t {
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
} hg_type_t;

/* types commonly used in libarry */
typedef struct _hg_mem_t	hg_mem_t;
typedef struct _hg_vm_t		hg_vm_t;

/* types of callback functions */
typedef hg_bool_t (* hg_cb_BOOL__QUARK_t)       (hg_mem_t      *mem,
						 hg_quark_t     q,
						 hg_pointer_t   data,
						 GError       **error);
typedef hg_bool_t (* hg_cb_BOOL__QUARK_QUARK_t) (hg_mem_t      *mem,
						 hg_quark_t     q1,
						 hg_quark_t     q2,
						 hg_pointer_t   data,
						 GError       **error);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGTYPES_H__ */
