/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
 * Copyright (C) 2007 Akira TAGOH
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

#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgstack.h>
#include <hieroglyph/vm.h>


int
main(int    argc,
     char **argv)
{
	hg_vm_t *vm;
	hg_stack_t *stack;
	hg_object_t *obj;
	gint i = 1;

	vm = hg_vm_new();
	stack = hg_stack_new(vm, 10);
	hg_stack_dump(vm, stack);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	hg_stack_dump(vm, stack);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	hg_stack_dump(vm, stack);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	hg_stack_dump(vm, stack);
	hg_stack_pop(stack);
	hg_stack_dump(vm, stack);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	hg_stack_dump(vm, stack);
	hg_stack_roll(stack, 3, 2);
	hg_stack_dump(vm, stack);
	obj = hg_object_integer_new(vm, i++);
	hg_stack_push(stack, obj);
	hg_stack_dump(vm, stack);
	hg_stack_roll(stack, 3, -2);
	hg_stack_dump(vm, stack);
	hg_stack_roll(stack, 6, 2);
	hg_stack_dump(vm, stack);
	hg_stack_pop(stack);
	hg_stack_pop(stack);
	hg_stack_pop(stack);
	hg_stack_pop(stack);
	hg_stack_pop(stack);
	hg_stack_pop(stack);
	hg_stack_dump(vm, stack);

	i = 1;
	do {
		obj = hg_object_integer_new(vm, i++);
	} while (hg_stack_push(stack, obj));
	printf("%d\n", i);
	hg_stack_dump(vm, stack);
	hg_stack_roll(stack, 3, 2);
	hg_stack_dump(vm, stack);
	hg_stack_roll(stack, 3, -2);
	hg_stack_dump(vm, stack);
	hg_stack_roll(stack, 10, -2);
	hg_stack_dump(vm, stack);

	hg_stack_free(vm, stack);
	hg_vm_destroy(vm);

	return 0;
}
