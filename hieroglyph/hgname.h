/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgname.h
 * Copyright (C) 2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGNAME_H__
#define __HIEROGLYPH_HGNAME_H__

#include <hieroglyph/hgencoding.h>

G_BEGIN_DECLS

#define HG_QNAME(_n_,_s_)			\
	hg_name_new_with_string(_n_,_s_, -1)
#define HG_IS_QNAME(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_NAME)

typedef struct _hg_name_t	hg_name_t;
typedef struct _hg_bs_name_t	hg_bs_name_t;
typedef enum _hg_bs_name_type_t	hg_bs_name_type_t;

enum _hg_bs_name_type_t {
	HG_name_offset = 1,
	HG_name_DPS = 0,
	HG_name_index = -1,
};
struct _hg_bs_name_t {
	hg_bs_template_t t;
	gint16           representation;
	union {
		guint32      index;
		guint32      offset;
	} v;
};


hg_name_t   *hg_name_init             (void);
void         hg_name_tini             (hg_name_t            *name);
hg_quark_t   hg_name_new_with_encoding(hg_name_t            *name,
                                       hg_system_encoding_t  encoding);
hg_quark_t   hg_name_new_with_string  (hg_name_t            *name,
                                       const gchar          *string,
                                       gssize                len);
const gchar *hg_name_lookup           (hg_name_t            *name,
                                       hg_quark_t            quark);

G_END_DECLS

#endif /* __HIEROGLYPH_HGNAME_H__ */
