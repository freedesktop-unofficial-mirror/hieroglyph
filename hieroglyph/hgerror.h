/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgerror.h
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

#ifndef __HIEROGLYPH_HGERROR_H__
#define __HIEROGLYPH_HGERROR_H__

#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

#define HG_ERROR_STATUS_MASK_SHIFT	8
#define HG_ERROR_STATUS_MASK		((1 << HG_ERROR_STATUS_MASK_SHIFT) - 1)

#define HG_ERROR_GET_STATUS(_e_)		\
	((_e_) & HG_ERROR_STATUS_MASK)
#define HG_ERROR_SET_STATUS(_e_,_t_)		\
	(((_e_) & ~HG_ERROR_STATUS_MASK) |	\
	 ((_t_) & HG_ERROR_STATUS_MASK))
#define HG_ERROR_GET_REASON(_e_)					\
	(((_e_) & ~(HG_ERROR_STATUS_MASK)) >> HG_ERROR_STATUS_MASK_SHIFT)
#define HG_ERROR_SET_REASON(_e_,_t_)					\
	(((_e_) & HG_ERROR_STATUS_MASK) | ((_t_) << HG_ERROR_STATUS_MASK_SHIFT))
#define HG_ERROR_IS_SUCCESS(_e_)			\
	(HG_ERROR_GET_STATUS(_e_) == HG_STATUS_SUCCESS)
#define HG_ERROR_IS_SUCCESS0()			\
	(HG_ERROR_IS_SUCCESS (hg_errno))

#define HG_ERROR	(hg_error_quark())
#define HG_ERROR_(_status_,_reason_)				\
	(hg_error_t)(HG_ERROR_SET_STATUS (0, (_status_)) |	\
		     HG_ERROR_SET_REASON (0, (_reason_)))

#define hg_error_return0(_status_,_reason_)			\
	HG_STMT_START {						\
		hg_errno = HG_ERROR_ ((_status_), (_reason_));	\
		return;						\
	} HG_STMT_END
#define hg_error_return_val(_v_,_status_,_reason_)		\
	HG_STMT_START {						\
		hg_errno = HG_ERROR_ ((_status_), (_reason_));	\
		return (_v_);					\
	} HG_STMT_END
#define hg_error_return(_status_,_reason_)				\
	hg_error_return_val((_status_) == HG_STATUS_SUCCESS, (_status_), (_reason_))

typedef enum _hg_error_status_t	hg_error_status_t;
typedef enum _hg_error_reason_t	hg_error_reason_t;

enum _hg_error_status_t {
	HG_STATUS_SUCCESS = 0,
	HG_STATUS_FAILED = 1,
	HG_STATUS_END
};
enum _hg_error_reason_t {
	HG_e_dictfull = 1,
	HG_e_dictstackoverflow,
	HG_e_dictstackunderflow,
	HG_e_execstackoverflow,
	HG_e_handleerror,
	HG_e_interrupt,
	HG_e_invalidaccess,
	HG_e_invalidexit,
	HG_e_invalidfileaccess,
	HG_e_invalidfont,
	HG_e_invalidrestore,
	HG_e_ioerror,
	HG_e_limitcheck,
	HG_e_nocurrentpoint,
	HG_e_rangecheck,
	HG_e_stackoverflow,
	HG_e_stackunderflow,
	HG_e_syntaxerror,
	HG_e_timeout,
	HG_e_typecheck,
	HG_e_undefined,
	HG_e_undefinedfilename,
	HG_e_undefinedresult,
	HG_e_unmatchedmark,
	HG_e_unregistered,
	HG_e_VMerror,
	HG_e_configurationerror,
	HG_e_undefinedresource,
	HG_e_END
};

extern HG_THREAD_VAR hg_error_t hg_errno;

HG_END_DECLS

#endif /* __HIEROGLYPH_HGERROR_H__ */
