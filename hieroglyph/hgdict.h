/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.h
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
#ifndef __HG_DICT_H__
#define __HG_DICT_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


HgDict      *hg_dict_new               (HgMemPool      *pool,
					guint           n_prealloc);
gboolean     hg_dict_insert            (HgMemPool      *pool,
					HgDict         *dict,
					HgValueNode    *key,
					HgValueNode    *val);
gboolean     hg_dict_insert_forcibly   (HgMemPool      *pool,
					HgDict         *dict,
					HgValueNode    *key,
					HgValueNode    *val,
					gboolean        force);
gboolean     hg_dict_remove            (HgDict         *dict,
					HgValueNode    *key);
HgValueNode *hg_dict_lookup            (const HgDict   *dict,
					HgValueNode    *key);
HgValueNode *hg_dict_lookup_with_string(HgDict         *dict,
					const gchar    *key);
guint        hg_dict_length            (const HgDict   *dict);
guint        hg_dict_maxlength         (const HgDict   *dict);
gboolean     hg_dict_traverse          (const HgDict   *dict,
					HgTraverseFunc  func,
					gpointer        data);
gboolean     hg_dict_first             (HgDict         *dict,
					HgValueNode   **key,
					HgValueNode   **val);
gboolean     hg_dict_compare           (const HgDict   *a,
					const HgDict   *b,
					guint           attributes_mask);


G_END_DECLS

#endif /* __HG_DICT_H__ */
