/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbool.h
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
#ifndef __HIEROGLYPH_HGBOOL_H__
#define __HIEROGLYPH_HGBOOL_H__

#include <hieroglyph/hgquark.h>

G_BEGIN_DECLS

typedef struct _hg_bs_bool_t	hg_bs_bool_t;

struct _hg_bs_bool_t {
	hg_bs_template_t t;
	guint16          unused1;
	guint32          value;
};


#define HG_QBOOL(_v_)					\
	hg_quark_new(HG_TYPE_BOOL, (_v_) == TRUE)
#define HG_BOOL(_q_)				\
	(hg_quark_get_value(_q_) == TRUE)
#define HG_IS_QBOOL(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_BOOL)


G_END_DECLS

#endif /* __HIEROGLYPH_HGBOOL_H__ */
