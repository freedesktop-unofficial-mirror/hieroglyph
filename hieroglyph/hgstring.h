/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.h
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
#ifndef __HIEROGLYPH_HGSTRING_H__
#define __HIEROGLYPH_HGSTRING_H__

#include <string.h>
#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

#define HG_STRING_INIT						\
	(hg_object_register(HG_TYPE_STRING,			\
			    hg_object_string_get_vtable()))

#define HG_QSTRING(_m_,_s_)					\
	(hg_string_new_with_value((_m_), NULL, (_s_), strlen(_s_)))
#define HG_IS_QSTRING(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_STRING)


typedef struct _hg_bs_string_t		hg_bs_string_t;
typedef struct _hg_string_t		hg_string_t;

struct _hg_bs_string_t {
	hg_bs_template_t t;
	guint16          length;
	guint32          offset;
};
struct _hg_string_t {
	hg_object_t o;
	hg_quark_t  qstring;
	guint16     offset;
	guint16     length;
	gsize       allocated_size;
	gboolean    is_fixed_size;
};

hg_object_vtable_t *hg_object_string_get_vtable (void);
hg_quark_t          hg_string_new               (hg_mem_t          *mem,
                                                 gpointer          *ret,
                                                 gsize              requisition_size);
hg_quark_t          hg_string_new_with_value    (hg_mem_t          *mem,
                                                 gpointer          *ret,
                                                 const gchar       *string,
                                                 gsize              length);
guint               hg_string_length            (const hg_string_t *string);
guint               hg_string_maxlength         (const hg_string_t *string);
gboolean            hg_string_clear             (hg_string_t       *string);
gboolean            hg_string_append_c          (hg_string_t       *string,
                                                 gchar              c);
gboolean            hg_string_append            (hg_string_t       *string,
                                                 const gchar       *str,
                                                 gssize             length);
gboolean            hg_string_insert_c          (hg_string_t       *string,
                                                 gchar              c,
                                                 guint              index);
gboolean            hg_string_erase             (hg_string_t       *string,
                                                 gssize             pos,
                                                 gssize             length);
gboolean            hg_string_concat            (hg_string_t       *string1,
                                                 hg_string_t       *string2);
gchar               hg_string_index             (hg_string_t       *string,
                                                 guint              index);
const gchar        *hg_string_get_static_cstr   (hg_string_t       *string);
hg_quark_t          hg_string_get_cstr          (hg_string_t       *string,
                                                 gpointer          *ret);
gboolean            hg_string_fix_string_size   (hg_string_t       *string);
gboolean            hg_string_compare           (const hg_string_t *a,
                                                 const hg_string_t *b);
gboolean            hg_string_ncompare          (const hg_string_t *a,
                                                 const hg_string_t *b,
                                                 guint              length);
gboolean            hg_string_ncompare_with_cstr(const hg_string_t *a,
                                                 const gchar       *b,
                                                 gssize             length);

G_END_DECLS

#endif /* __HIEROGLYPH_HGSTRING_H__ */
