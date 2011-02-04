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

#include <stdio.h>
#include <errno.h>
/* XXX: GLib deps are still needed for GQuark */
#include <glib.h>
#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

#define HG_ERROR_VM_STATUS_MASK_SHIFT	8
#define HG_ERROR_VM_STATUS_MASK		((1 << HG_ERROR_VM_STATUS_MASK_SHIFT) - 1)

#define HG_ERROR_GET_VM_STATUS(_e_)		\
	((_e_) & HG_ERROR_VM_STATUS_MASK)
#define HG_ERROR_SET_VM_STATUS(_e_,_t_)		\
	(((_e_) & ~HG_ERROR_VM_STATUS_MASK) |	\
	 ((_t_) & HG_ERROR_VM_STATUS_MASK))
#define HG_ERROR_GET_REASON(_e_)					\
	(((_e_) & ~(HG_ERROR_VM_STATUS_MASK)) >> HG_ERROR_VM_STATUS_MASK_SHIFT)
#define HG_ERROR_SET_REASON(_e_,_t_)					\
	(((_e_) & HG_ERROR_STATUS_MASK) | ((_t_) << HG_ERROR_VM_STATUS_MASK_SHIFT))
#define HG_ERROR	(hg_error_quark())
#define HG_ERROR_(_status_,_reason_)				\
	(hg_error_t)(HG_ERROR_SET_VM_STATUS (0, (_status_)) |	\
		     HG_ERROR_SET_REASON (0, (_reason_)))

typedef hg_int_t		hg_error_t;
typedef enum _hg_vm_error_t	hg_vm_error_t;
typedef enum _hg_error_reason_t	hg_error_reason_t;

enum _hg_vm_error_t {
	HG_VM_e_dictfull = 1,
	HG_VM_e_dictstackoverflow,
	HG_VM_e_dictstackunderflow,
	HG_VM_e_execstackoverflow,
	HG_VM_e_handleerror,
	HG_VM_e_interrupt,
	HG_VM_e_invalidaccess,
	HG_VM_e_invalidexit,
	HG_VM_e_invalidfileaccess,
	HG_VM_e_invalidfont,
	HG_VM_e_invalidrestore,
	HG_VM_e_ioerror,
	HG_VM_e_limitcheck,
	HG_VM_e_nocurrentpoint,
	HG_VM_e_rangecheck,
	HG_VM_e_stackoverflow,
	HG_VM_e_stackunderflow,
	HG_VM_e_syntaxerror,
	HG_VM_e_timeout,
	HG_VM_e_typecheck,
	HG_VM_e_undefined,
	HG_VM_e_undefinedfilename,
	HG_VM_e_undefinedresult,
	HG_VM_e_unmatchedmark,
	HG_VM_e_unregistered,
	HG_VM_e_VMerror,
	HG_VM_e_configurationerror,
	HG_VM_e_undefinedresource,
	HG_VM_e_END
};
enum _hg_error_reason_t {
	HG_e_END
};

GQuark    hg_error_quark          (void);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGERROR_H__ */
