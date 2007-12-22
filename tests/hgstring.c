/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.c
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
#include <string.h>
#include <hieroglyph/hgstring.h>
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

/* string object */
TDEF (hg_object_string_new)
{
	gchar *p;

	obj = hg_object_string_new(vm, "foo");

	fail_unless(obj != NULL, "Failed to create a string object");
	fail_unless(HG_OBJECT_IS_STRING (obj), "Created object isn't a string object");
	p = HG_OBJECT_STRING_DATA (obj)->string;
	fail_unless(p[0] == 'f' && p[1] == 'o' && p[2] == 'o', "Object isn't set correctly on object creation");
	fail_unless(HG_OBJECT_STRING (obj)->real_length == 3, "Object doesn't have an expected length");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_string_sized_new)
{
	obj = hg_object_string_sized_new(vm, 10);

	fail_unless(obj != NULL, "Failed to create a string object");
	fail_unless(HG_OBJECT_IS_STRING (obj), "Created object isn't a string object");
	fail_unless(HG_OBJECT_STRING (obj)->length == 10, "Object doesn't have an expected length");

	hg_object_free(vm, obj);

	obj = hg_object_string_sized_new(vm, 0);

	fail_unless(obj != NULL, "Failed to create a string object");
	fail_unless(HG_OBJECT_IS_STRING (obj), "Created object isn't a string object");
	fail_unless(HG_OBJECT_STRING_DATA (obj)->string == NULL, "Don't have an allocated space");
	fail_unless(HG_OBJECT_STRING (obj)->length == 0, "Object doesn't have an expected length");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_string_substring_new)
{
	hg_object_t *obj2;

	obj = hg_object_string_new(vm, "I like a dog");

	fail_unless(obj != NULL, "Failed to create a string object");
	fail_unless(HG_OBJECT_IS_STRING (obj), "Created object isn't a string object");

	obj2 = hg_object_string_substring_new(vm, obj, 9, 3);

	fail_unless(obj2 != NULL, "Failed to create a substring.");
	fail_unless(HG_OBJECT_IS_STRING (obj2), "Created substring isn't a string object");

	memcpy(HG_OBJECT_STRING_DATA (obj2)->string, "cat", 3);
	fail_unless(strncmp(HG_OBJECT_STRING_DATA (obj)->string,
			    "I like a cat",
			    HG_OBJECT_STRING (obj)->real_length) == 0, "Failed to modify through a substring");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_string_compare)
{
	hg_object_t *obj2;

	obj = hg_object_string_new(vm, "foo");
	obj2 = hg_object_string_new(vm, "foo");

	fail_unless(obj != NULL, "Failed to create a string object");
	fail_unless(obj2 != NULL, "Failed to create a string object(2)");

	fail_unless(hg_object_string_compare(obj, obj2), "Failed to compare the string objects");

	hg_object_free(vm, obj2);

	obj2 = hg_object_string_new(vm, "bar");

	fail_unless(obj2 != NULL, "Failed to create a string object(3)");

	fail_unless(!hg_object_string_compare(obj, obj2), "Have to be different.");

	hg_object_free(vm, obj2);
	hg_object_free(vm, obj);
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_string_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_object_string_new);
	T (hg_object_string_sized_new);
	T (hg_object_string_substring_new);
	T (hg_object_string_compare);

	suite_add_tcase(s, tc);

	return s;
}
