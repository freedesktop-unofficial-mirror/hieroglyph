/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.h
 * Copyright (C) 2005-2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#ifndef __HG_STRING_H__
#define __HG_STRING_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


HgString *hg_string_new                 (HgMemPool      *pool,
					 gint32          n_prealloc);
guint     hg_string_length              (HgString       *string);
guint     hg_string_maxlength           (HgString       *string);
gboolean  hg_string_clear               (HgString       *string);
gboolean  hg_string_append_c            (HgString       *string,
					 gchar           c);
gboolean  hg_string_append              (HgString       *string,
					 const gchar    *str,
					 gint            length);
gboolean  hg_string_insert_c            (HgString       *string,
					 gchar           c,
					 guint           index);
gboolean  hg_string_concat              (HgString       *string1,
					 HgString       *string2);
gchar     hg_string_index               (HgString       *string,
					 guint           index);
gchar    *hg_string_get_string          (HgString       *string);
gboolean  hg_string_fix_string_size     (HgString       *string);
gboolean  hg_string_compare             (const HgString *a,
					 const HgString *b);
gboolean  hg_string_ncompare            (const HgString *a,
					 const HgString *b,
					 guint           length);
gboolean  hg_string_compare_with_raw    (const HgString *a,
					 const gchar    *b,
					 gint            length);
HgString *hg_string_make_substring      (HgMemPool      *pool,
					 HgString       *string,
					 guint           start_index,
					 guint           end_index);
gboolean  hg_string_copy_as_substring   (HgString       *src,
					 HgString       *dest,
					 guint           start_index,
					 guint           end_index);
gboolean  hg_string_convert_from_integer(HgString       *string,
					 gint32          num,
					 guint           radix,
					 gboolean        is_lower);

/* HgObject */
HgString *hg_object_to_string(HgObject *object);

G_END_DECLS

#endif /* __HG_STRING_H__ */
