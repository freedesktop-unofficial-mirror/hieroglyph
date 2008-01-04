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
#include <hieroglyph/hgstack.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/vm.h>
#include "main.h"


hg_vm_t *vm;
hg_stack_t *obj;

void
setup(void)
{
	vm = hg_vm_new();
}

void
teardown(void)
{
	hg_vm_destroy(vm);
}

/* stack */
TDEF (hg_stack_new)
{
	gboolean flag;

	obj = hg_stack_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create a stack object");
	fail_unless(obj->stack_depth == 10, "the stack depth isn't expected depth.");

	hg_stack_free(vm, obj);

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	/* disable warnings */
	hg_quiet_warning_messages(TRUE);

	obj = hg_stack_new(vm, 0);

	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(obj == NULL, "depth must be more than 0");
}
TEND

TDEF (hg_stack_free)
{
	/* nothing to check here particularly */
}
TEND

TDEF (hg_stack_push)
{
	hg_object_t *obj2;
	gboolean flag, retval;

	obj = hg_stack_new(vm, 1);

	fail_unless(obj != NULL, "Failed to create a stack object");

	obj2 = hg_object_boolean_new(vm, TRUE);
	fail_unless(obj2 != NULL, "Failed to create a boolean object");

	fail_unless(hg_stack_push(obj, obj2), "Failed to push an object to stack");

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	/* disable warnings */
	hg_quiet_warning_messages(TRUE);

	retval = hg_stack_push(obj, obj2);

	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(!retval, "Object has to be unable to be pushed.");

	hg_stack_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_stack_pop)
{
	hg_object_t *obj2, *obj3;
	gboolean flag;

	obj = hg_stack_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create a stack object");

	obj2 = hg_object_boolean_new(vm, TRUE);
	fail_unless(obj2 != NULL, "Failed to create a boolean object");

	fail_unless(hg_stack_push(obj, obj2), "Failed to push an object to stack");

	fail_unless((obj3 = hg_stack_pop(obj)) != NULL, "Failed to pop an object from stack");
	fail_unless(obj2 == obj3, "Failed to pop an object from stack (verifying)");

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	/* disable warnings */
	hg_quiet_warning_messages(TRUE);

	obj3 = hg_stack_pop(obj);

	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(obj3 == NULL, "stack has to be empty.");

	hg_stack_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_stack_length)
{
	hg_object_t *obj2;

	obj = hg_stack_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create a stack object");
	fail_unless(hg_stack_length(obj) == 0, "stack has to be empty.");

	obj2 = hg_object_boolean_new(vm, TRUE);
	fail_unless(obj2 != NULL, "Failed to create a boolean object");

	fail_unless(hg_stack_push(obj, obj2), "Failed to push an object to stack");
	fail_unless(hg_stack_length(obj) == 1, "Failed to get current depth");

	hg_stack_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_stack_clear)
{
	hg_object_t *obj2;

	obj = hg_stack_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create a stack object");

	obj2 = hg_object_boolean_new(vm, TRUE);
	fail_unless(obj2 != NULL, "Failed to create a boolean object");

	fail_unless(hg_stack_push(obj, obj2), "Failed to push an object to stack");
	fail_unless(hg_stack_length(obj) == 1, "Failed to get current depth");

	hg_stack_clear(obj);

	fail_unless(hg_stack_length(obj) == 0, "Failed to clear the stack");

	hg_stack_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_stack_index)
{
	hg_object_t *obj2, *obj3, *obj4;
	gboolean flag;

	obj = hg_stack_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create a stack object");

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	/* disable warnings */
	hg_quiet_warning_messages(TRUE);

	obj2 = hg_stack_index(obj, 0);

	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(obj2 == NULL, "index has to be less than the amount of the objects in the stack.");

	obj2 = hg_object_boolean_new(vm, TRUE);
	fail_unless(obj2 != NULL, "Failed to create a boolean object");

	hg_stack_push(obj, obj2);
	obj3 = hg_stack_index(obj, 0);
	fail_unless(obj2 == obj3, "gotten object isn't an expected object");

	obj3 = hg_object_integer_new(vm, 10);
	fail_unless(obj3 != NULL, "Failed to create an integer object");

	hg_stack_push(obj, obj3);
	obj4 = hg_stack_index(obj, 0);
	fail_unless(obj3 == obj4, "gotten object isn't an expected object (integer)");
	obj4 = hg_stack_index(obj, 1);
	fail_unless(obj4 == obj2, "gotten object isn't an expected object (boolean)");

	hg_stack_free(vm, obj);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj3);
}
TEND

TDEF (hg_stack_roll)
{
	hg_object_t *obj2, *obj3, *obj4, *obj5;
	gboolean flag, retval;

	obj = hg_stack_new(vm, 1);
	fail_unless(obj != NULL, "Failed to create a stack object");

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	/* disable warnings */
	hg_quiet_warning_messages(TRUE);

	retval = hg_stack_roll(obj, 2, 1);

	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(!retval, "block has to be less than the stack size");

	hg_stack_free(vm, obj);

	obj = hg_stack_new(vm, 10);
	fail_unless(obj != NULL, "Failed to create a stack object");

	obj2 = hg_object_boolean_new(vm, TRUE);
	obj3 = hg_object_integer_new(vm, 1);
	obj4 = hg_object_mark_new(vm);
	obj5 = hg_object_boolean_new(vm, FALSE);
	fail_unless(obj2 != NULL, "Failed to create a boolean object");
	fail_unless(obj3 != NULL, "Failed to create an integer object");
	fail_unless(obj4 != NULL, "Failed to create a mark object");
	fail_unless(obj5 != NULL, "Failed to create a boolean object");

	hg_stack_push(obj, obj2);
	hg_stack_push(obj, obj3);
	fail_unless(hg_stack_roll(obj, 0, 1), "Failed to roll the objects");
	fail_unless(hg_stack_index(obj, 0) == obj3, "Failed to verify after rolling (integer) n_blocks == 0");
	fail_unless(hg_stack_index(obj, 1) == obj2, "Failed to verify after rolling (boolean) n_blocks == 0");

	fail_unless(hg_stack_roll(obj, 2, 0), "Failed to roll the objects");
	fail_unless(hg_stack_index(obj, 0) == obj3, "Failed to verify after rolling (integer) n_times == 0");
	fail_unless(hg_stack_index(obj, 1) == obj2, "Failed to verify after rolling (boolean) n_times == 0");

	hg_stack_push(obj, obj4);
	fail_unless(hg_stack_roll(obj, 3, 2), "Failed to roll the objects");
	fail_unless(hg_stack_index(obj, 0) == obj2, "Failed to verify after rolling (boolean)");
	fail_unless(hg_stack_index(obj, 1) == obj4, "Failed to verify after rolling (mark)");
	fail_unless(hg_stack_index(obj, 2) == obj3, "Failed to verify after rolling (integer)");

	hg_stack_push(obj, obj5);
	fail_unless(hg_stack_roll(obj, 3, -2), "Failed to roll the objects");
	fail_unless(hg_stack_index(obj, 0) == obj2, "Failed to verify after rolling (boolean)");
	fail_unless(hg_stack_index(obj, 1) == obj4, "Failed to verify after rolling (mark)");
	fail_unless(hg_stack_index(obj, 2) == obj5, "Failed to verify after rolling (boolean)");
	fail_unless(hg_stack_index(obj, 3) == obj3, "Failed to verify after rolling (integer)");

	fail_unless(hg_stack_roll(obj, 4, 3), "Failed to roll the objects");
	fail_unless(hg_stack_index(obj, 0) == obj3, "Failed to verify after rolling (integer)");
	fail_unless(hg_stack_index(obj, 1) == obj2, "Failed to verify after rolling (boolean)");
	fail_unless(hg_stack_index(obj, 2) == obj4, "Failed to verify after rolling (mark)");
	fail_unless(hg_stack_index(obj, 3) == obj5, "Failed to verify after rolling (boolean)");

	hg_stack_free(vm, obj);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj3);
	hg_object_free(vm, obj4);
}
TEND

TDEF (hg_stack_dump)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_stack_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_stack_new);
	T (hg_stack_free);
	T (hg_stack_push);
	T (hg_stack_pop);
	T (hg_stack_clear);
	T (hg_stack_length);
	T (hg_stack_index);
	T (hg_stack_roll);
	T (hg_stack_dump);

	suite_add_tcase(s, tc);

	return s;
}
