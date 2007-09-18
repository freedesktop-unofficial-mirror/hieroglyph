/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.c
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
#include <hieroglyph/hgobject.h>
#include <hieroglyph/vm.h>
#include "main.h"


hg_vm_t *vm;
hg_object_t *obj;

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

/* core object */
TDEF (hg_object_new)
{
	gboolean flag;

	obj = hg_object_new(vm, 1);

	fail_unless(obj != NULL, "Failed to create an object");
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");

	hg_object_free(vm, obj);

	obj = hg_object_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create an object");
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 10, "The amount of the object is different.");

	hg_object_free(vm, obj);

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	obj = hg_object_new(vm, 0);
	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);

	fail_unless(obj == NULL, "Not allowed to create empty object");
}
TEND

TDEF (hg_object_sized_new)
{
}
TEND

TDEF (hg_object_dup)
{
}
TEND

TDEF (hg_object_copy)
{
}
TEND

TDEF (hg_object_compare)
{
}
TEND

TDEF (hg_object_dump)
{
}
TEND

/* null object */
TDEF (hg_object_null_new)
{
	obj = hg_object_null_new(vm);

	fail_unless(obj != NULL, "Failed to create a null object");
	fail_unless(HG_OBJECT_IS_NULL (obj), "Created object isn't a null object.");

	hg_object_free(vm, obj);
}
TEND

/* integer object */
TDEF (hg_object_integer_new)
{
	obj = hg_object_integer_new(vm, 1);

	fail_unless(obj != NULL, "Failed to create an integer object");
	fail_unless(HG_OBJECT_IS_INTEGER (obj), "Created object isn't an integer object");
	fail_unless(HG_OBJECT_INTEGER (obj) == 1, "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
	obj = hg_object_integer_new(vm, -1);

	fail_unless(obj != NULL, "Failed to create an integer object");
	fail_unless(HG_OBJECT_IS_INTEGER (obj), "Created object isn't an integer object");
	fail_unless(HG_OBJECT_INTEGER (obj) == -1, "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
}
TEND

/* real object */
TDEF (hg_object_real_new)
{
}
TEND

/* name object */
TDEF (hg_object_name_new)
{
}
TEND

/* system encoding object */
TDEF (hg_object_system_encoding_new)
{
}
TEND

/* boolean object */
TDEF (hg_object_boolean_new)
{
}
TEND

/* mark object */
TDEF (hg_object_mark_new)
{
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_object_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_object_new);
	T (hg_object_sized_new);
	T (hg_object_dup);
	T (hg_object_copy);
	T (hg_object_compare);
	T (hg_object_dump);
	T (hg_object_null_new);
	T (hg_object_integer_new);
	T (hg_object_real_new);
	T (hg_object_name_new);
	T (hg_object_system_encoding_new);
	T (hg_object_boolean_new);
	T (hg_object_mark_new);

	suite_add_tcase(s, tc);

	return s;
}
