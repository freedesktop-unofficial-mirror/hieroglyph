/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgnull.h
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

#ifndef __HIEROGLYPH_HGNULL_H__
#define __HIEROGLYPH_HGNULL_H__

#include <hieroglyph/hgquark.h>

HG_BEGIN_DECLS

/**
 * HG_QNULL:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_QNULL						\
	(hg_quark_new(HG_TYPE_NULL, ((hg_uint_t)0xdeadbeef)))
/**
 * HG_IS_QNULL:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
#define HG_IS_QNULL(_q_)				\
	(hg_quark_get_type(_q_) == HG_TYPE_NULL)


HG_END_DECLS

#endif /* __HIEROGLYPH_HG_NULL_H__ */
