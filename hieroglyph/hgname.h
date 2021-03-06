/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgname.h
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

#ifndef __HIEROGLYPH_HGNAME_H__
#define __HIEROGLYPH_HGNAME_H__

#include <hieroglyph/hgencoding.h>

HG_BEGIN_DECLS

#define HG_QNAME(_s_)				\
	hg_name_new_with_string(_s_, -1)
#define HG_NAME(_q_)				\
	(hg_name_lookup((_q_)))
#define HG_IS_QNAME(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_NAME)
#define HG_QEVALNAME(_s_)					\
	(hg_quark_new(HG_TYPE_EVAL_NAME, HG_QNAME ((_s_))))
#define HG_IS_QEVALNAME(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_EVAL_NAME)


void             hg_name_init             (void);
void             hg_name_tini             (void);
hg_quark_t       hg_name_new_with_encoding(hg_system_encoding_t  encoding);
hg_quark_t       hg_name_new_with_string  (const hg_char_t      *string,
					   hg_size_t             len);
const hg_char_t *hg_name_lookup           (hg_quark_t            quark);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGNAME_H__ */
