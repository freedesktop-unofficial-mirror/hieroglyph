/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
 * Copyright (C) 2005-2007 Akira TAGOH
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/vm.h>
#include "hgstack.h"


/*
 * private functions
 */

/*
 * public functions
 */
hg_stack_t *
hg_stack_new(hg_vm_t *vm,
	     gsize    depth)
{
	hg_stack_t *retval;
	hg_stackdata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (depth > 0, NULL);

	retval = hg_vm_malloc(vm, hg_n_alignof (sizeof (hg_stack_t) + sizeof (hg_stackdata_t) * (depth + 1)));
	if (retval != NULL) {
		retval->stack_depth = depth;
		retval->current_depth = depth;
		data = (hg_stackdata_t *)retval->data;
		memset(data, 0, sizeof (hg_stackdata_t) * (depth + 1));
		data[depth].next = &data[depth-1];
		retval->stack_top = &data[depth];
	}

	return retval;
}

void
hg_stack_free(hg_vm_t    *vm,
	      hg_stack_t *stack)
{
	hg_return_if_fail (stack != NULL);

	hg_vm_mfree(vm, stack);
}

gboolean
hg_stack_push(hg_stack_t  *stack,
	      hg_object_t *object)
{
	hg_stackdata_t *data, *prev;

	hg_return_val_if_fail (stack != NULL, FALSE);
	hg_return_val_if_fail (object != NULL, FALSE);

	if (stack->current_depth == 0)
		return FALSE;
	prev = data = stack->stack_top;
	hg_return_val_if_fail (data->next != NULL, FALSE);

	stack->current_depth--;
	data = data->next;
	data->data = object;
	if (data->prev == NULL)
		data->prev = prev;
	if (stack->current_depth > 0) {
		if (data->next == NULL)
			data->next = &data[-1];
	} else {
		data->next = NULL;
	}
	stack->stack_top = data;

	return TRUE;
}

hg_object_t *
hg_stack_pop(hg_stack_t *stack)
{
	hg_object_t *retval;
	hg_stackdata_t *data;

	hg_return_val_if_fail (stack != NULL, NULL);
	hg_return_val_if_fail (stack->stack_depth > stack->current_depth, NULL);

	data = stack->stack_top;
	retval = data->data;
	stack->stack_top = data->prev;
	stack->current_depth++;

	return retval;
}

void
hg_stack_clear(hg_stack_t *stack)
{
	hg_stackdata_t *data;

	hg_return_if_fail (stack != NULL);

	stack->current_depth = stack->stack_depth;
	data = (hg_stackdata_t *)stack->data;
	stack->stack_top = &data[stack->stack_depth];
}

gsize
hg_stack_length(hg_stack_t *stack)
{
	hg_return_val_if_fail (stack != NULL, 0);

	return stack->stack_depth - stack->current_depth;
}

hg_object_t *
hg_stack_index(hg_stack_t *stack,
	       gsize       index)
{
	hg_stackdata_t *data;

	hg_return_val_if_fail (stack != NULL, NULL);
	hg_return_val_if_fail (index < (stack->stack_depth - stack->current_depth), NULL);

	for (data = stack->stack_top; index > 0; data = data->prev, index--);

	return data->data;
}

gboolean
hg_stack_roll(hg_stack_t *stack,
	      gsize       n_block,
	      gssize      n_times)
{
	hg_stackdata_t *before, *after, *beginning, *ending;
	gsize n, i;

	hg_return_val_if_fail (stack != NULL, FALSE);
	hg_return_val_if_fail (n_block <= (stack->stack_depth - stack->current_depth), FALSE);

	if (n_block == 0 ||
	    n_times == 0) {
		/* This case is the same to do nothing */
		return TRUE;
	}
	n = abs(n_times) % n_block;
	if (n == 0) {
		/* This case is the same to do nothing */
		return TRUE;
	}

	/* find a place where isn't targetted for roll */
	for (before = stack->stack_top, i = n_block;
	     i > 0;
	     before = before->prev, i--);
	after = before->next;
	/* try to find the place where cutting off */
	if (n_times > 0) {
		for (beginning = stack->stack_top; n > 1; beginning = beginning->prev, n--);
		ending = beginning->prev;
	} else {
		for (ending = after; n > 1; ending = ending->next, n--);
		beginning = ending->next;
	}
	ending->next = stack->stack_top->next;
	stack->stack_top->next = after;
	stack->stack_top->next->prev = stack->stack_top;
	before->next = beginning;
	before->next->prev = before;
	stack->stack_top = ending;

	return TRUE;
}

void
hg_stack_dump(hg_vm_t    *vm,
	      hg_stack_t *stack)
{
	gsize i;
	hg_stackdata_t *data;
	gchar *p;

	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (stack != NULL);

	for (i = 0, data = stack->stack_top;
	     i < (stack->stack_depth - stack->current_depth);
	     i++, data = data->prev) {
		p = hg_object_dump(data->data, TRUE);
		printf("%" G_GSIZE_FORMAT ": %s\n", i + 1, p);
		g_free(p);
	}
}
