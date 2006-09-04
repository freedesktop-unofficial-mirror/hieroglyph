/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * ibtree.h
 * Copyright (C) 2006 Akira TAGOH
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
#ifndef __HG_BTREE_H__
#define __HG_BTREE_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

typedef struct _HieroGlyphBTreePage	HgBTreePage;
typedef struct _HieroGlyphBTree		HgBTree;
typedef struct _HieroGlyphBTreeIter	*HgBTreeIter;

struct _HieroGlyphBTreeIter {
	gpointer id;
	gpointer stamp;
	guint    seq;
	gpointer key;
	gpointer val;
};

struct _HieroGlyphBTreePage {
	guint         n_data;
	gpointer     *key;
	gpointer     *val;
	HgBTreePage **page;
};

struct _HieroGlyphBTree {
	HgBTreePage    *root;
	guint           page_size;
	GDestroyNotify  key_destroy_func;
	GDestroyNotify  val_destroy_func;
};


#define hg_btree_new		_hg_btree_new
#define hg_btree_new_full	_hg_btree_new_full
#define hg_btree_destroy	_hg_btree_destroy
#define hg_btree_add		_hg_btree_add
#define hg_btree_replace	_hg_btree_replace
#define hg_btree_remove		_hg_btree_remove
#define hg_btree_find		_hg_btree_find
#define hg_btree_find_near	_hg_btree_find_near
#define hg_btree_foreach	_hg_btree_foreach
#define hg_btree_length		_hg_btree_length
#define hg_btree_iter_new	_hg_btree_iter_new
#define hg_btree_iter_free	g_free
#define hg_btree_get_iter_first	_hg_btree_get_iter_first
#define hg_btree_get_iter_next	_hg_btree_get_iter_next
#define hg_btree_is_iter_valid	_hg_btree_is_iter_valid
#define hg_btree_update_iter	_hg_btree_update_iter


HgBTree    *hg_btree_new      (guint          page_size);
HgBTree    *hg_btree_new_full (guint          page_size,
			       GDestroyNotify key_destroy_func,
			       GDestroyNotify val_destroy_func);
void        hg_btree_destroy  (HgBTree        *tree);
void        hg_btree_add      (HgBTree        *tree,
			       gpointer        key,
			       gpointer        val);
void        hg_btree_replace  (HgBTree        *tree,
			       gpointer        key,
			       gpointer        val);
void        hg_btree_remove   (HgBTree        *tree,
			       gpointer        key);
gpointer    hg_btree_find     (HgBTree        *tree,
			       gpointer        key);
gpointer    hg_btree_find_near(HgBTree       *tree,
			       gpointer       key);
void        hg_btree_foreach  (HgBTree        *tree,
			       HgTraverseFunc  func,
			       gpointer        data);
guint       hg_btree_length   (HgBTree        *tree);
/* iterator */
HgBTreeIter hg_btree_iter_new      (void);
gboolean    hg_btree_get_iter_first(HgBTree     *tree,
				    HgBTreeIter  iter);
gboolean    hg_btree_get_iter_next (HgBTree     *tree,
				    HgBTreeIter  iter);
gboolean    hg_btree_is_iter_valid (HgBTree     *tree,
				    HgBTreeIter  iter);
gboolean    hg_btree_update_iter   (HgBTree     *tree,
				    HgBTreeIter  iter);

G_END_DECLS

#endif /* __HG_BTREE_H__ */
