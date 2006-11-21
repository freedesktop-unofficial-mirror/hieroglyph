/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglist.c
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

#include "hglist.h"
#include "hglog.h"
#include "hgmem.h"
#include "hgallocator-bfit.h"


#define HG_LIST_POOL_SIZE 16384

enum {
	HG_LIST_MASK_UNUSED      = 1 << 0,
	HG_LIST_MASK_LAST_NODE   = 1 << 1,
	HG_LIST_MASK_OBJECT_NODE = 1 << 2,
};

struct _HieroGlyphList {
	HgObject  object;
	HgList   *prev;
	HgList   *next;
	gpointer  data;
};

struct _HieroGlyphListIter {
	HgObject  object;
	HgList   *top;
	HgList   *last;
	HgList   *current;
};

#define hg_list_next(_list)	((_list)->next)
#define hg_list_previous(_list)	((_list)->prev)

#define HG_LIST_GET_USER_DATA(_obj, _mask)		\
	(HG_OBJECT_GET_USER_DATA ((HgObject *)(_obj)) & _mask)
#define HG_LIST_SET_USER_DATA(_obj, _mask, _flag)			\
	if (_flag) {							\
		HG_OBJECT_SET_USER_DATA ((HgObject *)(_obj),		\
					 (HG_LIST_GET_USER_DATA (_obj, 0xff & ~(_mask)) | \
					  _mask));			\
	} else {							\
		HG_OBJECT_SET_USER_DATA ((HgObject *)(_obj),		\
					 (HG_LIST_GET_USER_DATA (_obj, 0xff & ~(_mask)) & \
					  ~(_mask)));			\
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


static void _hg_list_real_set_flags     (gpointer data,
					 guint    flags);
static void _hg_list_real_relocate      (gpointer           data,
					 HgMemRelocateInfo *info);
static void _hg_list_iter_real_set_flags(gpointer data,
					 guint    flags);
static void _hg_list_iter_real_relocate (gpointer           data,
					 HgMemRelocateInfo *info);


static HgObjectVTable __hg_list_vtable = {
	.free      = NULL,
	.set_flags = _hg_list_real_set_flags,
	.relocate  = _hg_list_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};
static HgObjectVTable __hg_list_iter_vtable = {
	.free      = NULL,
	.set_flags = _hg_list_iter_real_set_flags,
	.relocate  = _hg_list_iter_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};
static HgAllocator *__hg_list_allocator = NULL;
static HgMemPool *__hg_list_pool = NULL;
static gboolean __hg_list_initialized = FALSE;

/*
 * Private Functions
 */
static void
_hg_list_real_set_flags(gpointer data,
			guint    flags)
{
	HgList *list = data, *tmp;
	HgMemObject *obj;

	if (list->data && HG_LIST_IS_OBJECT_NODE (list)) {
		hg_mem_get_object__inline(list->data, obj);
		if (obj == NULL) {
			hg_log_warning("[BUG] Invalid object %p to be marked: HgList data",
				       list->data);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	if ((tmp = hg_list_next(list)) != NULL) {
		hg_mem_get_object__inline(tmp, obj);
		if (obj == NULL) {
			hg_log_warning("[BUG] Invalid object %p to be marked: HgList next node", tmp);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	if ((tmp = hg_list_previous(list)) != NULL) {
		hg_mem_get_object__inline(tmp, obj);
		if (obj == NULL) {
			hg_log_warning("[BUG] Invalid object %p to be marked: HgList previous node", tmp);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_list_real_relocate(gpointer           data,
		       HgMemRelocateInfo *info)
{
	HgList *list = data;

	if (list->data && HG_LIST_IS_OBJECT_NODE (list)) {
		if ((gsize)list->data >= info->start &&
		    (gsize)list->data <= info->end) {
			list->data = (gpointer)((gsize)list->data + info->diff);
		}
	}
	if (hg_list_next(list)) {
		if ((gsize)hg_list_next(list) >= info->start &&
		    (gsize)hg_list_next(list) <= info->end) {
			hg_list_next(list) = (gpointer)((gsize)hg_list_next(list) + info->diff);
		}
	}
	if (hg_list_previous(list)) {
		if ((gsize)hg_list_previous(list) >= info->start &&
		    (gsize)hg_list_previous(list) <= info->end) {
			hg_list_previous(list) = (gpointer)((gsize)hg_list_previous(list) + info->diff);
		}
	}
}

static void
_hg_list_iter_real_set_flags(gpointer data,
			     guint    flags)
{
	HgListIter iter = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(iter->top, obj);
	if (obj == NULL) {
		hg_log_warning("[BUG] Invalid object %p to be marked: HgListIter top node", iter->top);
	} else {
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
	hg_mem_get_object__inline(iter->last, obj);
	if (obj == NULL) {
		hg_log_warning("[BUG] Invalid object %p to be marked: HgListIter last node", iter->last);
	} else {
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
	hg_mem_get_object__inline(iter->current, obj);
	if (obj == NULL) {
		hg_log_warning("[BUG] Invalid object %p to be marked: HgListIter current node", iter->current);
	} else {
		if (!hg_mem_is_flags__inline(obj, flags))
			hg_mem_add_flags__inline(obj, flags, TRUE);
	}
}

static void
_hg_list_iter_real_relocate(gpointer           data,
			    HgMemRelocateInfo *info)
{
	HgListIter iter = data;

	if ((gsize)iter->top >= info->start &&
	    (gsize)iter->top <= info->end) {
		iter->top = (gpointer)((gsize)iter->top + info->diff);
	}
	if ((gsize)iter->last >= info->start &&
	    (gsize)iter->last <= info->end) {
		iter->last = (gpointer)((gsize)iter->last + info->diff);
	}
	if ((gsize)iter->current >= info->start &&
	    (gsize)iter->current <= info->end) {
		iter->current = (gpointer)((gsize)iter->current + info->diff);
	}
}

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
				hg_log_warning("[BUG] incomplete HgList node %p found.", tmp);
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
			hg_log_warning("[BUG] Circular reference happened without the last node mark: %p",
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

static HgList *
_hg_list_real_append(HgList   *list,
		     gpointer  data,
		     gboolean  is_object)
{
	HgMemObject *obj;
	HgList *tmp, *last, *top;

	g_return_val_if_fail (list != NULL, NULL);

	if ((last = _hg_list_get_last_node(list)) != NULL) {
		if (!HG_LIST_IS_UNUSED (last)) {
			hg_mem_get_object__inline(last, obj);
			if (obj == NULL) {
				hg_log_warning("[BUG] Invalid object %p is given to append a list.",
					       last);
				return NULL;
			}
			tmp = hg_list_new(obj->pool);
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

static HgList *
_hg_list_real_prepend(HgList   *list,
		      gpointer  data,
		      gboolean  is_object)
{
	HgMemObject *obj;
	HgList *tmp, *last, *top;

	g_return_val_if_fail (list != NULL, NULL);

	if ((top = _hg_list_get_top_node(list)) != NULL) {
		if (!HG_LIST_IS_UNUSED (top)) {
			hg_mem_get_object__inline(top, obj);
			if (obj == NULL) {
				hg_log_warning("[BUG] Invalid object %p is given to prepend a list.",
					       top);
				return NULL;
			}
			tmp = hg_list_new(obj->pool);
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
void
hg_list_init(void)
{
	if (!__hg_list_initialized) {
		hg_mem_init();
		__hg_list_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
		__hg_list_pool = hg_mem_pool_new(__hg_list_allocator,
						 "HgList Pool",
						 HG_LIST_POOL_SIZE, HG_MEM_GLOBAL | HG_MEM_RESIZABLE);
		hg_mem_pool_use_garbage_collection(__hg_list_pool, FALSE);
		__hg_list_initialized = TRUE;
	}
}

void
hg_list_finalize(void)
{
	if (__hg_list_initialized) {
		hg_mem_pool_destroy(__hg_list_pool);
		hg_allocator_destroy(__hg_list_allocator);
		__hg_list_initialized = FALSE;
	}
}

HgList *
hg_list_new(HgMemPool *pool)
{
	HgList *retval;

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgList),
					 HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_HGOBJECT);
	if (retval == NULL)
		return NULL;
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_list_vtable);

	HG_LIST_SET_UNUSED (&retval->object, TRUE);
	hg_list_next(retval) = NULL;
	hg_list_previous(retval) = NULL;
	retval->data = NULL;

	return retval;
}

HgList *
hg_list_append(HgList   *list,
	       gpointer  data)
{
	return _hg_list_real_append(list, data, FALSE);
}

HgList *
hg_list_append_object(HgList   *list,
		      HgObject *hobject)
{
	return _hg_list_real_append(list, hobject, TRUE);
}

HgList *
hg_list_prepend(HgList   *list,
		gpointer  data)
{
	return _hg_list_real_prepend(list, data, FALSE);
}

HgList *
hg_list_prepend_object(HgList   *list,
		       HgObject *hobject)
{
	return _hg_list_real_prepend(list, hobject, TRUE);
}

guint
hg_list_length(HgList *list)
{
	guint retval = 0;
	HgList *l = list;

	g_return_val_if_fail (list != NULL, 0);

	do {
		l = hg_list_next(l);
		retval++;
	} while (l && l != list);

	/* validate node */
	if (l == NULL) {
		hg_log_warning("[BUG] no loop detected in HgList %p", list);
	}

	return retval;
}

HgList *
hg_list_remove(HgList   *list,
	       gpointer  data)
{
	HgListIter iter;

	g_return_val_if_fail (list != NULL, NULL);

	iter = hg_list_iter_new(list);
	do {
		if (hg_list_iter_get_data(iter) == data) {
			list = hg_list_iter_delete_link(iter);
			break;
		}
	} while (hg_list_get_iter_next(list, iter));
	hg_list_iter_free(iter);

	return list;
}

HgList *
hg_list_first(HgList *list)
{
	return _hg_list_get_top_node(list);
}

HgList *
hg_list_last(HgList *list)
{
	return _hg_list_get_last_node(list);
}

/* iterators */
HgListIter
hg_list_iter_new(HgList *list)
{
	HgListIter iter;

	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (hg_list_previous(list) != NULL, NULL);
	g_return_val_if_fail (HG_LIST_IS_LAST_NODE (hg_list_previous(list)), NULL);

	if (!__hg_list_initialized)
		hg_list_init();
	iter = hg_mem_alloc_with_flags(__hg_list_pool, sizeof (struct _HieroGlyphListIter),
				       HG_FL_RESTORABLE | HG_FL_COMPLEX | HG_FL_HGOBJECT);
	if (iter == NULL)
		return NULL;
	HG_OBJECT_INIT_STATE (&iter->object);
	HG_OBJECT_SET_STATE (&iter->object, hg_mem_pool_get_default_access_mode(__hg_list_pool));
	hg_object_set_vtable(&iter->object, &__hg_list_iter_vtable);

	iter->top = list;
	iter->last = hg_list_previous(list);
	iter->current = list;

	return iter;
}

gboolean
hg_list_get_iter_first(HgList     *list,
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
hg_list_get_iter_next(HgList     *list,
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

gboolean
hg_list_get_iter_previous(HgList     *list,
			  HgListIter  iter)
{
	g_return_val_if_fail (list != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->top == list, FALSE);
	g_return_val_if_fail (iter->last == hg_list_previous(list), FALSE);

	iter->current = hg_list_previous(iter->current);
	if (iter->current == iter->last)
		return FALSE;

	return TRUE;
}

gboolean
hg_list_get_iter_last(HgList     *list,
		      HgListIter  iter)
{
	g_return_val_if_fail (list != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->top == list, FALSE);
	g_return_val_if_fail (iter->last == hg_list_previous(list), FALSE);

	iter->current = hg_list_last(iter->current);

	return TRUE;
}

gpointer
hg_list_iter_get_data(HgListIter iter)
{
	g_return_val_if_fail (iter != NULL, NULL);

	return iter->current->data;
}

void
hg_list_iter_set_data(HgListIter iter,
		      gpointer   data)
{
	_hg_list_iter_real_set_data(iter, data, FALSE);
}

void
hg_list_iter_set_object(HgListIter  iter,
			HgObject   *hobject)
{
	_hg_list_iter_real_set_data(iter, hobject, TRUE);
}

HgList *
hg_list_iter_delete_link(HgListIter iter)
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
	if (iter->current == next)
		iter->top = NULL;
	else if (iter->current == iter->top)
		iter->top = next;
	list = iter->top;
	iter->current = prev;

	return list;
}

HgListIter
hg_list_find_iter(HgList        *list,
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
hg_list_find_iter_custom(HgList        *list,
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
