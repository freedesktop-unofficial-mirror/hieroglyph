/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbtree.c
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

#include "hgbtree.h"
#include "hgmem.h"


static void         _hg_btree_page_real_free          (gpointer           data);
static void         _hg_btree_page_real_set_flags     (gpointer           data,
						       guint              flags);
static void         _hg_btree_page_real_relocate      (gpointer           data,
						       HgMemRelocateInfo *info);
static void         _hg_btree_page_destroy            (HgBTreePage       *page);
static void         _hg_btree_real_free               (gpointer           data);
static void         _hg_btree_real_set_flags          (gpointer           data,
						       guint              flags);
static void         _hg_btree_real_relocate           (gpointer           data,
						       HgMemRelocateInfo *info);
static HgBTreePage *hg_btree_page_new                 (HgBTree           *tree);
static gboolean     hg_btree_page_insert              (HgBTreePage       *page,
						       gpointer          *key,
						       gpointer          *val,
						       gboolean           replace,
						       HgBTreePage       **newpage);
static void         hg_btree_page_insert_data         (HgBTreePage       *page,
						       gpointer          *key,
						       gpointer          *val,
						       guint16            pos,
						       HgBTreePage       **newpage);
static void         hg_btree_page_blance              (HgBTreePage       *page,
						       gpointer          *key,
						       gpointer          *val,
						       guint16            pos,
						       HgBTreePage       **newpage);
static gboolean     hg_btree_page_remove              (HgBTreePage       *page,
						       gpointer          *key,
						       gpointer          *val,
						       gboolean          *need_restore);
static gboolean     hg_btree_page_remove_data         (HgBTreePage       *page,
						       guint16            pos);
static gboolean     hg_btree_page_restore             (HgBTreePage       *page,
						       guint16            pos);
static void         hg_btree_page_restore_right_blance(HgBTreePage       *page,
						       guint16            pos);
static void         hg_btree_page_restore_left_blance (HgBTreePage       *page,
						       guint16            pos);
static gboolean     hg_btree_page_combine             (HgBTreePage       *page,
						       guint16            pos);


static HgObjectVTable __hg_btree_page_vtable = {
	.free      = _hg_btree_page_real_free,
	.set_flags = _hg_btree_page_real_set_flags,
	.relocate  = _hg_btree_page_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};
static HgObjectVTable __hg_btree_vtable = {
	.free      = _hg_btree_real_free,
	.set_flags = _hg_btree_real_set_flags,
	.relocate  = _hg_btree_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};


/*
 * Private Functions
 */
static void
_hg_btree_page_real_free(gpointer data)
{
	HgBTreePage *page = data;
	guint16 i;

	for (i = 0; i < page->n_data; i++) {
		/* no need to destroy page->page[i] here. it will be destroyed
		 * by GC sweeper.
		 */
		if (page->parent->key_destroy_func)
			page->parent->key_destroy_func(page->key[i]);
		if (page->parent->val_destroy_func)
			page->parent->val_destroy_func(page->val[i]);
	}
	/* no need to destroy page->page[page->n_data] here. it will be
	 * destroyed by GC sweeper.
	 */
	hg_mem_free(page->key);
	hg_mem_free(page->val);
	hg_mem_free(page->page);
	page->n_data = 0;
	page->key = NULL;
	page->val = NULL;
	page->page = NULL;
}

static void
_hg_btree_page_real_set_flags(gpointer data,
			      guint    flags)
{
	HgBTreePage *page = data;
	guint16 i;
	HgMemObject *obj;

	if (page->key) {
		hg_mem_get_object__inline(page->key, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: HgBTreePage key",
				  page->key);
		} else {
			hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	if (page->val) {
		hg_mem_get_object__inline(page->val, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: HgBTreePage val",
				  page->val);
		} else {
			hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	if (page->page == NULL && page->n_data > 0) {
		g_warning("[BUG] HgBTree structure corruption. no real data, but it says there are %d item(s).",
			  page->n_data);
	}
	if (page->page) {
		hg_mem_get_object__inline(page->page, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: HgBTreePage page",
				  page->page);
		} else {
			hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	} else {
		return;
	}

	if (!page->parent->disable_marking) {
		for (i = 0; i < page->n_data; i++) {
			if (page->page[i]) {
				hg_mem_get_object__inline(page->page[i], obj);
				if (obj == NULL) {
					g_warning("[BUG] Invalid object %p to be marked: HgBTreePage page[%d]",
						  page->page[i], i);
				} else {
					hg_mem_add_flags__inline(obj, flags, TRUE);
				}
			}
			hg_mem_get_object__inline(page->val[i], obj);
			if (obj == NULL) {
				g_warning("[BUG] Invalid object %p to be marked: HgBTreePage val[%d]",
					  page->val[i], i);
			} else {
				hg_mem_add_flags__inline(obj, flags, TRUE);
			}
		}
		if (page->page[page->n_data]) {
			hg_mem_get_object__inline(page->page[page->n_data], obj);
			if (obj == NULL) {
				g_warning("[BUG] Invalid object %p to be marked: HgBTreePage page[%d]",
					  page->page[page->n_data], page->n_data);
			} else {
				hg_mem_add_flags__inline(obj, flags, TRUE);
			}
		}
	}
}

static void
_hg_btree_page_real_relocate(gpointer           data,
			     HgMemRelocateInfo *info)
{
	HgBTreePage *page = data;
	guint16 i;

	if ((gsize)page->page >= info->start &&
	    (gsize)page->page <= info->end) {
		page->page = (gpointer)((gsize)page->page + info->diff);
	}
	if ((gsize)page->key >= info->start &&
	    (gsize)page->key <= info->end) {
		page->key = (gpointer)((gsize)page->key + info->diff);
	}
	if ((gsize)page->val >= info->start &&
	    (gsize)page->val <= info->end) {
		page->val = (gpointer)((gsize)page->val + info->diff);
	}
	if (!page->parent->disable_marking) {
		for (i = 0; i < page->n_data; i++) {
			if (page->page[i]) {
				if ((gsize)page->page[i] >= info->start &&
				    (gsize)page->page[i] <= info->end) {
					page->page[i] = (gpointer)((gsize)page->page[i] + info->diff);
				}
			}
			if ((gsize)page->val[i] >= info->start &&
			    (gsize)page->val[i] <= info->end) {
				page->val[i] = (gpointer)((gsize)page->val[i] + info->diff);
			}
		}
		if (page->page[page->n_data]) {
			if ((gsize)page->page[page->n_data] >= info->start &&
			    (gsize)page->page[page->n_data] <= info->end) {
				page->page[page->n_data] = (gpointer)((gsize)page->page[page->n_data] + info->diff);
			}
		}
	}
}

static void
_hg_btree_page_destroy(HgBTreePage *page)
{
	if (page != NULL) {
		guint16 i;

		if (page->n_data == 0 &&
		    page->key == NULL &&
		    page->val == NULL &&
		    page->page == NULL) {
			/* this may be already freed */
		} else {
			for (i = 0; i < page->n_data; i++) {
				_hg_btree_page_destroy(page->page[i]);
			}
			_hg_btree_page_destroy(page->page[page->n_data]);
			hg_mem_free(page);
		}
	}
}

static void
_hg_btree_real_free(gpointer data)
{
	HgBTree *tree = data;

	_hg_btree_page_destroy(tree->root);
}

static void
_hg_btree_real_set_flags(gpointer data,
			 guint    flags)
{
	HgBTree *tree = data;
	HgMemObject *obj;

	if (tree->root) {
		hg_mem_get_object__inline(tree->root, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: HgBTree", tree->root);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_btree_real_relocate(gpointer           data,
			HgMemRelocateInfo *info)
{
	HgBTree *tree = data;

	if ((gsize)tree->root >= info->start &&
	    (gsize)tree->root <= info->end) {
		tree->root = (gpointer)((gsize)tree->root + info->diff);
	}
}

static HgBTreePage *
hg_btree_page_new(HgBTree *tree)
{
	HgBTreePage *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(tree, obj);
	/* HgBTree's free method will takes care of this object age.
	 * HG_FL_LOCK is necessary to avoid destroying object at GC
	 */
	retval = hg_mem_alloc_with_flags(obj->pool, sizeof (HgBTreePage),
					 HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_LOCK | HG_FL_HGOBJECT);
	if (retval == NULL)
		return NULL;
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(obj->pool));
	hg_object_set_vtable(&retval->object, &__hg_btree_page_vtable);

	retval->parent = tree;
	/* it may be entered into GC during allocating memory.
	 * need to initialize with NULL to avoid corruption.
	 */
	retval->key = NULL;
	retval->val = NULL;
	retval->page = NULL;
	retval->key = hg_mem_alloc_with_flags(obj->pool,
					      sizeof (gpointer) * tree->page_size * 2,
					      HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_LOCK);
	retval->val = hg_mem_alloc_with_flags(obj->pool,
					      sizeof (gpointer) * tree->page_size * 2,
					      HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_LOCK);
	retval->page = hg_mem_alloc_with_flags(obj->pool,
					       sizeof (gpointer) * (tree->page_size * 2 + 1),
					       HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_LOCK);
	if (retval->key == NULL ||
	    retval->val == NULL ||
	    retval->page == NULL) {
		return NULL;
	}

	return retval;
}

static gboolean
hg_btree_page_insert(HgBTreePage  *page,
		     gpointer     *key,
		     gpointer     *val,
		     gboolean      replace,
		     HgBTreePage **newpage)
{
	guint16 i;
	gboolean retval;

	if (page == NULL) {
		*newpage = NULL;
		return FALSE;
	}
	if (page->key[page->n_data - 1] >= *key) {
		for (i = 0; i < page->n_data && page->key[i] < *key; i++);
		if (i < page->n_data && page->key[i] == *key) {
			if (replace) {
				if (page->parent->val_destroy_func)
					page->parent->val_destroy_func(page->val[i]);
				page->key[i] = *key;
				page->val[i] = *val;
			}
			return TRUE;
		}
	} else {
		i = page->n_data;
	}
	retval = hg_btree_page_insert(page->page[i], key, val, replace, newpage);
	if (!retval) {
		if (page->n_data < (page->parent->page_size * 2)) {
			hg_btree_page_insert_data(page, key, val, i, newpage);
			retval = TRUE;
		} else {
			hg_btree_page_blance(page, key, val, i, newpage);
			retval = FALSE;
		}
	}
	return retval;
}

static void
hg_btree_page_insert_data(HgBTreePage  *page,
			  gpointer     *key,
			  gpointer     *val,
			  guint16       pos,
			  HgBTreePage **newpage)
{
	guint16 i;

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
hg_btree_page_blance(HgBTreePage  *page,
		     gpointer     *key,
		     gpointer     *val,
		     guint16       pos,
		     HgBTreePage **newpage)
{
	guint16 i, size, page_size = page->parent->page_size;
	HgBTreePage *new;

	if (pos <= page_size)
		size = page_size;
	else
		size = page_size + 1;
	new = hg_btree_page_new(page->parent);
	if (new == NULL) {
		g_warning("Failed to allocate a memory.");
		return;
	}
	for (i = size + 1; i <= page_size * 2; i++) {
		new->key[i - size - 1] = page->key[i - 1];
		new->val[i - size - 1] = page->val[i - 1];
		new->page[i - size] = page->page[i];
	}
	new->n_data = page_size * 2 - size;
	page->n_data = size;
	if (pos <= page_size)
		hg_btree_page_insert_data(page, key, val, pos, newpage);
	else
		hg_btree_page_insert_data(new, key, val, pos - size, newpage);
	*key = page->key[page->n_data - 1];
	*val = page->val[page->n_data - 1];
	new->page[0] = page->page[page->n_data];
	page->n_data--;
	*newpage = new;
}

static gboolean
hg_btree_page_remove(HgBTreePage *page,
		     gpointer    *key,
		     gpointer    *val,
		     gboolean    *need_restore)
{
	HgBTreePage *p;
	guint16 i;
	gboolean removed = FALSE;

	if (page == NULL)
		return FALSE;
	if (page->key[page->n_data - 1] >= *key)
		for (i = 0; i < page->n_data && page->key[i] < *key; i++);
	else
		i = page->n_data;
	if (i < page->n_data && page->key[i] == *key) {
		removed = TRUE;
		if (page->parent->key_destroy_func)
			page->parent->key_destroy_func(page->key[i]);
		if (page->parent->val_destroy_func)
			page->parent->val_destroy_func(page->val[i]);
		if ((p = page->page[i + 1]) != NULL) {
			while (p->page[0] != NULL)
				p = p->page[0];
			page->key[i] = *key = p->key[0];
			page->val[i] = *val = p->val[0];
			removed = hg_btree_page_remove(page->page[i + 1], key, val, need_restore);
			if (*need_restore)
				*need_restore = hg_btree_page_restore(page, i + 1);
		} else {
			*need_restore = hg_btree_page_remove_data(page, i);
		}
	} else {
		removed = hg_btree_page_remove(page->page[i], key, val, need_restore);
		if (*need_restore)
			*need_restore = hg_btree_page_restore(page, i);
	}

	return removed;
}

static gboolean
hg_btree_page_remove_data(HgBTreePage *page,
			  guint16      pos)
{
	while (++pos < page->n_data) {
		page->key[pos - 1] = page->key[pos];
		page->val[pos - 1] = page->val[pos];
		page->page[pos] = page->page[pos + 1];
	}

	return --(page->n_data) < page->parent->page_size;
}

static gboolean
hg_btree_page_restore(HgBTreePage *page,
		      guint16      pos)
{
	guint16 page_size = page->parent->page_size;

	if (pos > 0) {
		if (page->page[pos - 1]->n_data > page_size)
			hg_btree_page_restore_right_blance(page, pos);
		else
			return hg_btree_page_combine(page, pos);
	} else {
		if (page->page[1]->n_data > page_size)
			hg_btree_page_restore_left_blance(page, 1);
		else
			return hg_btree_page_combine(page, 1);
	}

	return FALSE;
}

static void
hg_btree_page_restore_right_blance(HgBTreePage *page,
				   guint16      pos)
{
	guint16 i;
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
hg_btree_page_restore_left_blance(HgBTreePage *page,
				  guint16      pos)
{
	guint16 i;
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
hg_btree_page_combine(HgBTreePage *page,
		      guint16      pos)
{
	guint16 i;
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
	need_restore = hg_btree_page_remove_data(page, pos - 1);
	/* 'right' page could be freed here though, it might be referred from
	 * somewhere. so the actual destroying the page relies on GC.
	 */

	return need_restore;
}

static void
hg_btree_page_foreach(HgBTreePage    *page,
		      HgTraverseFunc  func,
		      gpointer        data)
{
	if (page != NULL) {
		guint16 i;

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
		guint16 i;

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
hg_btree_new(HgMemPool *pool,
	     guint16    page_size)
{
	return hg_btree_new_full(pool, page_size, NULL, NULL);
}

HgBTree *
hg_btree_new_full(HgMemPool      *pool,
		  guint16         page_size,
		  GDestroyNotify  key_destroy_func,
		  GDestroyNotify  val_destroy_func)
{
	HgBTree *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (page_size <= G_MAXINT16, NULL);

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgBTree),
					 HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_HGOBJECT);
	if (retval == NULL)
		return NULL;
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_btree_vtable);

	retval->page_size = page_size;
	retval->root = NULL;
	retval->key_destroy_func = key_destroy_func;
	retval->val_destroy_func = val_destroy_func;
	retval->disable_marking = FALSE;

	return retval;
}

void
hg_btree_add(HgBTree *tree,
	     gpointer key,
	     gpointer val)
{
	gboolean inserted;
	HgBTreePage *page, *newpage = NULL;
	gpointer pkey, pval;

	g_return_if_fail (tree != NULL);

	pkey = key;
	pval = val;
	inserted = hg_btree_page_insert(tree->root, &pkey, &pval, FALSE, &newpage);
	if (!inserted) {
		page = hg_btree_page_new(tree);
		if (page == NULL) {
			g_warning("Failed to allocate a tree page.");
			return;
		}
		page->n_data = 1;
		page->key[0] = pkey;
		page->val[0] = pval;
		page->page[0] = tree->root;
		page->page[1] = newpage;
		tree->root = page;
	}
}

void
hg_btree_replace(HgBTree *tree,
		 gpointer key,
		 gpointer val)
{
	gboolean inserted;
	HgBTreePage *page, *newpage = NULL;
	gpointer pkey, pval;

	g_return_if_fail (tree != NULL);

	pkey = key;
	pval = val;
	inserted = hg_btree_page_insert(tree->root, &pkey, &pval, TRUE, &newpage);
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
hg_btree_remove(HgBTree *tree,
		gpointer key)
{
	HgBTreePage *page;
	gboolean removed, need_restore = FALSE;
	gpointer pkey, pval = NULL;

	g_return_if_fail (tree != NULL);

	pkey = key;
	removed = hg_btree_page_remove(tree->root, &pkey, &pval, &need_restore);
	if (removed) {
		if (tree->root->n_data == 0) {
			page = tree->root;
			tree->root = tree->root->page[0];
			/* 'page' page could be freed here though, it might be
			 * referred from somewhere. so the actual destroying
			 * the page relies on GC.
			 */
		}
	}
}

gpointer
hg_btree_find(HgBTree *tree,
	      gpointer key)
{
	HgBTreePage *page;

	g_return_val_if_fail (tree != NULL, NULL);

	page = tree->root;
	while (page != NULL) {
		guint16 i;

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
hg_btree_find_near(HgBTree *tree,
		   gpointer key)
{
	HgBTreePage *page, *prev = NULL;
	guint16 i = 0, prev_pos = 0;

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
hg_btree_foreach(HgBTree        *tree,
		 HgTraverseFunc  func,
		 gpointer        data)
{
	g_return_if_fail (tree != NULL);
	g_return_if_fail (func != NULL);

	hg_btree_page_foreach(tree->root, func, data);
}

HgBTreeIter
hg_btree_iter_new(void)
{
	HgBTreeIter iter = g_new(struct _HieroGlyphBTreeIter, 1);

	iter->id = NULL;
	iter->stamp = NULL;
	iter->seq = 0;
	iter->key = NULL;
	iter->val = NULL;

	return iter;
}

void
hg_btree_iter_free(HgBTreeIter iter)
{
	g_free(iter);
}

gboolean
hg_btree_get_iter_first(HgBTree     *tree,
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
hg_btree_get_iter_next(HgBTree     *tree,
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
hg_btree_is_iter_valid(HgBTree     *tree,
		       HgBTreeIter  iter)
{
	g_return_val_if_fail (tree != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->id == tree, FALSE);

	return iter->stamp == tree->root;
}

gboolean
hg_btree_update_iter(HgBTree     *tree,
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
hg_btree_length(HgBTree *tree)
{
	guint retval = 0;

	hg_btree_foreach(tree, _hg_btree_count_traverse, &retval);

	return retval;
}

void
hg_btree_allow_marking(HgBTree  *tree,
		       gboolean  flag)
{
	g_return_if_fail (tree != NULL);

	tree->disable_marking = (flag ? FALSE : TRUE);
}
