/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.c
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
#include <hieroglyph/hgdict.h>
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

/* dict object */
TDEF (hg_object_dict_new)
{
	obj = hg_object_dict_new(vm, 10);
	TNUL (obj);
	fail_unless(HG_OBJECT_IS_DICT (obj), "Object isn't a dict object");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_dict_compare)
{
	hg_object_t *obj2;

	obj = hg_object_dict_new(vm, 10);
	obj2 = hg_object_dict_new(vm, 10);

	TNUL (obj);
	TNUL (obj2);
	fail_unless(!hg_object_dict_compare(obj, obj2), "Different allocation has to be failed");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	obj = hg_object_dict_new(vm, 10);
	obj2 = hg_object_dup(vm, obj);

	TNUL (obj);
	TNUL (obj2);
	fail_unless(hg_object_dict_compare(obj, obj2), "Failed to compare the dict objects");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_dict_dump)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_dict_insert)
{
	hg_object_t *obj1, *obj1_, *obj2, *obj2_, *obj3, *obj3_, *obj4, *obj4_, *obj_;
	gboolean allocmode = hg_vm_get_current_allocation_mode(vm);

	/* objects allocated from the global pool */
	hg_vm_set_current_allocation_mode(vm, TRUE);

	obj = hg_object_dict_new(vm, 10);
	TNUL (obj);

	obj1 = hg_object_name_new(vm, "bool", FALSE);
	obj1_ = hg_object_boolean_new(vm, TRUE);
	obj2 = hg_object_name_new(vm, "int", FALSE);
	obj2_ = hg_object_integer_new(vm, 10);
	obj3 = hg_object_name_new(vm, "name", FALSE);
	obj3_ = hg_object_name_new(vm, "foo", FALSE);
	TNUL (obj1);
	TNUL (obj1_);
	TNUL (obj2);
	TNUL (obj2_);
	TNUL (obj3);
	TNUL (obj3_);

	fail_unless(hg_object_dict_insert(obj, obj1, obj1_), "Failed to insert an object (bool)");
	fail_unless(hg_object_dict_insert(obj, obj2, obj2_), "Failed to insert an object (int)");
	fail_unless(hg_object_dict_insert(obj, obj3, obj3_), "Failed to insert an object (named)");

	fail_unless((obj_ = hg_object_dict_lookup(obj, obj1)) != NULL, "Failed to look up the object (bool)");
	fail_unless(hg_object_compare(obj1_, obj_), "Failed to compare the object after looking up (bool)");
	fail_unless((obj_ = hg_object_dict_lookup(obj, obj2)) != NULL, "Failed to look up the object (int)");
	fail_unless(hg_object_compare(obj2_, obj_), "Failed to compare the object after looking up (int)");
	fail_unless((obj_ = hg_object_dict_lookup(obj, obj3)) != NULL, "Failed to look up the object (named)");
	fail_unless(hg_object_compare(obj3_, obj_), "Failed to compare the object after looking up (named)");

	/* Insert object allocated in the local pool to the global object */
	hg_vm_set_current_allocation_mode(vm, FALSE);

	obj4 = hg_object_name_new(vm, "bool2", FALSE);
	obj4_ = hg_object_boolean_new(vm, TRUE);
	TNUL (obj4);
	TNUL (obj4_);
	fail_unless(!hg_object_dict_insert(obj, obj4, obj4_), "Have to fail inserting the local object into the global object.");
	
	hg_object_free(vm, obj);
	hg_object_free(vm, obj4_);
	hg_object_free(vm, obj4);
	hg_object_free(vm, obj3_);
	hg_object_free(vm, obj3);
	hg_object_free(vm, obj2_);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj1_);
	hg_object_free(vm, obj1);

	/* objects allocated from the local pool */
	hg_vm_set_current_allocation_mode(vm, FALSE);

	obj = hg_object_dict_new(vm, 10);
	TNUL (obj);
	fail_unless(!HG_OBJECT_ATTR_IS_GLOBAL (obj), "Object wasn't allocated from the local pool.");

	obj1 = hg_object_name_new(vm, "bool", FALSE);
	obj1_ = hg_object_boolean_new(vm, TRUE);
	obj2 = hg_object_name_new(vm, "int", FALSE);
	obj2_ = hg_object_integer_new(vm, 10);
	obj3 = hg_object_name_new(vm, "name", FALSE);
	obj3_ = hg_object_name_new(vm, "foo", FALSE);
	TNUL (obj1);
	TNUL (obj1_);
	TNUL (obj2);
	TNUL (obj2_);
	TNUL (obj3);
	TNUL (obj3_);

	fail_unless(hg_object_dict_insert(obj, obj1, obj1_), "Failed to insert an object (bool)");
	fail_unless(hg_object_dict_insert(obj, obj2, obj2_), "Failed to insert an object (int)");
	fail_unless(hg_object_dict_insert(obj, obj3, obj3_), "Failed to insert an object (named)");

	fail_unless((obj_ = hg_object_dict_lookup(obj, obj1)) != NULL, "Failed to look up the object (bool)");
	fail_unless(hg_object_compare(obj1_, obj_), "Failed to compare the object after looking up (bool)");
	fail_unless((obj_ = hg_object_dict_lookup(obj, obj2)) != NULL, "Failed to look up the object (int)");
	fail_unless(hg_object_compare(obj2_, obj_), "Failed to compare the object after looking up (int)");
	fail_unless((obj_ = hg_object_dict_lookup(obj, obj3)) != NULL, "Failed to look up the object (named)");
	fail_unless(hg_object_compare(obj3_, obj_), "Failed to compare the object after looking up (named)");

	/* Insert object allocated in the global pool to the local object */
	hg_vm_set_current_allocation_mode(vm, TRUE);

	obj4 = hg_object_name_new(vm, "bool2", FALSE);
	obj4_ = hg_object_boolean_new(vm, TRUE);
	TNUL (obj4);
	TNUL (obj4_);
	fail_unless(HG_OBJECT_ATTR_IS_GLOBAL (obj4), "Object wasn't allocated from the global pool.");
	fail_unless(HG_OBJECT_ATTR_IS_GLOBAL (obj4_), "Object wasn't allocated from the global pool.");
	fail_unless(hg_object_dict_insert(obj, obj4, obj4_), "Don't have to fail inserting the global object into the local object.");
	
	hg_object_free(vm, obj);
	hg_object_free(vm, obj4_);
	hg_object_free(vm, obj4);
	hg_object_free(vm, obj3_);
	hg_object_free(vm, obj3);
	hg_object_free(vm, obj2_);
	hg_object_free(vm, obj2);
	hg_object_free(vm, obj1_);
	hg_object_free(vm, obj1);

	/* recover the allocation mode */
	hg_vm_set_current_allocation_mode(vm, allocmode);
}
TEND

TDEF (hg_object_dict_insert_without_consistency)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_dict_replace)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_dict_replace_without_consistency)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_dict_remove)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_dict_lookup)
{
	/* all the testcases should be in insert/replace testing.
	 * so there are nothing we have to do here so far.
	 */
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_dict_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_object_dict_new);
	T (hg_object_dict_compare);
	T (hg_object_dict_dump);
	T (hg_object_dict_insert);
	T (hg_object_dict_insert_without_consistency);
	T (hg_object_dict_replace);
	T (hg_object_dict_replace_without_consistency);
	T (hg_object_dict_remove);
	T (hg_object_dict_lookup);

	suite_add_tcase(s, tc);

	return s;
}
