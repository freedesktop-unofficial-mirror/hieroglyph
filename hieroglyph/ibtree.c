/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * ibtree.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ibtree.h"


static HgBTreePage *hg_btree_page_new                 (HgBTree      *tree);
static void         hg_btree_page_destroy             (HgBTree      *tree,
						       HgBTreePage  *page);
static void         hg_btree_page_free                (HgBTreePage  *page);
static gboolean     hg_btree_page_insert              (HgBTree      *tree,
						       HgBTreePage  *page,
						       gpointer     *key,
						       gpointer     *val,
						       gboolean      replace,
						       HgBTreePage **newpage);
static void         hg_btree_page_insert_data         (HgBTree      *tree,
						       HgBTreePage  *page,
						       gpointer     *key,
						       gpointer     *val,
						       guint         pos,
						       HgBTreePage **newpage);
static void         hg_btree_page_blance              (HgBTree      *tree,
						       HgBTreePage  *page,
						       gpointer     *key,
						       gpointer     *val,
						       guint         pos,
						       HgBTreePage **newpage);
static gboolean     hg_btree_page_remove              (HgBTree      *tree,
						       HgBTreePage  *page,
						       gpointer     *key,
						       gpointer     *val,
						       gboolean     *need_restore);
static gboolean     hg_btree_page_remove_data         (HgBTree      *tree,
						       HgBTreePage  *page,
						       guint         pos);
static gboolean     hg_btree_page_restore             (HgBTree      *tree,
						       HgBTreePage  *page,
						       guint         pos);
static void         hg_btree_page_restore_right_blance(HgBTree      *tree,
						       HgBTreePage  *page,
						       guint         pos);
static void         hg_btree_page_restore_left_blance (HgBTree      *tree,
						       HgBTreePage  *page,
						       guint         pos);
static gboolean     hg_btree_page_combine             (HgBTree      *tree,
						       HgBTreePage  *page,
						       guint         pos);


/*
 * Private Functions
 */
static HgBTreePage *
hg_btree_page_new(HgBTree *tree)
{
	HgBTreePage *retval;

	retval = g_new(HgBTreePage, 1);
	retval->n_data = 0;
	retval->key = g_new(gpointer, tree->page_size * 2);
	retval->val = g_new(gpointer, tree->page_size * 2);
	retval->page = g_new(HgBTreePage *, tree->page_size * 2 + 1);

	return retval;
}

static void
hg_btree_page_destroy(HgBTree *tree, HgBTreePage *page)
{
	guint i;

	if (page != NULL) {
		for (i = 0; i < page->n_data; i++) {
			hg_btree_page_destroy(tree, page->page[i]);
			if (tree->key_destroy_func)
				tree->key_destroy_func(page->key[i]);
			if (tree->val_destroy_func)
				tree->val_destroy_func(page->val[i]);
		}
		hg_btree_page_destroy(tree, page->page[page->n_data]);
		hg_btree_page_free(page);
	}
}

static void
hg_btree_page_free(HgBTreePage *page)
{
	g_free(page->key);
	g_free(page->val);
	g_free(page->page);
	g_free(page);
}

static gboolean
hg_btree_page_insert(HgBTree      *tree,
		     HgBTreePage  *page,
		     gpointer     *key,
		     gpointer     *val,
		     gboolean      replace,
		     HgBTreePage **newpage)
{
	guint i;
	gboolean retval;

	if (page == NULL) {
		*newpage = NULL;
		return FALSE;
	}
	if (page->key[page->n_data - 1] >= *key) {
		for (i = 0; i < page->n_data && page->key[i] < *key; i++);
		if (i < page->n_data && page->key[i] == *key) {
			if (replace) {
				if (tree->val_destroy_func)
					tree->val_destroy_func(page->val[i]);
				page->key[i] = *key;
				page->val[i] = *val;
			}
			return TRUE;
		}
	} else {
		i = page->n_data;
	}
	retval = hg_btree_page_insert(tree, page->page[i], key, val, replace, newpage);
	if (!retval) {
		if (page->n_data < tree->page_size * 2) {
			hg_btree_page_insert_data(tree, page, key, val, i, newpage);
			retval = TRUE;
		} else {
			hg_btree_page_blance(tree, page, key, val, i, newpage);
			retval = FALSE;
		}
	}
	return retval;
}

static void
hg_btree_page_insert_data(HgBTree      *tree,
			  HgBTreePage  *page,
			  gpointer     *key,
			  gpointer     *val,
			  guint         pos,
			  HgBTreePage **newpage)
{
	guint i;

	for (i = page->n_data; i > pos; i--) {
		page->key[i] = page->key[i - 1];
		page->val[i] = page->val[i - 1];
		page->page[i + 1] = page->page[i];
	}
	page->key[pos] = *key;
	page->val[pos] = *val;
	page->page[pos + 1] = *newpage;
	page->n_data++;
}

static void
hg_btree_page_blance(HgBTree      *tree,
		     HgBTreePage  *page,
		     gpointer     *key,
		     gpointer     *val,
		     guint         pos,
		     HgBTreePage **newpage)
{
	guint i, size;
	HgBTreePage *new;

	if (pos <= tree->page_size)
		size = tree->page_size;
	else
		size = tree->page_size + 1;
	new = hg_btree_page_new(tree);
	if (new == NULL) {
		g_warning("Failed to allocate a memory.");
		return;
	}
	for (i = size + 1; i <= tree->page_size * 2; i++) {
		new->key[i - size - 1] = page->key[i - 1];
		new->val[i - size - 1] = page->val[i - 1];
		new->page[i - size] = page->page[i];
	}
	new->n_data = tree->page_size * 2 - size;
	page->n_data = size;
	if (pos <= tree->page_size)
		hg_btree_page_insert_data(tree, page, key, val, pos, newpage);
	else
		hg_btree_page_insert_data(tree, new, key, val, pos - size, newpage);
	*key = page->key[page->n_data - 1];
	*val = page->val[page->n_data - 1];
	new->page[0] = page->page[page->n_data];
	page->n_data--;
	*newpage = new;
}

static gboolean
hg_btree_page_remove(HgBTree     *tree,
		     HgBTreePage *page,
		     gpointer    *key,
		     gpointer    *val,
		     gboolean    *need_restore)
{
	HgBTreePage *p;
	guint i;
	gboolean removed = FALSE;

	if (page == NULL)
		return FALSE;
	if (page->key[page->n_data - 1] >= *key)
		for (i = 0; i < page->n_data && page->key[i] < *key; i++);
	else
		i = page->n_data;
	if (i < page->n_data && page->key[i] == *key) {
		removed = TRUE;
		if (tree->key_destroy_func)
			tree->key_destroy_func(page->key[i]);
		if (tree->val_destroy_func)
			tree->val_destroy_func(page->val[i]);
		if ((p = page->page[i + 1]) != NULL) {
			while (p->page[0] != NULL)
				p = p->page[0];
			page->key[i] = *key = p->key[0];
			page->val[i] = *val = p->val[0];
			removed = hg_btree_page_remove(tree, page->page[i + 1], key, val, need_restore);
			if (*need_restore)
				*need_restore = hg_btree_page_restore(tree, page, i + 1);
		} else {
			*need_restore = hg_btree_page_remove_data(tree, page, i);
		}
	} else {
		removed = hg_btree_page_remove(tree, page->page[i], key, val, need_restore);
		if (*need_restore)
			*need_restore = hg_btree_page_restore(tree, page, i);
	}

	return removed;
}

static gboolean
hg_btree_page_remove_data(HgBTree     *tree,
			  HgBTreePage *page,
			  guint        pos)
{
	while (++pos < page->n_data) {
		page->key[pos - 1] = page->key[pos];
		page->val[pos - 1] = page->val[pos];
		page->page[pos] = page->page[pos + 1];
	}

	return --(page->n_data) < tree->page_size;
}

static gboolean
hg_btree_page_restore(HgBTree     *tree,
		      HgBTreePage *page,
		      guint        pos)
{
	if (pos > 0) {
		if (page->page[pos - 1]->n_data > tree->page_size)
			hg_btree_page_restore_right_blance(tree, page, pos);
		else
			return hg_btree_page_combine(tree, page, pos);
	} else {
		if (page->page[1]->n_data > tree->page_size)
			hg_btree_page_restore_left_blance(tree, page, 1);
		else
			return hg_btree_page_combine(tree, page, 1);
	}
	return FALSE;
}

static void
hg_btree_page_restore_right_blance(HgBTree     *tree,
				   HgBTreePage *page,
				   guint        pos)
{
	guint i;
	HgBTreePage *left, *right;

	left = page->page[pos - 1];
	right = page->page[pos];

	for (i = right->n_data; i > 0; i--) {
		right->key[i] = right->key[i - 1];
		right->val[i] = right->val[i - 1];
		right->page[i + 1] = right->page[i];
	}
	right->page[1] = right->page[0];
	right->n_data++;
	right->key[0] = page->key[pos - 1];
	right->val[0] = page->val[pos - 1];
	page->key[pos - 1] = left->key[left->n_data - 1];
	page->val[pos - 1] = left->val[left->n_data - 1];
	right->page[0] = left->page[left->n_data];
	left->n_data--;
}

static void
hg_btree_page_restore_left_blance(HgBTree     *tree,
				  HgBTreePage *page,
				  guint        pos)
{
	guint i;
	HgBTreePage *left, *right;

	left = page->page[pos - 1];
	right = page->page[pos];
	left->n_data++;
	left->key[left->n_data - 1] = page->key[pos - 1];
	left->val[left->n_data - 1] = page->val[pos - 1];
	left->page[left->n_data] = right->page[0];
	page->key[pos - 1] = right->key[0];
	page->val[pos - 1] = right->val[0];
	right->page[0] = right->page[1];
	right->n_data--;
	for (i = 1; i <= right->n_data; i++) {
		right->key[i - 1] = right->key[i];
		right->val[i - 1] = right->val[i];
		right->page[i] = right->page[i + 1];
	}
}

static gboolean
hg_btree_page_combine(HgBTree     *tree,
		      HgBTreePage *page,
		      guint        pos)
{
	guint i;
	HgBTreePage *left, *right;
	gboolean need_restore;

	left = page->page[pos - 1];
	right = page->page[pos];
	left->n_data++;
	left->key[left->n_data - 1] = page->key[pos - 1];
	left->val[left->n_data - 1] = page->val[pos - 1];
	left->page[left->n_data] = right->page[0];
	for (i = 1; i <= right->n_data; i++) {
		left->n_data++;
		left->key[left->n_data - 1] = right->key[i - 1];
		left->val[left->n_data - 1] = right->val[i - 1];
		left->page[left->n_data] = right->page[i];
	}
	need_restore = hg_btree_page_remove_data(tree, page, pos - 1);
	hg_btree_page_free(right);

	return need_restore;
}

static void
hg_btree_page_foreach(HgBTreePage    *page,
		      HgTraverseFunc  func,
		      gpointer        data)
{
	if (page != NULL) {
		guint i;

		for (i = 0; i < page->n_data; i++) {
			hg_btree_page_foreach(page->page[i], func, data);
			if (!func(page->key[i], page->val[i], data))
				return;
		}
		hg_btree_page_foreach(page->page[page->n_data], func, data);
	}
}

static gboolean
hg_btree_page_get_iter(HgBTreePage *page,
		       guint       *sequence,
		       HgBTreeIter  iter,
		       gboolean     valid_stamp)
{
	gboolean retval = FALSE;

	if (page != NULL) {
		guint i;

		for (i = 0; i < page->n_data; i++) {
			retval = hg_btree_page_get_iter(page->page[i], sequence, iter, valid_stamp);
			if (retval) {
				return retval;
			} else {
				if (valid_stamp) {
					if (*sequence == iter->seq) {
						iter->key = page->key[i];
						iter->val = page->val[i];
						iter->seq++;

						return TRUE;
					}
				} else {
					if (iter->key == page->key[i]) {
						iter->seq = *sequence;

						return TRUE;
					} else if (page->key[i] > iter->key) {
						iter->seq = *sequence - 1;

						return TRUE;
					}
				}
				(*sequence)++;
			}
		}
		retval = hg_btree_page_get_iter(page->page[page->n_data], sequence, iter, valid_stamp);
	}

	return retval;
}

static gboolean
_hg_btree_count_traverse(gpointer key,
			 gpointer val,
			 gpointer data)
{
	guint *len = data;

	(*len)++;

	return TRUE;
}

/*
 * Public Functions
 */
HgBTree *
_hg_btree_new(guint page_size)
{
	return _hg_btree_new_full(page_size, NULL, NULL);
}

HgBTree *
_hg_btree_new_full(guint          page_size,
		   GDestroyNotify key_destroy_func,
		   GDestroyNotify val_destroy_func)
{
	HgBTree *retval;

	retval = g_new(HgBTree, 1);
	retval->page_size = page_size;
	retval->root = NULL;
	retval->key_destroy_func = key_destroy_func;
	retval->val_destroy_func = val_destroy_func;

	return retval;
}

void
_hg_btree_destroy(HgBTree *tree)
{
	g_return_if_fail (tree != NULL);

	hg_btree_page_destroy(tree, tree->root);
	g_free(tree);
}

void
_hg_btree_add(HgBTree *tree,
	      gpointer key,
	      gpointer val)
{
	gboolean inserted;
	HgBTreePage *page, *newpage = NULL;
	gpointer pkey, pval;

	g_return_if_fail (tree != NULL);

	pkey = key;
	pval = val;
	inserted = hg_btree_page_insert(tree, tree->root, &pkey, &pval, FALSE, &newpage);
	if (!inserted) {
		page = hg_btree_page_new(tree);
		page->n_data = 1;
		page->key[0] = pkey;
		page->val[0] = pval;
		page->page[0] = tree->root;
		page->page[1] = newpage;
		tree->root = page;
	}
}

void
_hg_btree_replace(HgBTree *tree,
		  gpointer key,
		  gpointer val)
{
	gboolean inserted;
	HgBTreePage *page, *newpage = NULL;
	gpointer pkey, pval;

	g_return_if_fail (tree != NULL);

	pkey = key;
	pval = val;
	inserted = hg_btree_page_insert(tree, tree->root, &pkey, &pval, TRUE, &newpage);
	if (!inserted) {
		page = hg_btree_page_new(tree);
		page->n_data = 1;
		page->key[0] = pkey;
		page->val[0] = pval;
		page->page[0] = tree->root;
		page->page[1] = newpage;
		tree->root = page;
	}
}

void
_hg_btree_remove(HgBTree *tree,
		 gpointer key)
{
	HgBTreePage *page;
	gboolean removed, need_restore = FALSE;
	gpointer pkey, pval = NULL;

	g_return_if_fail (tree != NULL);

	pkey = key;
	removed = hg_btree_page_remove(tree, tree->root, &pkey, &pval, &need_restore);
	if (removed) {
		if (tree->root->n_data == 0) {
			page = tree->root;
			tree->root = tree->root->page[0];
			hg_btree_page_free(page);
		}
	}
}

gpointer
_hg_btree_find(HgBTree *tree,
	       gpointer key)
{
	HgBTreePage *page;
	guint i;

	g_return_val_if_fail (tree != NULL, NULL);

	page = tree->root;
	while (page != NULL) {
		if (page->key[page->n_data - 1] >= key) {
			for (i = 0; i < page->n_data && page->key[i] < key; i++);
			if (i < page->n_data && page->key[i] == key)
				return page->val[i];
		} else {
			i = page->n_data;
		}
		page = page->page[i];
	}

	return NULL;
}

gpointer
_hg_btree_find_near(HgBTree *tree,
		    gpointer key)
{
	HgBTreePage *page, *prev = NULL;
	guint i = 0, prev_pos = 0;

	g_return_val_if_fail (tree != NULL, NULL);

	page = tree->root;
	while (page != NULL) {
		if (page->key[page->n_data - 1] >= key) {
			for (i = 0; i < page->n_data && page->key[i] < key; i++);
			if (i < page->n_data && page->key[i] == key)
				return page->val[i];
			prev = page;
			prev_pos = i;
		} else {
			i = page->n_data;
		}
		page = page->page[i];
	}
	if (prev) {
		if (prev_pos < prev->n_data) {
			/* prev->val[i - 1] should be less than key */
			return prev->val[prev_pos];
		}
		/* no items is bigger than key found. */
	}

	return NULL;
}

void
_hg_btree_foreach(HgBTree        *tree,
		  HgTraverseFunc  func,
		  gpointer        data)
{
	g_return_if_fail (tree != NULL);
	g_return_if_fail (func != NULL);

	hg_btree_page_foreach(tree->root, func, data);
}

HgBTreeIter
_hg_btree_iter_new(void)
{
	HgBTreeIter iter = g_new(struct _HieroGlyphBTreeIter, 1);

	iter->id = NULL;
	iter->stamp = NULL;
	iter->seq = 0;
	iter->key = NULL;
	iter->val = NULL;

	return iter;
}

gboolean
_hg_btree_get_iter_first(HgBTree     *tree,
			 HgBTreeIter  iter)
{
	guint sequence = 0;

	g_return_val_if_fail (tree != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	iter->id = tree;
	iter->stamp = tree->root;
	iter->seq = 0;

	return hg_btree_page_get_iter(tree->root, &sequence, iter, TRUE);
}

gboolean
_hg_btree_get_iter_next(HgBTree     *tree,
			HgBTreeIter  iter)
{
	guint sequence = 0;

	g_return_val_if_fail (tree != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->id == tree, FALSE);
	g_return_val_if_fail (iter->stamp == tree->root, FALSE);

	return hg_btree_page_get_iter(tree->root, &sequence, iter, TRUE);
}

gboolean
_hg_btree_is_iter_valid(HgBTree     *tree,
			HgBTreeIter  iter)
{
	g_return_val_if_fail (tree != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->id == tree, FALSE);

	return iter->stamp == tree->root;
}

gboolean
_hg_btree_update_iter(HgBTree     *tree,
		      HgBTreeIter  iter)
{
	gboolean retval;
	guint sequence = 0;

	g_return_val_if_fail (tree != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->id == tree, FALSE);

	retval = hg_btree_page_get_iter(tree->root, &sequence, iter, FALSE);
	if (retval) {
		iter->seq++;
		iter->stamp = tree->root;
	}

	return retval;
}

guint
_hg_btree_length(HgBTree *tree)
{
	guint retval = 0;

	hg_btree_foreach(tree, _hg_btree_count_traverse, &retval);

	return retval;
}
