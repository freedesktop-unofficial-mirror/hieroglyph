/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgint.h
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGINT_H__
#define __HIEROGLYPH_HGINT_H__

#include <hieroglyph/hgquark.h>

HG_BEGIN_DECLS

/**
 * HG_QINT:
 * @value:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_QINT(_v_)					\
	(hg_quark_new(HG_TYPE_INT, (hg_uint_t)(_v_)))
/**
 * HG_INT:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_INT(_q_)				\
	((hg_int_t)(hg_quark_get_value((_q_))))
/**
 * HG_IS_QINT:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_IS_QINT(_q_)				\
	(hg_quark_get_type(_q_) == HG_TYPE_INT)


HG_END_DECLS

#endif /* __HIEROGLYPH_HGINT_H__ */
