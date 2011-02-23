/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
 * Copyright (C) 2005-2011 Akira TAGOH
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
#include "hgstack.h"
#include "hgstack-private.h"
#include "hgvm.h"

#include "hgstack.proto.h"

struct _hg_slist_t {
	hg_quark_t  self;
	hg_quark_t  data;
	hg_slist_t *next;
};
struct _hg_stack_spool_t {
	hg_mem_t   *mem;
	hg_quark_t  self;
	hg_slist_t *spool;
};

HG_DEFINE_VTABLE_WITH (stack, NULL, NULL, NULL);

/*< private >*/
HG_INLINE_FUNC hg_slist_t *
_hg_stack_spooler_new_node(hg_stack_spool_t *spool)
{
	hg_slist_t *retval;

	if (spool->spool) {
		retval = spool->spool;
		spool->spool = retval->next;

		hg_mem_ref(spool->mem, retval->self);
	} else {
		retval = _hg_slist_new(spool->mem);
	}

	return retval;
}

HG_INLINE_FUNC void
_hg_stack_spooler_free_node(hg_stack_spool_t *spool,
			    hg_slist_t       *node)
{
	node->next = spool->spool;
	spool->spool = node;

	hg_mem_unref(spool->mem, node->self);
}

static hg_bool_t
_hg_stack_spooler_gc_mark(hg_stack_spool_t  *spool)
{
	hg_slist_t *l;

	if (spool) {
		if (!hg_mem_gc_mark(spool->mem, spool->self))
			return FALSE;
		for (l = spool->spool; l != NULL; l = l->next) {
			if (!hg_mem_gc_mark(spool->mem, l->self))
				return FALSE;
		}
	}

	return TRUE;
}

static hg_usize_t
_hg_object_stack_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_stack_t));
}

static hg_uint_t
_hg_object_stack_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_stack_initialize(hg_object_t *object,
			    va_list      args)
{
	hg_stack_t *stack = (hg_stack_t *)object;

	stack->max_depth = va_arg(args, hg_usize_t);
	stack->last_stack = NULL;
	stack->depth = 0;
	stack->validate_depth = TRUE;
	stack->vm = va_arg(args, hg_vm_t *);

	return TRUE;
}

static hg_quark_t
_hg_object_stack_copy(hg_object_t             *object,
		      hg_quark_iterate_func_t  func,
		      hg_pointer_t             user_data,
		      hg_pointer_t            *ret)
{
	return Qnil;
}

static hg_char_t *
_hg_object_stack_to_cstr(hg_object_t             *object,
			 hg_quark_iterate_func_t  func,
			 hg_pointer_t             user_data)
{
	return g_strdup("-stack-");
}

static hg_bool_t
_hg_object_stack_gc_mark(hg_object_t *object)
{
	hg_stack_t *stack = (hg_stack_t *)object;
	hg_slist_t *l;
	hg_mem_t *m;

	for (l = stack->last_stack; l != NULL; l = l->next) {
		if (!hg_mem_gc_mark(object->mem, l->self))
			return FALSE;
		m = hg_mem_spool_get(hg_quark_get_mem_id(l->data));
		if (!m) {
			hg_warning("Unable to obtain the memory spooler from %lx",
				   l->data);
			hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
		}
		if (!hg_mem_gc_mark(m, l->data))
			return FALSE;
	}
	return _hg_stack_spooler_gc_mark(stack->spool);
}

static hg_bool_t
_hg_object_stack_compare(hg_object_t             *o1,
			 hg_object_t             *o2,
			 hg_quark_compare_func_t  func,
			 hg_pointer_t             user_data)
{
	return o1->self == o2->self;
}

HG_INLINE_FUNC hg_slist_t *
_hg_slist_new(hg_mem_t *mem)
{
	hg_quark_t self;
	hg_slist_t *l = NULL;

	self = hg_mem_alloc(mem,
			    sizeof (hg_slist_t),
			    (hg_pointer_t *)&l);
	if (self == Qnil)
		return NULL;

	l->self = self;
	l->data = Qnil;
	l->next = NULL;

	hg_mem_ref(mem, self);

	return l;
}

HG_INLINE_FUNC void
_hg_slist_free(hg_mem_t   *mem,
	       hg_slist_t *list)
{
	hg_slist_t *l, *tmp = NULL;

	for (l = list; l != NULL; l = tmp) {
		tmp = l->next;
		hg_mem_free(mem, l->self);
	}
}

static hg_bool_t
_hg_stack_push(hg_stack_t *stack,
	       hg_quark_t  quark)
{
	hg_slist_t *l;
	hg_mem_t *m;

	if (stack->spool)
		l = _hg_stack_spooler_new_node(stack->spool);
	else
		l = _hg_slist_new(stack->o.mem);
	if (l == NULL)
		return FALSE;

	l->data = quark;
	l->next = stack->last_stack;
	stack->last_stack = l;
	stack->depth++;

	if (stack->vm &&
	    !hg_quark_is_simple_object(quark) &&
	    hg_quark_get_type(quark) != HG_TYPE_OPER) {
		m = hg_vm_get_mem_from_quark(stack->vm, quark);
		hg_mem_unref(m, quark);
	}

	return TRUE;
}

static hg_quark_t
_hg_stack_pop(hg_stack_t  *stack)
{
	hg_slist_t *l;
	hg_quark_t retval;

	if (stack->last_stack == NULL)
		return Qnil;

	l = stack->last_stack;
	stack->last_stack = l->next;
	retval = l->data;
	if (stack->spool)
		_hg_stack_spooler_free_node(stack->spool, l);
	else
		hg_mem_free(stack->o.mem, l->self);
	stack->depth--;

	return retval;
}

/*< public >*/

/**
 * hg_stack_spooler_new:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
hg_stack_spool_t *
hg_stack_spooler_new(hg_mem_t *mem)
{
	hg_stack_spool_t *retval;
	hg_quark_t q;

	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);

	q = hg_mem_alloc(mem, sizeof (hg_stack_spool_t),
			 (hg_pointer_t)&retval);
	if (q == Qnil)
		return NULL;

	retval->self = q;
	retval->mem = mem;
	retval->spool = NULL;

	hg_mem_ref(mem, q);

	return retval;
}

/**
 * hg_stack_spooler_destroy:
 * @spool:
 *
 * FIXME
 */
void
hg_stack_spooler_destroy(hg_stack_spool_t *spool)
{
	hg_return_if_fail (spool != NULL, HG_e_VMerror);

	_hg_slist_free(spool->mem, spool->spool);
	hg_mem_free(spool->mem, spool->self);
}

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
hg_stack_new(hg_mem_t   *mem,
	     hg_usize_t  max_depth,
	     hg_vm_t    *vm)
{
	hg_stack_t *retval = NULL;
	hg_quark_t self;

	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);
	hg_return_val_if_fail (max_depth > 0, NULL, HG_e_VMerror);

	self = hg_object_new(mem, (hg_pointer_t *)&retval, HG_TYPE_STACK, 0, max_depth, vm);
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

	hg_object_free(stack->o.mem, stack->self);
}

/**
 * hg_stack_set_spooler:
 * @stack:
 * @spool:
 *
 * FIXME
 */
void
hg_stack_set_spooler(hg_stack_t       *stack,
		     hg_stack_spool_t *spool)
{
	hg_return_if_fail (stack != NULL, HG_e_typecheck);

	stack->spool = spool;
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
			hg_bool_t   flag)
{
	hg_return_if_fail (stack != NULL, HG_e_typecheck);

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
hg_usize_t
hg_stack_depth(hg_stack_t *stack)
{
	hg_return_val_if_fail (stack != NULL, 0, HG_e_typecheck);

	return stack->depth;
}

/**
 * hg_stack_set_max_depth:
 * @stack:
 * @depth:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_stack_set_max_depth(hg_stack_t *stack,
		       hg_usize_t  depth)
{
	hg_return_val_if_fail (stack != NULL, FALSE, HG_e_typecheck);

	if (depth < stack->depth || depth == 0)
		return FALSE;

	stack->max_depth = depth;

	return TRUE;
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
hg_bool_t
hg_stack_push(hg_stack_t *stack,
	      hg_quark_t  quark)
{
	hg_return_val_if_fail (stack != NULL, FALSE, HG_e_typecheck);

	if (stack->validate_depth &&
	    stack->depth >= stack->max_depth)
		return FALSE;

	return _hg_stack_push(stack, quark);
}

/**
 * hg_stack_pop:
 * @stack:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_stack_pop(hg_stack_t *stack)
{
	hg_quark_t retval;
	hg_mem_t *m;

	hg_return_val_if_fail (stack != NULL, Qnil, HG_e_typecheck);

	retval = _hg_stack_pop(stack);

	if (stack->vm &&
	    !hg_quark_is_simple_object(retval) &&
	    hg_quark_get_type(retval) != HG_TYPE_OPER) {
		m = hg_vm_get_mem_from_quark(stack->vm, retval);
		hg_mem_ref(m, retval);
	}

	return retval;
}

/**
 * hg_stack_drop:
 * @stack:
 *
 * FIXME
 */
void
hg_stack_drop(hg_stack_t *stack)
{
	hg_return_if_fail (stack != NULL, HG_e_typecheck);

	_hg_stack_pop(stack);
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
	hg_return_if_fail (stack != NULL, HG_e_typecheck);

	_hg_slist_free(stack->o.mem, stack->last_stack);
	stack->last_stack = NULL;
	stack->depth = 0;
}

/**
 * hg_stack_index:
 * @stack:
 * @index:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_stack_index(hg_stack_t *stack,
	       hg_usize_t  index)
{
	hg_slist_t *l;

	hg_return_val_if_fail (stack != NULL, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (index < stack->depth, Qnil, HG_e_stackunderflow);

	for (l = stack->last_stack; index > 0; l = l->next, index--);

	return l->data;
}

/**
 * hg_stack_peek:
 * @stack:
 * @index:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t *
hg_stack_peek(hg_stack_t *stack,
	      hg_usize_t  index)
{
	hg_slist_t *l;

	hg_return_val_if_fail (stack != NULL, NULL, HG_e_typecheck);
	hg_return_val_if_fail (index < stack->depth, NULL, HG_e_stackunderflow);

	for (l = stack->last_stack; index > 0; l = l->next, index--);

	return &l->data;
}

/**
 * hg_stack_exch:
 * @stack:
 *
 * FIXME
 */
void
hg_stack_exch(hg_stack_t *stack)
{
	hg_quark_t q;

	hg_return_if_fail (stack != NULL, HG_e_typecheck);
	hg_return_if_fail (stack->depth > 1, HG_e_stackunderflow);

	q = stack->last_stack->data;
	stack->last_stack->data = stack->last_stack->next->data;
	stack->last_stack->next->data = q;
}

/**
 * hg_stack_roll:
 * @stack:
 * @n_blocks:
 * @n_times:
 *
 * FIXME
 */
void
hg_stack_roll(hg_stack_t *stack,
	      hg_usize_t  n_blocks,
	      hg_size_t   n_times)
{
	hg_slist_t *oonode, *top_node, *last_node;
	hg_size_t n, i;

	hg_return_if_fail (stack != NULL, HG_e_typecheck);
	hg_return_if_fail (n_blocks <= stack->depth, HG_e_stackunderflow);

	if (n_blocks == 0 ||
	    n_times == 0)
		return;
	n = abs(n_times) % n_blocks;
	if (n != 0) {
		/* find out a node out of the blocks */
		for (last_node = stack->last_stack, i = n_blocks;
		     i > 1;
		     last_node = last_node->next, i--);
		oonode = last_node->next;
		last_node->next = stack->last_stack;
		/* rolling out! */
		if (n_times < 0)
			n = n_blocks - n;
		for (top_node = stack->last_stack;
		     n > 1;
		     top_node = top_node->next, n--);
		stack->last_stack = top_node->next;
		top_node->next = oonode;
	}
}

/**
 * hg_stack_foreach:
 * @stack:
 * @func:
 * @data:
 * @is_forwarded:
 *
 * FIXME
 */
void
hg_stack_foreach(hg_stack_t               *stack,
		 hg_stack_traverse_func_t  func,
		 hg_pointer_t              data,
		 hg_bool_t                 is_forwarded)
{
	hg_slist_t *l;

	hg_return_if_fail (stack != NULL, HG_e_typecheck);
	hg_return_if_fail (func != NULL, HG_e_VMerror);

	if (stack->depth == 0)
		return;

	if (is_forwarded) {
		hg_pointer_t *p = g_new0(hg_pointer_t, stack->depth + 1);
		hg_size_t i;

		for (l = stack->last_stack, i = stack->depth;
		     l != NULL && i > 0;
		     l = l->next, i--)
			p[i - 1] = l;
		for (i = 0; i < stack->depth; i++) {
			if (!func(stack->o.mem, ((hg_slist_t *)p[i])->data, data))
				break;
		}
		g_free(p);
	} else {
		for (l = stack->last_stack; l != NULL; l = l->next) {
			if (!func(stack->o.mem, l->data, data))
				break;
		}
	}
}
