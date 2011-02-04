/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmacros.h
 * Copyright (C) 2006-2011 Akira TAGOH
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

#ifndef __HIEROGLYPH_HGMACROS_H__
#define __HIEROGLYPH_HGMACROS_H__

#include <stddef.h>

/* enable a debugging code */
#if defined(GNOME_ENABLE_DEBUG) || defined(DEBUG)
#define HG_DEBUG
#define hg_d(x)		x
#else
#define hg_d(x)
#endif /* GNOME_ENABLE_DEBUG || DEBUG */

/* Guard C code in headers, while including them from C++ */
#ifdef __cplusplus
#define HG_BEGIN_DECLS	extern "C" {
#define HG_END_DECLS	}
#else
#define HG_BEGIN_DECLS
#define HG_END_DECLS
#endif

/* statement wrappers */
#if !(defined (HG_STMT_START) && defined (HG_STMT_END))
#define HG_STMT_START	do
#define HG_STMT_END	while (0)
#endif

/*
 * The HG_LIKELY and HG_UNLIKELY macros let the programmer give hints to
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 *
 * The _HG_BOOLEAN_EXPR macro is intended to trigger a gcc warning when
 * putting assignments in hg_return_if_fail()
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _HG_BOOLEAN_EXPR(_e_)				\
	__extension__ ({				\
			int __bool_var__;		\
			if (_e_)			\
				__bool_var__ = 1;	\
			else				\
				__bool_var__ = 0;	\
			__bool_var__;			\
		})
#define HG_LIKELY(_e_)		(__builtin_expect (_HG_BOOLEAN_EXPR (_e_), 1))
#define HG_UNLIKELY(_e_)	(__builtin_expect (_HG_BOOLEAN_EXPR (_e_), 0))
#else
#define HG_LIKELY(_e_)		(_e_)
#define HG_UNLIKELY(_e_)	(_e_)
#endif

/* boolean */
#ifndef FALSE
#define FALSE	(0)
#endif
#ifndef TRUE
#define TRUE	(!FALSE)
#endif

/* interconversion between hg_quark_t and hg_pointer_t */
#define HGPOINTER_TO_QUARK(_p_)	((hg_quark_t)(_p_))
#define HGQUARK_TO_POINTER(_q_)	((hg_pointer_t)(hg_quark_t)(_q_))

/* Macros to adjust an alignment */
#define HG_ALIGNED_TO(_x_,_y_)			\
	(((_x_) + (_y_) - 1) & ~((_y_) - 1))
#define HG_ALIGNED_TO_POINTER(_x_)		\
	HG_ALIGNED_TO ((_x_), ALIGNOF_VOID_P)


#endif /* __HIEROGLYPH_HGMACROS_H__ */
