/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
 * Copyright (C) 2005-2010 Akira TAGOH
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
#include "hgerror.h"
#include "hgmem.h"
#include "hgstack-private.h"
#include "hgstack.h"
#include "hgvm.h"


static hg_list_t *_hg_list_new  (hg_mem_t   *mem);
static void       _hg_list_free (hg_mem_t   *mem,
                                 hg_list_t  *list);
static gboolean   _hg_stack_push(hg_stack_t *stack,
                                 hg_quark_t  quark);


HG_DEFINE_VTABLE_WITH (stack, NULL, NULL, NULL);

/*< private >*/
static gsize
_hg_object_stack_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_stack_t));
}

static guint
_hg_object_stack_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static gboolean
_hg_object_stack_initialize(hg_object_t *object,
			    va_list      args)
{
	hg_stack_t *stack = (hg_stack_t *)object;

	stack->max_depth = va_arg(args, gsize);
	stack->stack = NULL;
	stack->last_stack = NULL;
	stack->depth = 0;
	stack->validate_depth = TRUE;
	stack->vm = va_arg(args, hg_vm_t *);

	return TRUE;
}

static hg_quark_t
_hg_object_stack_copy(hg_object_t              *object,
		      hg_quark_iterate_func_t   func,
		      gpointer                  user_data,
		      gpointer                 *ret,
		      GError                  **error)
{
	return Qnil;
}

static gchar *
_hg_object_stack_to_cstr(hg_object_t              *object,
			 hg_quark_iterate_func_t   func,
			 gpointer                  user_data,
			 GError                  **error)
{
	return g_strdup("-stack-");
}

static gboolean
_hg_object_stack_gc_mark(hg_object_t           *object,
			 hg_gc_iterate_func_t   func,
			 gpointer               user_data,
			 GError               **error)
{
	hg_stack_t *stack = (hg_stack_t *)object;
	hg_list_t *l;
	gboolean retval = TRUE;

	for (l = stack->stack; l != NULL; l = l->next) {
		if (!hg_mem_gc_mark(object->mem, l->self, error)) {
			retval = FALSE;
			break;
		}
		if (!func(l->data, user_data, error)) {
			retval = FALSE;
			break;
		}
	}

	return retval;
}

static gboolean
_hg_object_stack_compare(hg_object_t             *o1,
			 hg_object_t             *o2,
			 hg_quark_compare_func_t  func,
			 gpointer                 user_data)
{
	return o1->self == o2->self;
}

static hg_list_t *
_hg_list_new(hg_mem_t *mem)
{
	hg_quark_t self;
	hg_list_t *l = NULL;

	self = hg_mem_alloc(mem, sizeof (hg_list_t), (gpointer *)&l);
	if (self == Qnil)
		return NULL;

	l->self = self;
	l->data = Qnil;
	l->next = l->prev = NULL;

	return l;
}

static void
_hg_list_free(hg_mem_t  *mem,
	      hg_list_t *list)
{
	hg_list_t *l, *tmp = NULL;

	for (l = list; l != NULL; l = tmp) {
		tmp = l->next;
		hg_mem_free(mem, l->self);
	}
}

static gboolean
_hg_stack_push(hg_stack_t *stack,
	       hg_quark_t  quark)
{
	hg_list_t *l = _hg_list_new(stack->o.mem);
	hg_mem_t *m;

	if (l == NULL)
		return FALSE;

	l->data = quark;
	if (stack->stack == NULL) {
		stack->stack = stack->last_stack = l;
	} else {
		stack->last_stack->next = l;
		l->prev = stack->last_stack;
		stack->last_stack = l;
	}
	stack->depth++;

	if (stack->vm &&
	    !hg_quark_is_simple_object(quark) &&
	    hg_quark_get_type(quark) != HG_TYPE_OPER) {
		m = hg_vm_get_mem_from_quark(stack->vm, quark);
		hg_mem_reserved_spool_remove(m, quark);
	}

	return TRUE;
}

static hg_quark_t
_hg_stack_pop(hg_stack_t  *stack,
	      GError     **error)
{
	hg_list_t *l;
	hg_quark_t retval;

	if (stack->last_stack == NULL)
		return Qnil;

	l = stack->last_stack;
	stack->last_stack = l->prev;
	if (l->prev) {
		l->prev->next = NULL;
		l->prev = NULL;
	}
	if (stack->stack == l) {
		stack->stack = NULL;
	}
	retval = l->data;
	hg_mem_free(stack->o.mem, l->self);
	stack->depth--;

	return retval;
}

/*< public >*/
/**
 * hg_stack_new:
 * @mem:
 * @max_depth:
 *
 * FIXME
 *
 * Returns:
 */
hg_stack_t *
hg_stack_new(hg_mem_t *mem,
	     gsize     max_depth,
	     hg_vm_t  *vm)
{
	hg_stack_t *retval = NULL;
	hg_quark_t self;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (max_depth > 0, NULL);

	self = hg_object_new(mem, (gpointer *)&retval, HG_TYPE_STACK, 0, max_depth, vm);
	if (self != Qnil) {
		retval->self = self;
	}

	return retval;
}

/**
 * hg_stack_free:
 * @stack:
 *
 * FIXME
 */
void
hg_stack_free(hg_stack_t *stack)
{
	if (!stack)
		return;

	hg_mem_free(stack->o.mem, stack->self);
}

/**
 * hg_stack_set_validation:
 * @stack:
 * @flag:
 *
 * FIXME
 */
void
hg_stack_set_validation(hg_stack_t *stack,
			gboolean    flag)
{
	hg_return_if_fail (stack != NULL);

	stack->validate_depth = (flag == TRUE);
}

/**
 * hg_stack_depth:
 * @stack:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_stack_depth(hg_stack_t *stack)
{
	hg_return_val_if_fail (stack != NULL, 0);

	return stack->depth;
}

/**
 * hg_stack_push:
 * @stack:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_stack_push(hg_stack_t *stack,
	      hg_quark_t  quark)
{
	hg_return_val_if_fail (stack != NULL, FALSE);

	if (stack->validate_depth &&
	    stack->depth >= stack->max_depth)
		return FALSE;

	return _hg_stack_push(stack, quark);
}

/**
 * hg_stack_pop:
 * @stack:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_stack_pop(hg_stack_t  *stack,
	     GError     **error)
{
	hg_quark_t retval;
	hg_mem_t *m;

	hg_return_val_with_gerror_if_fail (stack != NULL, Qnil, error);

	retval = _hg_stack_pop(stack, error);

	if (stack->vm &&
	    !hg_quark_is_simple_object(retval) &&
	    hg_quark_get_type(retval) != HG_TYPE_OPER) {
		m = hg_vm_get_mem_from_quark(stack->vm, retval);
		hg_mem_reserved_spool_add(m, retval);
	}

	return retval;
}

/**
 * hg_stack_drop:
 * @stack:
 * @error:
 *
 * FIXME
 */
void
hg_stack_drop(hg_stack_t  *stack,
	      GError     **error)
{
	hg_return_with_gerror_if_fail (stack != NULL, error);

	_hg_stack_pop(stack, error);
}

/**
 * hg_stack_clear:
 * @stack:
 *
 * FIXME
 */
void
hg_stack_clear(hg_stack_t *stack)
{
	hg_return_if_fail (stack != NULL);

	_hg_list_free(stack->o.mem, stack->stack);
	stack->stack = stack->last_stack = NULL;
	stack->depth = 0;
}

/**
 * hg_stack_index:
 * @stack:
 * @index:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_stack_index(hg_stack_t  *stack,
	       gsize        index,
	       GError     **error)
{
	hg_list_t *l;

	hg_return_val_with_gerror_if_fail (stack != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail (index < stack->depth, Qnil, error);

	for (l = stack->last_stack; index > 0; l = l->prev, index--);

	return l->data;
}

/**
 * hg_stack_roll:
 * @stack:
 * @n_blocks:
 * @n_times:
 * @error:
 *
 * FIXME
 */
void
hg_stack_roll(hg_stack_t  *stack,
	      gsize        n_blocks,
	      gssize       n_times,
	      GError     **error)
{
	hg_list_t *notargeted_before, *notargeted_after, *beginning, *ending;
	gsize n, i;

	hg_return_with_gerror_if_fail (stack != NULL, error);
	hg_return_with_gerror_if_fail (n_blocks <= stack->depth, error);

	if (n_blocks == 0 ||
	    n_times == 0)
		return;
	n = abs(n_times) % n_blocks;
	if (n != 0) {
		/* find the place that isn't targeted for roll */
		for (notargeted_before = stack->last_stack, i = n_blocks;
		     i > 0;
		     notargeted_before = notargeted_before->prev, i--);
		if (!notargeted_before)
			notargeted_after = stack->stack;
		else
			notargeted_after = notargeted_before->next;
		/* try to find the place to cut off */
		if (n_times > 0) {
			for (beginning = stack->last_stack; n > 1; beginning = beginning->prev, n--);
			ending = beginning->prev;
		} else {
			for (ending = notargeted_after; n > 1; ending = ending->next, n--);
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

/**
 * hg_stack_foreach:
 * @stack:
 * @func:
 * @data:
 * @is_forwarded:
 * @error:
 *
 * FIXME
 */
void
hg_stack_foreach(hg_stack_t                *stack,
		 hg_array_traverse_func_t   func,
		 gpointer                   data,
		 gboolean                   is_forwarded,
		 GError                   **error)
{
	hg_list_t *l;

	hg_return_if_fail (stack != NULL);
	hg_return_if_fail (func != NULL);

	if (is_forwarded)
		l = stack->stack;
	else
		l = stack->last_stack;
	while (l != NULL) {
		if (!func(stack->o.mem, l->data, data, error))
			break;
		if (is_forwarded)
			l = l->next;
		else
			l = l->prev;
	}
}
