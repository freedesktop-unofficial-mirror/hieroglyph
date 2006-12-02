/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include "hgstack.h"
#include "hglist.h"
#include "hglog.h"
#include "hgmem.h"
#include "hgfile.h"
#include "hgstring.h"
#include "hgvaluenode.h"


struct _HieroGlyphStack {
	HgObject  object;
	HgList   *stack;
	guint     max_depth;
	guint     current_depth;
	gboolean  use_validator;
};


static void     _hg_stack_real_free     (gpointer           data);
static void     _hg_stack_real_set_flags(gpointer           data,
					 guint              flags);
static void     _hg_stack_real_relocate (gpointer           data,
					 HgMemRelocateInfo *info);
static gpointer _hg_stack_real_dup      (gpointer           data);


static HgObjectVTable __hg_stack_vtable = {
	.free      = _hg_stack_real_free,
	.set_flags = _hg_stack_real_set_flags,
	.relocate  = _hg_stack_real_relocate,
	.dup       = _hg_stack_real_dup,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_hg_stack_real_free(gpointer data)
{
	HgStack *stack = data;

	if (stack->stack)
		hg_mem_free(stack->stack);
}

static void
_hg_stack_real_set_flags(gpointer data,
			 guint    flags)
{
	HgStack *stack = data;
	HgMemObject *obj;
	HgListIter iter;

	if (stack->stack) {
		iter = hg_list_iter_new(stack->stack);
		do {
			gpointer p = hg_list_iter_get_data(iter);

			hg_mem_get_object__inline(p, obj);
			if (obj == NULL) {
				hg_log_warning("Invalid object %p to be marked: HgValueNode in stack.", p);
			} else {
				if (!hg_mem_is_flags__inline(obj, flags))
					hg_mem_set_flags__inline(obj, flags, TRUE);
			}
		} while (hg_list_get_iter_next(stack->stack, iter));
		hg_list_iter_free(iter);
	}
}

static void
_hg_stack_real_relocate(gpointer           data,
			HgMemRelocateInfo *info)
{
	HgStack *stack = data;
	HgListIter iter;

	if (stack->stack) {
		iter = hg_list_iter_new(stack->stack);
		do {
			gpointer p = hg_list_iter_get_data(iter);

			if ((gsize)p >= info->start &&
			    (gsize)p <= info->end) {
				hg_list_iter_set_data(iter, (gpointer)((gsize)p + info->diff));
			}
		} while (hg_list_get_iter_next(stack->stack, iter));
		hg_list_iter_free(iter);
	}
}

static gpointer
_hg_stack_real_dup(gpointer data)
{
	HgStack *stack = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;

	retval = hg_stack_new(obj->pool, stack->max_depth);
	if (retval == NULL) {
		hg_log_warning("Failed to duplicate a stack.");
		return NULL;
	}
	retval->stack = hg_object_dup((HgObject *)stack->stack);

	return retval;
}

/*
 * Public Functions
 */
HgStack *
hg_stack_new(HgMemPool *pool,
	     guint      max_depth)
{
	HgStack *retval;

	g_return_val_if_fail (pool != NULL, NULL);
	g_return_val_if_fail (max_depth > 0, NULL);

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgStack),
					 HG_FL_HGOBJECT);
	if (retval == NULL) {
		hg_log_warning("Failed to create a stack.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	hg_object_set_vtable(&retval->object, &__hg_stack_vtable);
	retval->current_depth = 0;
	retval->max_depth = max_depth;
	retval->stack = NULL;
	retval->use_validator = TRUE;

	return retval;
}

void
_hg_stack_use_stack_validator(HgStack  *stack,
			      gboolean  flag)
{
	g_return_if_fail (stack != NULL);

	stack->use_validator = flag;
}

guint
hg_stack_depth(HgStack *stack)
{
	g_return_val_if_fail (stack != NULL, 0);

	return stack->current_depth;
}

gboolean
_hg_stack_push(HgStack     *stack,
	       HgValueNode *node)
{
	g_return_val_if_fail (stack != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);

	if (stack->stack == NULL) {
		HgMemObject *obj;

		hg_mem_get_object__inline(stack, obj);
		stack->stack = hg_list_new(obj->pool);
	}
	stack->stack = hg_list_append(stack->stack, node);
	stack->current_depth++;

	return TRUE;
}

gboolean
hg_stack_push(HgStack     *stack,
	      HgValueNode *node)
{
	g_return_val_if_fail (stack != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);

	if (stack->use_validator &&
	    stack->current_depth >= stack->max_depth)
		return FALSE;

	return _hg_stack_push(stack, node);
}

HgValueNode *
hg_stack_pop(HgStack *stack)
{
	HgValueNode *retval;
	HgListIter iter;

	g_return_val_if_fail (stack != NULL, NULL);

	if (stack->stack == NULL)
		return NULL;

	iter = hg_list_iter_new(stack->stack);
	hg_list_get_iter_last(stack->stack, iter);
	retval = hg_list_iter_get_data(iter);
	stack->stack = hg_list_iter_delete_link(iter);
	hg_list_iter_free(iter);
	stack->current_depth--;

	return retval;
}

void
hg_stack_clear(HgStack *stack)
{
	g_return_if_fail (stack != NULL);

	if (stack->stack)
		hg_mem_free(stack->stack);
	stack->stack = NULL;
	stack->current_depth = 0;
}

HgValueNode *
hg_stack_index(HgStack *stack,
	       guint    index_from_top)
{
	HgListIter iter;
	HgValueNode *retval;

	g_return_val_if_fail (stack != NULL, NULL);
	g_return_val_if_fail (index_from_top < stack->current_depth, NULL);

	iter = hg_list_iter_new(stack->stack);
	hg_list_get_iter_last(stack->stack, iter);
	while (index_from_top > 0) {
		if (!hg_list_get_iter_previous(stack->stack, iter)) {
			hg_log_warning("Detected inconsistency of stack during getting with index.");
			return NULL;
		}
		index_from_top--;
	}
	retval = hg_list_iter_get_data(iter);
	hg_list_iter_free(iter);

	return retval;
}

void
hg_stack_roll(HgStack *stack,
	      guint    n_block,
	      gint32   n_times)
{
	HgListIter start, end;
	gint32 n, i;

	g_return_if_fail (stack != NULL);
	g_return_if_fail (n_block <= stack->current_depth);

	if (n_block == 0 ||
	    n_times == 0)
		return;
	n = abs(n_times) % n_block;
	if (n != 0) {
		start = hg_list_iter_new(stack->stack);
		end = hg_list_iter_new(stack->stack);
		hg_list_get_iter_last(stack->stack, start);
		hg_list_get_iter_last(stack->stack, end);
		for (i = n_block; i > 1; i--, hg_list_get_iter_previous(stack->stack, start));
		if (n_times < 0) {
			HgListIter tmp = start;

			start = end;
			end = tmp;
		}
		stack->stack = hg_list_iter_roll(start, end, n);
		hg_list_iter_free(start);
		hg_list_iter_free(end);
	}
}

void
hg_stack_dump(HgStack      *stack,
	      HgFileObject *file)
{
	HgListIter iter;
	HgValueNode *node;

	g_return_if_fail (stack != NULL);

	hg_file_object_printf(file, "   address|   type|content\n");
	hg_file_object_printf(file, "----------+-------+-------------------------------\n");
	if (stack->stack) {
		iter = hg_list_iter_new(stack->stack);
		do {
			node = hg_list_iter_get_data(iter);
			hg_value_node_debug_print(file,
						  HG_DEBUG_DUMP,
						  HG_VALUE_GET_VALUE_TYPE (node),
						  stack, node, NULL);
		} while (hg_list_get_iter_next(stack->stack, iter));
		hg_list_iter_free(iter);
	}
}
