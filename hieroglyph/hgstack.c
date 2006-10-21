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
#include "hglog.h"
#include "hgmem.h"
#include "hgfile.h"
#include "hgstring.h"
#include "hgvaluenode.h"


struct _HieroGlyphStack {
	HgObject  object;
	GList    *stack;
	GList    *last_stack;
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

	g_list_free(stack->stack);
}

static void
_hg_stack_real_set_flags(gpointer data,
			 guint    flags)
{
	HgStack *stack = data;
	GList *list;
	HgMemObject *obj;

	for (list = stack->stack; list != NULL; list = g_list_next(list)) {
		hg_mem_get_object__inline(list->data, obj);
		if (obj == NULL) {
			hg_log_warning("Invalid object %p to be marked: HgValueNode in stack.", list->data);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_set_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_stack_real_relocate(gpointer           data,
			HgMemRelocateInfo *info)
{
	HgStack *stack = data;
	GList *list;

	for (list = stack->stack; list != NULL; list = g_list_next(list)) {
		if ((gsize)list->data >= info->start &&
		    (gsize)list->data <= info->end) {
			list->data = (gpointer)((gsize)list->data + info->diff);
		}
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
	retval->stack = g_list_copy(stack->stack);
	retval->last_stack = g_list_last(retval->stack);

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
	retval->last_stack = NULL;
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
	GList *list;

	g_return_val_if_fail (stack != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);

	list = g_list_alloc();
	list->data = node;
	if (stack->stack == NULL) {
		stack->stack = stack->last_stack = list;
	} else {
		stack->last_stack->next = list;
		list->prev = stack->last_stack;
		stack->last_stack = list;
	}
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
	GList *list;

	g_return_val_if_fail (stack != NULL, NULL);

	if (stack->last_stack == NULL)
		return NULL;

	list = stack->last_stack;
	stack->last_stack = list->prev;
	if (list->prev) {
		list->prev->next = NULL;
		list->prev = NULL;
	}
	if (stack->stack == list) {
		stack->stack = NULL;
	}
	retval = list->data;
	g_list_free_1(list);
	stack->current_depth--;

	return retval;
}

void
hg_stack_clear(HgStack *stack)
{
	g_return_if_fail (stack != NULL);

	g_list_free(stack->stack);
	stack->stack = NULL;
	stack->current_depth = 0;
}

HgValueNode *
hg_stack_index(HgStack *stack,
	       guint    index_from_top)
{
	GList *list;

	g_return_val_if_fail (stack != NULL, NULL);
	g_return_val_if_fail (index_from_top < stack->current_depth, NULL);

	for (list = stack->last_stack; index_from_top > 0; list = g_list_previous(list), index_from_top--);

	return list->data;
}

void
hg_stack_roll(HgStack *stack,
	      guint    n_block,
	      gint32   n_times)
{
	GList *notargeted_before, *notargeted_after, *beginning, *ending;
	gint32 n, i;

	g_return_if_fail (stack != NULL);
	g_return_if_fail (n_block <= stack->current_depth);

	if (n_block == 0 ||
	    n_times == 0)
		return;
	n = abs(n_times) % n_block;
	if (n != 0) {
		/* find the place that isn't targeted for roll */
		for (notargeted_before = stack->last_stack, i = n_block;
		     i > 0;
		     notargeted_before = g_list_previous(notargeted_before), i--);
		if (!notargeted_before)
			notargeted_after = stack->stack;
		else
			notargeted_after = notargeted_before->next;
		/* try to find the place to cut off */
		if (n_times > 0) {
			for (beginning = stack->last_stack; n > 1; beginning = g_list_previous(beginning), n--);
			ending = beginning->prev;
		} else {
			for (ending = notargeted_after; n > 1; ending = g_list_next(ending), n--);
			beginning = ending->next;
		}
		stack->last_stack->next = notargeted_after;
		stack->last_stack->next->prev = stack->last_stack;
		if (notargeted_before) {
			notargeted_before->next = beginning;
			notargeted_before->next->prev = notargeted_before;
		} else {
			stack->stack = beginning;
			stack->stack->prev = NULL;
		}
		stack->last_stack = ending;
		stack->last_stack->next = NULL;
	}
}

void
hg_stack_dump(HgStack      *stack,
	      HgFileObject *file)
{
	GList *l;
	HgValueNode *node;

	g_return_if_fail (stack != NULL);

	hg_file_object_printf(file, "   address|   type|content\n");
	hg_file_object_printf(file, "----------+-------+-------------------------------\n");
	for (l = stack->stack; l != NULL; l = g_list_next(l)) {
		node = l->data;
		hg_value_node_debug_print(file,
					  HG_DEBUG_DUMP,
					  HG_VALUE_GET_VALUE_TYPE (node),
					  stack, node, NULL);
	}
}
