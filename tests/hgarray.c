/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
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
#include <hieroglyph/hgarray.h>
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

/* array object */
TDEF (hg_object_array_new)
{
	obj = hg_object_array_new(vm, 10);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");
	fail_unless(HG_OBJECT_IS_ARRAY (obj), "Object isn't an array.");
	fail_unless(HG_OBJECT_ARRAY (obj)->length == 10, "The amount of the array is different.");

	hg_object_free(vm, obj);

	obj = hg_object_array_new(vm, 0);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");
	fail_unless(HG_OBJECT_IS_ARRAY (obj), "Object isn't an array.");
	fail_unless(HG_OBJECT_ARRAY (obj)->length == 0, "The amount of the array is different.");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_array_subarray_new)
{
	gboolean flag;
	hg_object_t *obj2;

	obj = hg_object_array_new(vm, 10);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");
	fail_unless(HG_OBJECT_IS_ARRAY (obj), "Object isn't an array.");
	fail_unless(HG_OBJECT_ARRAY (obj)->length == 10, "The amount of the array is different.");

	obj2 = hg_object_array_subarray_new(vm, obj, 5, 5);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj2) == 1, "The amount of the object is different.");
	fail_unless(HG_OBJECT_IS_ARRAY (obj2), "Object isn't an array.");
	fail_unless(HG_OBJECT_ARRAY (obj2)->length == 0, "The amount of the array is different.");
	fail_unless(HG_OBJECT_ARRAY (obj2)->real_length == 5, "The amount of the array is different.");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	obj = hg_object_new(vm, 1);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");

	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	hg_quiet_warning_messages(TRUE);
	obj2 = hg_object_array_subarray_new(vm, obj, 0, 0);
	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(obj2 == NULL, "Not allowed to create an array object without the parent array object.");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_array_put)
{
	hg_object_t *obj2, *obj3;
	gboolean retval;

	obj = hg_object_array_new(vm, 5);

	TNUL (obj);

	obj2 = hg_object_boolean_new(vm, TRUE);

	TNUL (obj2);
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj2), "Failed to create a boolean object");

	retval = hg_object_array_put(vm, obj, obj2, 0);
	fail_unless(retval == TRUE, "Failed to put an object to the array");
	fail_unless(((hg_object_t **)HG_OBJECT_ARRAY_DATA (obj)->array)[0] == obj2, "Failed to put an object to the array expectedly");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	obj = hg_object_array_new(vm, 10);

	TNUL (obj);

	obj2 = hg_object_array_subarray_new(vm, obj, 5, 5);

	TNUL (obj2);

	obj3 = hg_object_boolean_new(vm, TRUE);

	TNUL (obj3);

	retval = hg_object_array_put(vm, obj2, obj3, 0);
	fail_unless(retval == TRUE, "Failed to put an object to the sub array");
	fail_unless(((hg_object_t **)HG_OBJECT_ARRAY_DATA (obj2)->array)[0] == obj3, "Failed to put an object to the sub array expectedly");
	fail_unless(((hg_object_t **)HG_OBJECT_ARRAY_DATA (obj)->array)[5] == obj3, "Failed to sync between the array and the sub array on putting an object");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj3);
}
TEND

TDEF (hg_object_array_get)
{
	hg_object_t *obj2, *obj3;
	gboolean retval;

	obj = hg_object_array_new(vm, 10);
	TNUL (obj);

	obj2 = hg_object_boolean_new(vm, TRUE);
	TNUL (obj2);

	retval = hg_object_array_put(vm, obj, obj2, 0);
	fail_unless(retval == TRUE, "Failed to put an object to the array");

	obj3 = hg_object_array_get(vm, obj, 0);
	fail_unless(obj3 != NULL, "Failed to get an object from the array");
	fail_unless(hg_object_compare(obj2, obj3), "Failed to get an expected object from the array");
	fail_unless(obj2 != obj3, "Object has to be new one");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj3);

	obj = hg_object_array_new(vm, 10);
	TNUL (obj);

	obj2 = hg_object_array_new(vm, 5);
	TNUL (obj2);

	retval = hg_object_array_put(vm, obj, obj2, 0);
	fail_unless(retval == TRUE, "Failed to put an object to the array");

	obj3 = hg_object_array_get(vm, obj, 0);
	fail_unless(obj3 != NULL, "Failed to get an object from the array");
	fail_unless(hg_object_compare(obj2, obj3), "Failed to get an expected object from the array");
	fail_unless(obj2 == obj3, "Have to return the same object for the complex object");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_object_array_get_const)
{
	hg_object_t *obj2, *obj3;
	gboolean retval;

	obj = hg_object_array_new(vm, 10);
	TNUL (obj);

	obj2 = hg_object_boolean_new(vm, TRUE);
	TNUL (obj2);

	retval = hg_object_array_put(vm, obj, obj2, 0);
	fail_unless(retval == TRUE, "Failed to put an object to the array");

	obj3 = hg_object_array_get_const(obj, 0);
	fail_unless(obj3 != NULL, "Failed to get an object from the array");
	fail_unless(hg_object_compare(obj2, obj3), "Failed to get an expected object from the array");
	fail_unless(obj2 == obj3, "Object has to be the same");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	obj = hg_object_array_new(vm, 10);
	TNUL (obj);

	obj2 = hg_object_array_new(vm, 5);
	TNUL (obj2);

	retval = hg_object_array_put(vm, obj, obj2, 0);
	fail_unless(retval == TRUE, "Failed to put an object to the array");

	obj3 = hg_object_array_get_const(obj, 0);
	fail_unless(obj3 != NULL, "Failed to get an object from the array");
	fail_unless(hg_object_compare(obj2, obj3), "Failed to get an expected object from the array");
	fail_unless(obj2 == obj3, "Have to return the same object for the complex object");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_object_array_compare)
{
	hg_object_t *obj_, *obj1, *obj1_, *obj2, *obj2_;
	gboolean r, r_;

	obj = hg_object_array_new(vm, 2);
	obj_ = hg_object_array_new(vm, 2);

	TNUL (obj);
	TNUL (obj_);

	obj1 = hg_object_boolean_new(vm, TRUE);
	obj1_ = hg_object_boolean_new(vm, TRUE);

	TNUL (obj1);
	TNUL (obj1_);

	obj2 = hg_object_integer_new(vm, 10);
	obj2_ = hg_object_integer_new(vm, 10);

	TNUL (obj2);
	TNUL (obj2_);

	r = hg_object_array_put(vm, obj, obj1, 0);
	r_ = hg_object_array_put(vm, obj_, obj1_, 0);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	r = hg_object_array_put(vm, obj, obj2, 1);
	r_ = hg_object_array_put(vm, obj_, obj2_, 1);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	fail_unless(hg_object_array_compare(obj, obj_), "Failed to compare the objects");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj_);
	hg_object_free(vm, obj1);
	hg_object_free(vm, obj1_);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj2_);

	/* check failing pattern */
	obj = hg_object_array_new(vm, 2);
	obj_ = hg_object_array_new(vm, 2);

	TNUL (obj);
	TNUL (obj_);

	obj1 = hg_object_boolean_new(vm, TRUE);
	obj1_ = hg_object_boolean_new(vm, TRUE);

	TNUL (obj1);
	TNUL (obj1_);

	obj2 = hg_object_integer_new(vm, 10);
	obj2_ = hg_object_integer_new(vm, 1);

	TNUL (obj2);
	TNUL (obj2_);

	r = hg_object_array_put(vm, obj, obj1, 0);
	r_ = hg_object_array_put(vm, obj_, obj1_, 0);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	r = hg_object_array_put(vm, obj, obj2, 1);
	r_ = hg_object_array_put(vm, obj_, obj2_, 1);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	fail_unless(!hg_object_array_compare(obj, obj_), "Failed to compare the objects");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj_);
	hg_object_free(vm, obj1);
	hg_object_free(vm, obj1_);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj2_);

	/* check with the complex object */
	obj = hg_object_array_new(vm, 2);
	obj_ = hg_object_array_new(vm, 2);

	TNUL (obj);
	TNUL (obj_);

	obj1 = hg_object_boolean_new(vm, TRUE);
	obj1_ = hg_object_boolean_new(vm, TRUE);

	TNUL (obj1);
	TNUL (obj1_);

	obj2 = hg_object_array_new(vm, 0);
	obj2_ = hg_object_array_new(vm, 0);

	TNUL (obj2);
	TNUL (obj2_);

	r = hg_object_array_put(vm, obj, obj1, 0);
	r_ = hg_object_array_put(vm, obj_, obj1_, 0);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	r = hg_object_array_put(vm, obj, obj2, 1);
	r_ = hg_object_array_put(vm, obj_, obj2_, 1);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	fail_unless(hg_object_array_compare(obj, obj_), "Failed to compare the objects");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj_);
	hg_object_free(vm, obj1);
	hg_object_free(vm, obj1_);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj2_);

	/* check the circular reference */
	obj = hg_object_array_new(vm, 2);
	obj_ = hg_object_array_new(vm, 2);

	TNUL (obj);
	TNUL (obj_);

	obj1 = hg_object_boolean_new(vm, TRUE);
	obj1_ = hg_object_boolean_new(vm, TRUE);

	TNUL (obj1);
	TNUL (obj1_);

	obj2 = obj;
	obj2_ = obj_;

	r = hg_object_array_put(vm, obj, obj1, 0);
	r_ = hg_object_array_put(vm, obj_, obj1_, 0);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	r = hg_object_array_put(vm, obj, obj2, 1);
	r_ = hg_object_array_put(vm, obj_, obj2_, 1);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	fail_unless(hg_object_array_compare(obj, obj_), "Failed to compare the objects");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj_);
	hg_object_free(vm, obj1);
	hg_object_free(vm, obj1_);

	/* check the circular reference (failing pattern) */
	obj = hg_object_array_new(vm, 2);
	obj_ = hg_object_array_new(vm, 2);

	TNUL (obj);
	TNUL (obj_);

	obj1 = hg_object_boolean_new(vm, TRUE);
	obj1_ = hg_object_boolean_new(vm, FALSE);

	TNUL (obj1);
	TNUL (obj1_);

	obj2 = obj;
	obj2_ = obj_;

	r = hg_object_array_put(vm, obj, obj1, 0);
	r_ = hg_object_array_put(vm, obj_, obj1_, 0);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	r = hg_object_array_put(vm, obj, obj2, 1);
	r_ = hg_object_array_put(vm, obj_, obj2_, 1);

	fail_unless(r, "Failed to put an object to the array");
	fail_unless(r_, "Failed to put an object to the array");

	fail_unless(!hg_object_array_compare(obj, obj_), "Failed to compare the objects");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj_);
	hg_object_free(vm, obj1);
	hg_object_free(vm, obj1_);
}
TEND

TDEF (hg_object_array_dump)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_array_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_object_array_new);
	T (hg_object_array_subarray_new);
	T (hg_object_array_put);
	T (hg_object_array_get);
	T (hg_object_array_get_const);
	T (hg_object_array_compare);
	T (hg_object_array_dump);

	suite_add_tcase(s, tc);

	return s;
}
