/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.c
 * Copyright (C) 2007-2009 Akira TAGOH
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

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");

	hg_object_free(vm, obj);

	obj = hg_object_new(vm, 10);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 10, "The amount of the object is different.");

	hg_object_free(vm, obj);

	/* disable stacktrace */
	if (hg_is_stacktrace_enabled)
		flag = hg_is_stacktrace_enabled();
	hg_use_stacktrace(FALSE);
	/* disable warnings */
	hg_quiet_warning_messages(TRUE);

	obj = hg_object_new(vm, 0);
	if (hg_is_stacktrace_enabled)
		hg_use_stacktrace(flag);
	hg_quiet_warning_messages(FALSE);

	fail_unless(obj == NULL, "Not allowed to create empty object");
}
TEND

TDEF (hg_object_sized_new)
{
	obj = hg_object_sized_new(vm, 10);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");
	fail_unless(HG_OBJECT_HEADER (obj)->total_length == hg_n_alignof (hg_n_alignof (sizeof (hg_object_header_t) + sizeof (_hg_object_t)) + 10), "The length of the object is different.");

	hg_object_free(vm, obj);

	obj = hg_object_sized_new(vm, 0);

	TNUL (obj);
	fail_unless(HG_OBJECT_GET_N_OBJECTS (obj) == 1, "The amount of the object is different.");
	fail_unless(HG_OBJECT_HEADER (obj)->total_length == hg_n_alignof (sizeof (hg_object_header_t) + sizeof (_hg_object_t)), "The length of the object is different.");

	hg_object_free(vm, obj);
}
TEND

TDEF (hg_object_dup)
{
	hg_object_t *obj2;

	obj = hg_object_new(vm, 1);

	TNUL (obj);
	/* disable warning messages */
	hg_quiet_warning_messages(TRUE);
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 == NULL, "Not allowed to duplicate the uninitialized object");
	hg_quiet_warning_messages(FALSE);

	hg_object_free(vm, obj);

	/* null */
	obj = hg_object_null_new(vm);

	fail_unless(obj != NULL, "Failed to create a null object");
	fail_unless(HG_OBJECT_IS_NULL (obj), "Created object isn't a null object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a null object");
	fail_unless(HG_OBJECT_IS_NULL (obj2), "Duplicated object isn't a null object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* integer */
	obj = hg_object_integer_new(vm, 123);

	fail_unless(obj != NULL, "Failed to create an integer object");
	fail_unless(HG_OBJECT_IS_INTEGER (obj), "Created object isn't an integer object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate an integer object");
	fail_unless(HG_OBJECT_IS_INTEGER (obj2), "Duplicated object isn't an integer object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* real */
	obj = hg_object_real_new(vm, 0.01);

	fail_unless(obj != NULL, "Failed to create an real object");
	fail_unless(HG_OBJECT_IS_REAL (obj), "Created object isn't an real object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate an real object");
	fail_unless(HG_OBJECT_IS_REAL (obj2), "Duplicated object isn't an real object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* name */
	obj = hg_object_name_new(vm, "foo", FALSE);

	fail_unless(obj != NULL, "Failed to create a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj), "Created object isn't a name object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj2), "Duplicated object isn't a name object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* name eval */
	obj = hg_object_name_new(vm, "foo", TRUE);

	fail_unless(obj != NULL, "Failed to create a name(eval) object");
	fail_unless(HG_OBJECT_IS_EVAL (obj), "Created object isn't a name(eval) object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a name object");
	fail_unless(HG_OBJECT_IS_EVAL (obj2), "Duplicated object isn't a name(eval) object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* system encoding */
	obj = hg_object_system_encoding_new(vm, HG_enc_abs, FALSE);

	fail_unless(obj != NULL, "Failed to create a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj), "Created object isn't a name object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj2), "Duplicated object isn't a name object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* system encoding eval */
	obj = hg_object_system_encoding_new(vm, HG_enc_abs, TRUE);

	fail_unless(obj != NULL, "Failed to create a name(eval) object");
	fail_unless(HG_OBJECT_IS_EVAL (obj), "Created object isn't a name(eval) object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a name(eval) object");
	fail_unless(HG_OBJECT_IS_EVAL (obj2), "Duplicated object isn't a name(eval) object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* boolean */
	obj = hg_object_boolean_new(vm, TRUE);

	fail_unless(obj != NULL, "Failed to create a boolean object");
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj), "Created object isn't a boolean object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a boolean object");
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj2), "Duplicated object isn't a boolean object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* mark */
	obj = hg_object_mark_new(vm);

	fail_unless(obj != NULL, "Failed to create a mark object");
	fail_unless(HG_OBJECT_IS_MARK (obj), "Created object isn't a mark object");
	obj2 = hg_object_dup(vm, obj);
	fail_unless(obj2 != NULL, "Failed to duplicate a mark object");
	fail_unless(HG_OBJECT_IS_MARK (obj2), "Duplicated object isn't a mark object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly duplicated");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_object_copy)
{
	hg_object_t *obj2;

	obj = hg_object_new(vm, 1);

	TNUL (obj);
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy an uninitialized object");

	hg_object_free(vm, obj);

	/* null */
	obj = hg_object_null_new(vm);

	fail_unless(obj != NULL, "Failed to create a null object");
	fail_unless(HG_OBJECT_IS_NULL (obj), "Created object isn't a null object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a null object");
	fail_unless(HG_OBJECT_IS_NULL (obj2), "Copied object isn't a null object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* integer */
	obj = hg_object_integer_new(vm, 123);

	fail_unless(obj != NULL, "Failed to create an integer object");
	fail_unless(HG_OBJECT_IS_INTEGER (obj), "Created object isn't an integer object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy an integer object");
	fail_unless(HG_OBJECT_IS_INTEGER (obj2), "Copied object isn't an integer object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* real */
	obj = hg_object_real_new(vm, 0.01);

	fail_unless(obj != NULL, "Failed to create an real object");
	fail_unless(HG_OBJECT_IS_REAL (obj), "Created object isn't an real object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy an real object");
	fail_unless(HG_OBJECT_IS_REAL (obj2), "Copied object isn't an real object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* name */
	obj = hg_object_name_new(vm, "foo", FALSE);

	fail_unless(obj != NULL, "Failed to create a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj), "Created object isn't a name object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj2), "Copied object isn't a name object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* name eval */
	obj = hg_object_name_new(vm, "foo", TRUE);

	fail_unless(obj != NULL, "Failed to create a name(eval) object");
	fail_unless(HG_OBJECT_IS_EVAL (obj), "Created object isn't a name(eval) object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a name object");
	fail_unless(HG_OBJECT_IS_EVAL (obj2), "Copied object isn't a name(eval) object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* system encoding */
	obj = hg_object_system_encoding_new(vm, HG_enc_abs, FALSE);

	fail_unless(obj != NULL, "Failed to create a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj), "Created object isn't a name object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj2), "Copied object isn't a name object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* system encoding eval */
	obj = hg_object_system_encoding_new(vm, HG_enc_abs, TRUE);

	fail_unless(obj != NULL, "Failed to create a name(eval) object");
	fail_unless(HG_OBJECT_IS_EVAL (obj), "Created object isn't a name(eval) object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a name(eval) object");
	fail_unless(HG_OBJECT_IS_EVAL (obj2), "Copied object isn't a name(eval) object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* boolean */
	obj = hg_object_boolean_new(vm, TRUE);

	fail_unless(obj != NULL, "Failed to create a boolean object");
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj), "Created object isn't a boolean object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a boolean object");
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj2), "Copied object isn't a boolean object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);

	/* mark */
	obj = hg_object_mark_new(vm);

	fail_unless(obj != NULL, "Failed to create a mark object");
	fail_unless(HG_OBJECT_IS_MARK (obj), "Created object isn't a mark object");
	obj2 = hg_object_copy(vm, obj);
	fail_unless(obj2 != NULL, "Failed to copy a mark object");
	fail_unless(HG_OBJECT_IS_MARK (obj2), "Copied object isn't a mark object");
	fail_unless(hg_object_compare(obj, obj2), "Object wasn't exactly copied");

	hg_object_free(vm, obj);
	hg_object_free(vm, obj2);
}
TEND

TDEF (hg_object_compare)
{
	/* almost test cases has been done at hg_object_dup/copy */
}
TEND

TDEF (hg_object_dump)
{
	g_print("FIXME: %s\n", __FUNCTION__);
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
	obj = hg_object_real_new(vm, 1);

	fail_unless(obj != NULL, "Failed to create an real object");
	fail_unless(HG_OBJECT_IS_REAL (obj), "Created object isn't an real object");
	fail_unless(HG_OBJECT_REAL_IS_EQUAL_TO (obj, 1), "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
	obj = hg_object_real_new(vm, -1);

	fail_unless(obj != NULL, "Failed to create an real object");
	fail_unless(HG_OBJECT_IS_REAL (obj), "Created object isn't an real object");
	fail_unless(HG_OBJECT_REAL_IS_EQUAL_TO (obj, -1), "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
}
TEND

/* name object */
TDEF (hg_object_name_new)
{
	obj = hg_object_name_new(vm, "foo", FALSE);

	fail_unless(obj != NULL, "Failed to create a name object");
	fail_unless(HG_OBJECT_IS_NAME (obj), "Created object isn't a name object");
	fail_unless(hg_object_name_compare_with_raw(obj, "foo"), "Object isn't set correctly on creation");

	hg_object_free(vm, obj);

	obj = hg_object_name_new(vm, "foo", TRUE);

	fail_unless(obj != NULL, "Failed to create a name object");
	fail_unless(HG_OBJECT_IS_EVAL (obj), "Created object isn't a name object");
	fail_unless(hg_object_name_compare_with_raw(obj, "foo"), "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
}
TEND

/* system encoding object */
TDEF (hg_object_system_encoding_new)
{
	hg_vm_initialize(vm);
	obj = hg_object_system_encoding_new(vm, HG_enc_abs, FALSE);

	fail_unless(obj != NULL, "Failed to create a system encoding object");
	fail_unless(HG_OBJECT_IS_NAME (obj), "Created object isn't a system encoding object");
	fail_unless(hg_object_name_compare_with_raw(obj, "abs"), "Object isn't set correctly on creation");

	hg_object_free(vm, obj);

	obj = hg_object_system_encoding_new(vm, HG_enc_abs, TRUE);

	fail_unless(obj != NULL, "Failed to create a system encoding object");
	fail_unless(HG_OBJECT_IS_EVAL (obj), "Created object isn't a system encoding object");
	fail_unless(hg_object_name_compare_with_raw(obj, "abs"), "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
	hg_vm_finalize(vm);
}
TEND

/* boolean object */
TDEF (hg_object_boolean_new)
{
	obj = hg_object_boolean_new(vm, FALSE);

	fail_unless(obj != NULL, "Failed to create a boolean object");
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj), "Created object isn't a boolean object");
	fail_unless(HG_OBJECT_BOOLEAN (obj) == FALSE, "Object isn't set correctly on creation");

	hg_object_free(vm, obj);

	obj = hg_object_boolean_new(vm, TRUE);

	fail_unless(obj != NULL, "Failed to create a boolean object");
	fail_unless(HG_OBJECT_IS_BOOLEAN (obj), "Created object isn't a boolean object");
	fail_unless(HG_OBJECT_BOOLEAN (obj) == TRUE, "Object isn't set correctly on creation");

	hg_object_free(vm, obj);
}
TEND

/* mark object */
TDEF (hg_object_mark_new)
{
	obj = hg_object_mark_new(vm);

	fail_unless(obj != NULL, "Failed to create a mark object");
	fail_unless(HG_OBJECT_IS_MARK (obj), "Created object isn't a mark object");

	hg_object_free(vm, obj);
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
