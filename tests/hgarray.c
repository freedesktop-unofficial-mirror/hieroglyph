/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
 * Copyright (C) 2010-2011 Akira TAGOH
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

#include "main.h"
#include "hgarray.h"
#include "hgint.h"
#include "hgmem.h"
#include "hgnull.h"


hg_mem_t *mem = NULL;
hg_object_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	hg_object_init();
	HG_ARRAY_INIT;
	mem = hg_mem_spool_new(HG_MEM_TYPE_LOCAL, 100000);
	vtable = hg_object_array_get_vtable();
}

void
teardown(void)
{
	hg_char_t *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
	hg_object_tini();
	hg_mem_spool_destroy(mem);
}

/** test cases **/
TDEF (get_capsulated_size)
{
	hg_usize_t size;

	size = vtable->get_capsulated_size();
	fail_unless(size == sizeof (hg_array_t), "Obtaining the different size: expect: %lx actual: %lx", sizeof (hg_array_t), size);
} TEND

TDEF (gc_mark)
{
	hg_quark_t q HG_GNUC_UNUSED;
	hg_array_t *a;
	hg_size_t size = 0;
	hg_mem_t *m = hg_mem_spool_new(HG_MEM_TYPE_LOCAL, 256);

	q = hg_array_new(m, 10, (hg_pointer_t *)&a);
	fail_unless(hg_array_set(a, HG_QINT (1), 9, FALSE), "Unable to put a value into the array");
	size = hg_mem_spool_run_gc(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed [take 1]", size);
	hg_mem_spool_set_gc_procedure(m, NULL, NULL);
	size = hg_mem_spool_run_gc(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed [take 2]", size);
	size = hg_mem_spool_run_gc(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed [take 3]", size);
} TEND

TDEF (hg_array_new)
{
	hg_quark_t q;

	q = hg_array_new(mem, 65536, NULL);
	fail_unless(q == Qnil, "Unexpected result to create too big array.");
	g_free(hieroglyph_test_pop_error());
	q = hg_array_new(mem, 0, NULL);
	fail_unless(q != Qnil, "Unexpected result to create small array");
} TEND

TDEF (hg_array_set)
{
	hg_quark_t q;
	hg_array_t *a;

	q = hg_array_new(mem, 5, (hg_pointer_t *)&a);
	fail_unless(q != Qnil, "Unable to create an array object");
	fail_unless(hg_array_length(a) == 0, "Unexpected result to obtain the size of the array.");
	fail_unless(hg_array_maxlength(a) == 5, "Unexpected result to obtain the allocated size of the array.");
	fail_unless(hg_array_set(a, HG_QINT (1), 0, FALSE), "Unable to put a value into the array.");
	fail_unless(hg_array_length(a) == 1, "Unexpected result to obtain the size of the array after putting a value");
	fail_unless(hg_array_get(a, 0) == HG_QINT (1), "Unexpected result to obtain the value in the array.");

	q = hg_array_new(mem, 0, (hg_pointer_t *)&a);
	fail_unless(q != Qnil, "Unable to create an array object");
	fail_unless(hg_array_length(a) == 0, "Unexpected result to obtain the size of the 0-array.");
	fail_unless(hg_array_maxlength(a) == 0, "Unexpected result to obtain the allocated size of the 0-array.");
	fail_unless(!hg_array_set(a, HG_QINT (1), 0, FALSE), "Unexpected result to put a value into the 0-array.");
	g_free(hieroglyph_test_pop_error());
	fail_unless(hg_array_get(a, 0) == Qnil, "Unexpected result to obtain the value from the 0-array.");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_array_get)
{
	/* should be done the above */
} TEND

static hg_bool_t
_a2s(hg_mem_t     *mem,
     hg_quark_t    q,
     hg_pointer_t  data)
{
	GString *s = data;

	if (q == Qnil)
		g_string_append(s, "nil ");
	else if (HG_IS_QNULL (q))
		g_string_append(s, "null ");
	else
		g_string_append_printf(s, "%ld ", hg_quark_get_value(q));

	return TRUE;
}

TDEF (hg_array_insert)
{
	hg_quark_t q, t;
	hg_array_t *a;
	GString *str = g_string_new(NULL);

	q = hg_array_new(mem, 8, (hg_pointer_t *)&a);
	fail_unless(q != Qnil, "Unable to create an array object");
	fail_unless(hg_array_insert(a, HG_QINT(2), 0), "Unable to insert a value into the array"); /* [2] */
	fail_unless(hg_array_length(a) == 1, "Unexpected result to obtain the current size of the array.");
	fail_unless(hg_array_insert(a, HG_QINT(3), 0), "Unable to insert a value into the array [take 2]"); /* [3 2] */
	fail_unless(hg_array_length(a) == 2, "Unexpected result to obtain the current size of the array. [take 2]");
	fail_unless(hg_array_get(a, 1) == HG_QINT(2), "Unexpected result on shifting the content of the array after inserting.");
	fail_unless(hg_array_insert(a, HG_QINT(4), 3), "Unable to insert a value into the array [take 3]"); /* [3 2 nil 4] */
	fail_unless(hg_array_length(a) == 4, "Unexpected result to obtain the current size of the array. [take 3]");
	fail_unless(hg_array_insert(a, HG_QINT(5), 1), "Unable to insert a value into the array [take 4]"); /* [3 5 2 nil 4] */
	t = hg_array_get(a, 2);
	fail_unless(t == HG_QINT(2), "Unexpected result on shifting the content of the array after inserting [take 4]: actual: %ld", hg_quark_get_value(t));
	t = hg_array_get(a, 3);
	fail_unless(HG_IS_QNULL (t) && HG_ERROR_IS_SUCCESS0 (), "Unexpected result to obtain 3rd element in the array.");
	hg_array_foreach(a, _a2s, str);
	fail_unless(strcmp(str->str, "3 5 2 null 4 ") == 0, "Unexpected result on converting the array to the string: actual: %s", str->str);
	fail_unless(!hg_array_remove(a, 5), "Unexpected result to remove an element outside of the array.");
	g_free(hieroglyph_test_pop_error());
	fail_unless(hg_array_remove(a, 1), "Unable to remove an element from the array."); /* [3 2 nil 4] */
	fail_unless(hg_array_length(a) == 4, "Unexpected result to obtain the current size of the array after removing.");
	fail_unless(hg_array_get(a, 1) == HG_QINT (2), "Unexpected result on the value after removing.");
} TEND

TDEF (hg_array_remove)
{
	/* should be done the above */
} TEND

TDEF (hg_array_length)
{
	/* should be done the above */
} TEND

TDEF (hg_array_maxlength)
{
	/* should be done the above */
} TEND

TDEF (hg_array_foreach)
{
	/* should be done the above */
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgarray.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (get_capsulated_size);
	T (gc_mark);
	T (hg_array_new);
	T (hg_array_set);
	T (hg_array_get);
	T (hg_array_insert);
	T (hg_array_remove);
	T (hg_array_length);
	T (hg_array_maxlength);
	T (hg_array_foreach);

	suite_add_tcase(s, tc);

	return s;
}
