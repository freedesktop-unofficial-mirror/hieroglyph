/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.c
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
#include "hgstring.h"
#include "hgmem.h"


hg_mem_t *mem = NULL;
hg_object_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	hg_object_init();
	HG_STRING_INIT;
	mem = hg_mem_new(HG_MEM_TYPE_LOCAL, 100000);
	vtable = hg_object_string_get_vtable();
}

void
teardown(void)
{
	gchar *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
	hg_object_tini();
	hg_mem_destroy(mem);
}

/** test cases **/
TDEF (get_capsulated_size)
{
	gsize size;

	size = vtable->get_capsulated_size();
	fail_unless(size == sizeof (hg_string_t), "Obtaining the different size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, sizeof (hg_string_t), size);
} TEND

static hg_error_t
_gc_iter_func(hg_quark_t   qdata,
	      gpointer     data)
{
	return HG_ERROR_ (HG_STATUS_SUCCESS, 0);
}

static hg_error_t
_gc_func(hg_mem_t     *mem,
	 hg_pointer_t  data)
{
	hg_string_t *s = data;

	if (data == NULL)
		return HG_ERROR_ (HG_STATUS_SUCCESS, 0);

	return hg_object_gc_mark((hg_object_t *)s, _gc_iter_func, NULL);
}

TDEF (gc_mark)
{
	hg_quark_t q;
	hg_string_t *s;
	gssize size = 0;
	hg_mem_t *m = hg_mem_new(HG_MEM_TYPE_LOCAL, 256);

	q = hg_string_new_with_value(m, "abc", -1, (gpointer *)&s);
	hg_mem_set_garbage_collector(m, _gc_func, s);
	size = hg_mem_collect_garbage(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed", size);
} TEND

TDEF (hg_string_new)
{
	hg_quark_t q;
	hg_string_t *s = NULL;

	q = hg_string_new(mem, 100, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_quark_get_type(q) == HG_TYPE_STRING, "No type information in the quark");
	fail_unless(s->allocated_size == 101, "Unexpected result of the allocated size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, 100, s->allocated_size);
	hg_mem_free(mem, q);

/* it's valid now
	q = hg_string_new(mem, 0, NULL);
	fail_unless(q == Qnil, "Unexpected result to create the 0-sized string");
	g_free(hieroglyph_test_pop_error());
*/
	q = hg_string_new(mem, 65536, NULL);
	fail_unless(q == Qnil, "Unexpected result to create too long string");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_string_new_with_value)
{
	hg_quark_t q;
	hg_string_t *s = NULL;

	q = hg_string_new_with_value(mem, NULL, 1, NULL);
	fail_unless(q == Qnil, "Unexpected result to create the string object with NULL string");
	g_free(hieroglyph_test_pop_error());
/* it's valid now
	q = hg_string_new_with_value(mem, "a", 0, NULL);
	fail_unless(q == Qnil, "Unexpected result to create the string object with 0-sized string");
	g_free(hieroglyph_test_pop_error());
*/
	q = hg_string_new_with_value(mem, "a", 65536, NULL);
	fail_unless(q == Qnil, "Unexpected result to create the string object with too long string");
	g_free(hieroglyph_test_pop_error());
	q = hg_string_new_with_value(mem, "abc", 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_quark_get_type(q) == HG_TYPE_STRING, "No type information in the quark");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result of the allocated size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, 3, s->allocated_size);
	fail_unless(hg_string_length(s) == 3, "Unexpected result of the string size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, 3, s->length);

	hg_mem_free(mem, q);
} TEND

TDEF (hg_string_length)
{
	/* should be done in hg_string_new_with_value testing */
} TEND

TDEF (hg_string_maxlength)
{
	/* should be done in hg_string_new_with_value testing */
} TEND

TDEF (hg_string_clear)
{
	hg_quark_t q;
	hg_string_t *s;

	q = hg_string_new_with_value(mem, "abc", 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_string_clear(s), "Unable to clean up the string object.");
	fail_unless(hg_string_length(s) == 0, "Unexpected result after clearing.");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size.");
} TEND

TDEF (hg_string_append_c)
{
	hg_quark_t q;
	hg_string_t *s;

	q = hg_string_new(mem, 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_string_append_c(s, 'a', NULL), "Unable to add a character into the string object.");
	fail_unless(hg_string_length(s) == 1, "Unexpected result after adding a character.");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size");
	fail_unless(hg_string_ncompare_with_cstr(s, "a", 1), "Unexpected result on the string after adding.");
	fail_unless(hg_string_append_c(s, 'b', NULL), "Unable to add a character into the string object. [take 2]");
	fail_unless(hg_string_length(s) == 2, "Unexpected result after adding a character. [take 2]");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size [take 2]");
	fail_unless(hg_string_ncompare_with_cstr(s, "ab", 2), "Unexpected result on the string after adding. [take 2]");
	fail_unless(hg_string_append_c(s, 'c', NULL), "Unable to add a character into the string object. [take 3]");
	fail_unless(hg_string_length(s) == 3, "Unexpected result after adding a character. [take 3]");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size [take 3]");
	fail_unless(hg_string_ncompare_with_cstr(s, "abc", 3), "Unexpected result on the string after adding. [take 3]");
	fail_unless(hg_string_append_c(s, 'd', NULL), "Unable to add a character into the string object. [take 4]");
	fail_unless(hg_string_length(s) == 4, "Unexpected result after adding a character. [take 4]");
	fail_unless(hg_string_maxlength(s) > 3, "Unexpected result on the allocated size [take 4]");
	fail_unless(hg_string_ncompare_with_cstr(s, "abcd", 4), "Unexpected result on the string after adding. [take 4]");
} TEND

TDEF (hg_string_append)
{
	hg_quark_t q;
	hg_string_t *s;

	q = hg_string_new(mem, 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_string_append(s, "ab", -1, NULL), "Unable to add a character into the string object.");
	fail_unless(hg_string_length(s) == 2, "Unexpected result after adding a character.");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size");
	fail_unless(hg_string_ncompare_with_cstr(s, "ab", 2), "Unexpected result on the string after adding.");
	fail_unless(hg_string_append(s, "cd", -1, NULL), "Unable to add a character into the string object. [take 2]");
	fail_unless(hg_string_length(s) == 4, "Unexpected result after adding a character. [take 2]");
	fail_unless(hg_string_maxlength(s) > 3, "Unexpected result on the allocated size [take 2]");
	fail_unless(hg_string_ncompare_with_cstr(s, "abcd", 4), "Unexpected result on the string after adding. [take 2]");
	fail_unless(hg_string_append(s, "efg", 2, NULL), "Unable to add a character into the string object. [take 3]");
	fail_unless(hg_string_length(s) == 6, "Unexpected result after adding a character. [take 3]");
	fail_unless(hg_string_ncompare_with_cstr(s, "abcdef", 6), "Unexpected result on the string after adding. [take 3]");
	fail_unless(!hg_string_append(s, "a", 65536, NULL), "Unexpected result to add too long string.");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_string_overwrite_c)
{
/*
	hg_quark_t q;
	hg_string_t *s;

	q = hg_string_new(mem, 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_string_append_c(s, 'a'), "Unable to add a character into the string object.");
	fail_unless(hg_string_length(s) == 1, "Unexpected result after adding a character.");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size");
	fail_unless(hg_string_append_c(s, 'b'), "Unable to add a character into the string object. [take 2]");
	fail_unless(hg_string_length(s) == 2, "Unexpected result after adding a character. [take 2]");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size [take 2]");
	fail_unless(hg_string_append_c(s, 'c'), "Unable to add a character into the string object. [take 3]");
	fail_unless(hg_string_length(s) == 3, "Unexpected result after adding a character. [take 3]");
	fail_unless(hg_string_maxlength(s) == 3, "Unexpected result on the allocated size [take 3]");
	fail_unless(hg_string_append_c(s, 'd'), "Unable to add a character into the string object. [take 4]");
	fail_unless(hg_string_length(s) == 4, "Unexpected result after adding a character. [take 4]");
	fail_unless(hg_string_maxlength(s) > 3, "Unexpected result on the allocated size [take 4]");
*/
} TEND

TDEF (hg_string_erase)
{
	hg_quark_t q;
	hg_string_t *s;

	q = hg_string_new_with_value(mem, "abcdefg", 7, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	fail_unless(hg_string_ncompare_with_cstr(s, "abcdefg", -1), "Unexpected result after creating the string object.");
	fail_unless(hg_string_erase(s, 2, 2), "Unable to delete the characters");
	fail_unless(hg_string_ncompare_with_cstr(s, "abefg", -1), "Unexpected result after deleting the characters.");
	fail_unless(hg_string_length(s) == 5, "Unexpected result of the string size.");
	fail_unless(hg_string_erase(s, 2, -1), "Unable to delete the characters [take 2]");
	fail_unless(hg_string_ncompare_with_cstr(s, "ab", -1), "Unexpected result after deleting the characters [take 2]");
	fail_unless(hg_string_length(s) == 2, "Unexpected result of the string size [take 2]");
	fail_unless(!hg_string_erase(s, -1, 1), "Unexpected result when giving the negative position.");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_string_concat)
{
	hg_quark_t q1, q2;
	hg_string_t *s1, *s2;

	q1 = hg_string_new_with_value(mem, "abcd", 4, (gpointer *)&s1);
	q2 = hg_string_new_with_value(mem, "efgh", 4, (gpointer *)&s2);
	fail_unless(q1 != Qnil, "Unable to create the string object [1]");
	fail_unless(q2 != Qnil, "Unable to create the string object [2]");
	fail_unless(!hg_string_concat(s1, NULL), "Unexpected result on concatenating the string object with NULL. [1]");
	g_free(hieroglyph_test_pop_error());
	fail_unless(!hg_string_concat(NULL, s2), "Unexpected result on concatenating the string object with NULL. [2]");
	g_free(hieroglyph_test_pop_error());
	fail_unless(hg_string_concat(s1, s2), "Unable to concatenate the objects.");
	fail_unless(hg_string_ncompare_with_cstr(s1, "abcdefgh", -1), "Unexpected result after concatenating the objects: cstr in obj: %s", hg_string_get_cstr(s1));
	fail_unless(hg_string_ncompare_with_cstr(s2, "efgh", -1), "Unexpected result after concatenating the objects [2]");
} TEND

TDEF (hg_string_index)
{
	hg_quark_t q;
	hg_string_t *s;
	gchar c;

	q = hg_string_new_with_value(mem, "abc", 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	c = hg_string_index(s, 0);
	fail_unless(c == 'a', "Unexpected result to obtain the character at the index 0: expect: %c, actual: %c", 'a', c);
	c = hg_string_index(s, 1);
	fail_unless(c == 'b', "Unexpected result to obtain the character at the index 0: expect: %c, actual: %c", 'b', c);
	c = hg_string_index(s, 2);
	fail_unless(c == 'c', "Unexpected result to obtain the character at the index 0: expect: %c, actual: %c", 'c', c);
	c = hg_string_index(s, 3);
	fail_unless(c == 0, "Unexpected result to obtain the character at the index 0: expect: %c, actual: %c", 0, c);
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_string_get_cstr)
{
	hg_quark_t q;
	hg_string_t *s;
	gchar *p;

	q = hg_string_new_with_value(mem, "abc", 3, (gpointer *)&s);
	fail_unless(q != Qnil, "Unable to create the string object.");
	p = hg_string_get_cstr(s);
	fail_unless(strcmp(p, "abc") == 0, "Unexpected result of the obtained string");
	g_free(p);
} TEND

TDEF (hg_string_fix_string_size)
{
} TEND

TDEF (hg_string_compare)
{
	hg_quark_t q1, q2;
	hg_string_t *s1, *s2;

	q1 = hg_string_new_with_value(mem, "abc", 3, (gpointer *)&s1);
	q2 = hg_string_new_with_value(mem, "abc", 3, (gpointer *)&s2);
	fail_unless(q1 != Qnil, "Unable to create the string object [1]");
	fail_unless(q2 != Qnil, "Unable to create the string object [2]");
	fail_unless(hg_string_compare(s1, s2), "Unexpected result on comparing the string objects");
	q2 = hg_string_new_with_value(mem, "ABC", 3, (gpointer *)&s2);
	fail_unless(!hg_string_compare(s1, s2), "Unexpected result on comparing the string objects [take 2]");
	q2 = hg_string_new_with_value(mem, "abcd", 4, (gpointer *)&s2);
	fail_unless(!hg_string_compare(s1, s2), "Unexpected result on comparing the string objects [take 3]");
	fail_unless(hg_string_ncompare(s1, s2, 3), "Unexpected result on comparing the string objects [take 4]");
} TEND

TDEF (hg_string_ncompare)
{
	/* should be done on hg_string_compare */
} TEND

TDEF (hg_string_ncompare_with_cstr)
{
	/* should be done anywhere on the above testing */
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgstring.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (get_capsulated_size);
	T (gc_mark);
	T (hg_string_new);
	T (hg_string_new_with_value);
	T (hg_string_length);
	T (hg_string_maxlength);
	T (hg_string_clear);
	T (hg_string_append_c);
	T (hg_string_append);
	T (hg_string_overwrite_c);
	T (hg_string_erase);
	T (hg_string_concat);
	T (hg_string_index);
	T (hg_string_get_cstr);
	T (hg_string_fix_string_size);
	T (hg_string_compare);
	T (hg_string_ncompare);
	T (hg_string_ncompare_with_cstr);

	suite_add_tcase(s, tc);

	return s;
}
