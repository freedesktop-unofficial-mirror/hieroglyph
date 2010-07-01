/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbtree.h
 * Copyright (C) 2006-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGBTREE_H__
#define __HIEROGLYPH_HGBTREE_H__

#include <hieroglyph/hgquark.h>

G_BEGIN_DECLS

typedef struct _hg_btree_t	hg_btree_t;
typedef struct _hg_btree_node_t	hg_btree_node_t;
typedef gboolean (* hg_btree_traverse_func_t) (hg_mem_t    *mem,
					       hg_quark_t   qkey,
					       hg_quark_t   qval,
					       gpointer     data,
					       GError     **error);


hg_quark_t hg_btree_new    (hg_mem_t                 *mem,
                            gsize                     size,
                            gpointer                 *ret);
void       hg_btree_destroy(hg_mem_t                 *mem,
			    hg_quark_t                qtree);
void       hg_btree_add    (hg_btree_t               *tree,
                            hg_quark_t                qkey,
                            hg_quark_t                qval,
                            GError                   **error);
void       hg_btree_remove (hg_btree_t               *tree,
                            hg_quark_t                qkey,
                            GError                   **error);
hg_quark_t hg_btree_find   (hg_btree_t               *tree,
                            hg_quark_t                qkey,
                            GError                   **error);
void       hg_btree_foreach(hg_btree_t               *tree,
                            hg_btree_traverse_func_t  func,
                            gpointer                  data,
                            GError                   **error);
gsize      hg_btree_length (hg_btree_t               *tree,
                            GError                   **error);


G_END_DECLS

#endif /* __HIEROGLYPH_HGBTREE_H__ */
