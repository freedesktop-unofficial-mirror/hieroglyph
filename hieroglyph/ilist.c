/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * ilist.c
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

#include "ilist.h"

/*
 * Most code are duplicate from HgList.
 * big difference are whether HgMemPool is used or not.
 * this is for internal, where can't depends on it.
 */
enum {
	HG_LIST_MASK_UNUSED      = 1 << 0,
	HG_LIST_MASK_LAST_NODE   = 1 << 1,
	HG_LIST_MASK_OBJECT_NODE = 1 << 2,
};

struct _HieroGlyphList {
	HgList   *prev;
	HgList   *next;
	gpointer  data;
	guint     state;
};

struct _HieroGlyphListIter {
	HgList   *top;
	HgList   *last;
	HgList   *current;
};

#define hg_list_next(_list)	((_list)->next)
#define hg_list_previous(_list)	((_list)->prev)

#define HG_LIST_GET_USER_DATA(_obj, _mask)	\
	((_obj)->state & _mask)
#define HG_LIST_SET_USER_DATA(_obj, _mask, _flag)			\
	if (_flag) {							\
		((_obj)->state = (HG_LIST_GET_USER_DATA (_obj, 0xff & ~(_mask)) | \
				  _mask));				\
	} else {							\
		((_obj)->state = (HG_LIST_GET_USER_DATA (_obj, 0xff & ~(_mask)) & \
				  ~(_mask)));				\
	}
#define HG_LIST_SET_UNUSED(_obj, _flag)					\
	HG_LIST_SET_USER_DATA (_obj, HG_LIST_MASK_UNUSED, _flag)
#define HG_LIST_IS_UNUSED(_obj)				\
	(HG_LIST_GET_USER_DATA (_obj, HG_LIST_MASK_UNUSED) != 0)
#define HG_LIST_SET_LAST_NODE(_obj, _flag)				\
	HG_LIST_SET_USER_DATA (_obj, HG_LIST_MASK_LAST_NODE, _flag)
#define HG_LIST_IS_LAST_NODE(_obj)				\
	(HG_LIST_GET_USER_DATA (_obj, HG_LIST_MASK_LAST_NODE) != 0)
#define HG_LIST_SET_OBJECT_NODE(_obj, _flag)				\
	HG_LIST_SET_USER_DATA (_obj, HG_LIST_MASK_OBJECT_NODE, _flag)
#define HG_LIST_IS_OBJECT_NODE(_obj)					\
	(HG_LIST_GET_USER_DATA (_obj, HG_LIST_MASK_OBJECT_NODE) != 0)


/*
 * Private Functions
 */
static HgList *
_hg_list_get_last_node(HgList *list)
{
	HgList *tmp = list;

	g_return_val_if_fail (list != NULL, NULL);

	/* assume that the initial position may be the top node */
	while (tmp) {
		if (HG_LIST_IS_UNUSED (tmp)) {
			/* validate node */
			if (hg_list_next(tmp) != NULL ||
			    hg_list_previous(tmp) != NULL) {
				/* found incomplete node */
				g_warning("[BUG] incomplete HgList node %p found.", tmp);
			} else {
				/* this can be used as the last node */
				return tmp;
			}
		} else {
			if (HG_LIST_IS_LAST_NODE (tmp)) {
				return tmp;
			}
		}
		tmp = hg_list_previous(tmp);
		/* validate node */
		if (tmp == list) {
			/* detected the circular reference */
			g_warning("[BUG] Circular reference happened without the last node mark: %p",
				  list);
			/* workaround */
			return hg_list_previous(list);
		}
	}

	return NULL;
}

static HgList *
_hg_list_get_top_node(HgList *list)
{
	HgList *retval;

	g_return_val_if_fail (list != NULL, NULL);

	retval = _hg_list_get_last_node(list);
	if (retval != NULL) {
		retval = hg_list_next(retval);
	}

	return retval;
}

HgList *
_hg_list_real_append(HgList   *list,
		     gpointer  data,
		     gboolean  is_object)
{
	HgList *tmp, *last, *top;

	g_return_val_if_fail (list != NULL, NULL);

	if ((last = _hg_list_get_last_node(list)) != NULL) {
		if (!HG_LIST_IS_UNUSED (last)) {
			tmp = hg_list_new();
			top = hg_list_next(last);
			hg_list_next(tmp) = top;
			hg_list_next(last) = tmp;
			hg_list_previous(top) = tmp;
			hg_list_previous(tmp) = last;
			HG_LIST_SET_LAST_NODE (last, FALSE);
			list = top;
		} else {
			tmp = list;
			hg_list_next(tmp) = tmp;
			hg_list_previous(tmp) = tmp;
		}
		tmp->data = data;
		HG_LIST_SET_LAST_NODE (tmp, TRUE);
		HG_LIST_SET_UNUSED (tmp, FALSE);
		HG_LIST_SET_OBJECT_NODE (tmp, is_object);
	}

	return list;
}

HgList *
_hg_list_real_prepend(HgList   *list,
		      gpointer  data,
		      gboolean  is_object)
{
	HgList *tmp, *last, *top;

	g_return_val_if_fail (list != NULL, NULL);

	if ((top = _hg_list_get_top_node(list)) != NULL) {
		if (!HG_LIST_IS_UNUSED (top)) {
			tmp = hg_list_new();
			last = hg_list_previous(top);
			hg_list_next(last) = tmp;
			hg_list_next(tmp) = top;
			hg_list_previous(top) = tmp;
			hg_list_previous(tmp) = last;
			list = tmp;
		} else {
			tmp = list;
			hg_list_next(tmp) = tmp;
			hg_list_previous(tmp) = tmp;
		}
		tmp->data = data;
		HG_LIST_SET_UNUSED (tmp, FALSE);
		HG_LIST_SET_OBJECT_NODE (tmp, is_object);
	}

	return list;
}

static void
_hg_list_iter_real_set_data(HgListIter iter,
			    gpointer   data,
			    gboolean   is_object)
{
	g_return_if_fail (iter != NULL);

	iter->current->data = data;
	HG_LIST_SET_OBJECT_NODE (iter->current, is_object);
}

/*
 * Public Functions
 */
HgList *
_hg_list_new()
{
	HgList *retval;

	retval = g_new(HgList, 1);
	if (retval == NULL)
		return NULL;

	HG_LIST_SET_UNUSED (retval, TRUE);
	hg_list_next(retval) = NULL;
	hg_list_previous(retval) = NULL;
	retval->data = NULL;

	return retval;
}

void
_hg_list_free(HgList *list)
{
	HgList *tmp;

	/* break the loop to detect the end of list. */
	if (hg_list_previous(list))
		hg_list_next(hg_list_previous(list)) = NULL;
	while (list) {
		tmp = hg_list_next(list);
		g_free(list);
		list = tmp;
	}
}

HgList *
_hg_list_append(HgList   *list,
		gpointer  data)
{
	return _hg_list_real_append(list, data, FALSE);
}

HgList *
_hg_list_prepend(HgList   *list,
		 gpointer  data)
{
	return _hg_list_real_prepend(list, data, FALSE);
}

guint
_hg_list_length(HgList *list)
{
	guint retval = 0;
	HgList *l = list;

	g_return_val_if_fail (list != NULL, 0);

	do {
		l = hg_list_next(list);
		retval++;
	} while (l && l != list);

	/* validate node */
	if (l == NULL) {
		g_warning("[BUG] no loop detected in HgList %p", list);
	}

	return retval;
}

HgList *
_hg_list_remove(HgList   *list,
		gpointer  data)
{
	HgList *l = list, *top = NULL, *prev, *next;

	g_return_val_if_fail (list != NULL, NULL);

	do {
		if (top == NULL) {
			if (HG_LIST_IS_LAST_NODE (l)) {
				top = hg_list_next(l);
			} else {
				prev = hg_list_previous(l);
				if (prev && HG_LIST_IS_LAST_NODE (prev)) {
					top = l;
				}
			}
		}
		if (l->data == data) {
			prev = hg_list_previous(l);
			next = hg_list_next(l);
			hg_list_next(prev) = next;
			hg_list_previous(next) = prev;
			if (HG_LIST_IS_LAST_NODE (l)) {
				HG_LIST_SET_LAST_NODE (l, FALSE);
				HG_LIST_SET_LAST_NODE (prev, TRUE);
				top = next;
			}
			HG_LIST_SET_UNUSED (l, TRUE);
			hg_list_next(l) = NULL;
			hg_list_previous(l) = NULL;
			/* relying on GC to be really freed. */
			break;
		}
		l = hg_list_next(l);
	} while (l && l != list);

	/* validate node */
	if (l == NULL) {
		g_warning("[BUG] no loop detected in HgList %p during removing %p",
			  list, data);
		top = list;
	} else {
		if (!top)
			top = _hg_list_get_top_node(l);
	}

	return top;
}

HgList *
_hg_list_first(HgList *list)
{
	return _hg_list_get_top_node(list);
}

HgList *
_hg_list_last(HgList *list)
{
	return _hg_list_get_last_node(list);
}

/* iterators */
HgListIter
_hg_list_iter_new(HgList *list)
{
	HgListIter iter;

	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (hg_list_previous(list) != NULL, NULL);
	g_return_val_if_fail (HG_LIST_IS_LAST_NODE (hg_list_previous(list)), NULL);

	iter = g_new(struct _HieroGlyphListIter, 1);
	if (iter == NULL)
		return NULL;

	iter->top = list;
	iter->last = hg_list_previous(list);
	iter->current = list;

	return iter;
}

gboolean
_hg_list_get_iter_first(HgList     *list,
			HgListIter  iter)
{
	g_return_val_if_fail (list != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->top == list, FALSE);
	g_return_val_if_fail (iter->last == hg_list_previous(list), FALSE);

	iter->current = iter->top;

	return TRUE;
}

gboolean
_hg_list_get_iter_next(HgList     *list,
		       HgListIter  iter)
{
	g_return_val_if_fail (list != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->top == list, FALSE);
	g_return_val_if_fail (iter->last == hg_list_previous(list), FALSE);

	iter->current = hg_list_next(iter->current);
	if (iter->current == iter->top)
		return FALSE;

	return TRUE;
}

gpointer
_hg_list_iter_get_data(HgListIter iter)
{
	g_return_val_if_fail (iter != NULL, NULL);

	return iter->current->data;
}

void
_hg_list_iter_set_data(HgListIter iter,
		       gpointer   data)
{
	_hg_list_iter_real_set_data(iter, data, FALSE);
}

HgList *
_hg_list_iter_delete_link(HgListIter iter)
{
	HgList *list, *next, *prev;

	g_return_val_if_fail (iter != NULL, NULL);

	prev = hg_list_previous(iter->current);
	next = hg_list_next(iter->current);
	hg_list_next(prev) = next;
	hg_list_previous(next) = prev;

	if (HG_LIST_IS_LAST_NODE (iter->current)) {
		HG_LIST_SET_LAST_NODE (iter->current, FALSE);
		HG_LIST_SET_LAST_NODE (prev, TRUE);
		HG_LIST_SET_UNUSED (iter->current, TRUE);
	}
	hg_list_next(iter->current) = NULL;
	hg_list_previous(iter->current) = NULL;
	if (iter->current == next) {
		hg_list_free(iter->current);
		iter->top = NULL;
	} else if (iter->current == iter->top) {
		iter->top = next;
	}
	list = iter->top;
	iter->current = prev;

	return list;
}

HgListIter
_hg_list_find_iter(HgList        *list,
		   gconstpointer  data)
{
	gpointer p;
	HgListIter iter;

	g_return_val_if_fail (list != NULL, NULL);

	iter = hg_list_iter_new(list);
	while (iter) {
		p = hg_list_iter_get_data(iter);
		if (p == data)
			return iter;
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	if (iter)
		hg_list_iter_free(iter);

	return NULL;
}

HgListIter
_hg_list_find_iter_custom(HgList        *list,
			  gconstpointer  data,
			  HgCompareFunc  func)
{
	gpointer p;
	HgListIter iter;

	g_return_val_if_fail (list != NULL, NULL);

	iter = hg_list_iter_new(list);
	while (iter) {
		p = hg_list_iter_get_data(iter);
		if (func(p, data))
			return iter;
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	if (iter)
		hg_list_iter_free(iter);

	return NULL;
}
