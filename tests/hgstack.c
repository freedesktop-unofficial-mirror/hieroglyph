/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
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
#include "hgint.h"
#include "hgstack.h"
#include "hgstack-private.h"
#include "hgmem.h"


hg_mem_t *mem = NULL;
hg_object_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	hg_object_init();
	HG_STACK_INIT;
	mem = hg_mem_new(HG_MEM_TYPE_LOCAL, 100000);
	vtable = hg_object_stack_get_vtable();
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
	fail_unless(size == sizeof (hg_stack_t), "Obtaining the different size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, sizeof (hg_stack_t), size);
} TEND

TDEF (hg_stack_new)
{
	hg_stack_t *s;

	s = hg_stack_new(mem, 10, NULL);
	fail_unless(s != NULL, "Unable to create a stack object");
} TEND

TDEF (hg_stack_set_validation)
{
	hg_stack_t *s;

	s = hg_stack_new(mem, 1, NULL);
	fail_unless(s != NULL, "Unable to create a stack object");
	fail_unless(hg_stack_push(s, Qnil), "Unexpected result to push a value into the stack");
	fail_unless(hg_stack_depth(s) == 1, "Unexpected result to obtain the current depth on the stack");
	fail_unless(!hg_stack_push(s, Qnil), "Unexpected result to push more than the max depth");
	hg_stack_set_validation(s, FALSE);
	fail_unless(hg_stack_push(s, Qnil), "Unexpected result to push more than the max depth without the validation");
	fail_unless(hg_stack_depth(s) == 2, "Unexpected result to obtain the current depth on the stack [take 2]");
} TEND

TDEF (hg_stack_depth)
{
	/* should be done the above */
} TEND

TDEF (hg_stack_push)
{
	/* should be done the above */
} TEND

TDEF (hg_stack_pop)
{
	hg_stack_t *s;

	s = hg_stack_new(mem, 5, NULL);
	fail_unless(s != NULL, "Unable to create a stack object");
	fail_unless(hg_stack_push(s, HG_QINT (5)), "Unexpected result to push a value into the stack");
	fail_unless(hg_stack_depth(s) == 1, "Unexpected result to obtain the current depth on the stack");
	fail_unless(hg_stack_push(s, HG_QINT (6)), "Unexpected result to push a value into the stack [take 2]");
	fail_unless(hg_stack_index(s, 1, NULL) == HG_QINT (5), "Unexpected result to obtain a value with the index.");
	fail_unless(hg_stack_pop(s, NULL) == HG_QINT (6), "Unexpected result to pop a value from the stack");
	hg_stack_clear(s);
	fail_unless(hg_stack_depth(s) == 0, "Unexpected result to obtain the current depth after cleaning up");
} TEND

TDEF (hg_stack_clear)
{
	/* should be done the above */
} TEND

TDEF (hg_stack_index)
{
	/* should be done the above */
} TEND

static gboolean
_s2s(hg_mem_t    *mem,
     hg_quark_t   q,
     gpointer     data,
     GError     **error)
{
	GString *s = data;

	if (q == Qnil)
		g_string_append(s, "nil ");
	else
		g_string_append_printf(s, "%" G_GSIZE_FORMAT " ", hg_quark_get_value(q));

	return TRUE;
}

TDEF (hg_stack_roll)
{
	hg_stack_t *s;
	gsize i;
	GString *str = g_string_new(NULL);

	s = hg_stack_new(mem, 10, NULL);
	fail_unless(s != NULL, "Unable to create a stack object");
	for (i = 1; i < 6; i++) {
		fail_unless(hg_stack_push(s, HG_QINT (i)), "Unexpected result to push a value into the stack: %" G_GSIZE_FORMAT, i);
	}
	hg_stack_roll(s, 5, 1, NULL);
	hg_stack_foreach(s, _s2s, str, FALSE, NULL);
	fail_unless(strcmp(str->str, "4 3 2 1 5 ") == 0, "Unexpected result to roll up: actual: %s", str->str);
	hg_stack_roll(s, 5, -1, NULL);
	g_string_erase(str, 0, -1);
	hg_stack_foreach(s, _s2s, str, FALSE, NULL);
	fail_unless(strcmp(str->str, "5 4 3 2 1 ") == 0, "Unexpected result to roll down: actual: %s", str->str);
	fail_unless(hg_stack_push(s, HG_QINT (6)), "Unexpected result to push a value into the stack: %" G_GSIZE_FORMAT, 6);

	hg_stack_roll(s, 5, 1, NULL);
	g_string_erase(str, 0, -1);
	hg_stack_foreach(s, _s2s, str, FALSE, NULL);
	fail_unless(strcmp(str->str, "5 4 3 2 6 1 ") == 0, "Unexpected result to roll up [take 2]: actual: %s", str->str);
	hg_stack_roll(s, 5, -1, NULL);
	g_string_erase(str, 0, -1);
	hg_stack_foreach(s, _s2s, str, FALSE, NULL);
	fail_unless(strcmp(str->str, "6 5 4 3 2 1 ") == 0, "Unexpected result to roll down [take 2]: actual: %s", str->str);

	hg_stack_roll(s, 5, 2, NULL);
	g_string_erase(str, 0, -1);
	hg_stack_foreach(s, _s2s, str, FALSE, NULL);
	fail_unless(strcmp(str->str, "4 3 2 6 5 1 ") == 0, "Unexpected result to roll up [take 3]: actual: %s", str->str);
	hg_stack_roll(s, 5, -2, NULL);
	g_string_erase(str, 0, -1);
	hg_stack_foreach(s, _s2s, str, FALSE, NULL);
	fail_unless(strcmp(str->str, "6 5 4 3 2 1 ") == 0, "Unexpected result to roll down [take 3]: actual: %s", str->str);
} TEND

TDEF (hg_stack_foreach)
{
	/* should be done the above */
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgstack.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (get_capsulated_size);
	T (hg_stack_new);
	T (hg_stack_set_validation);
	T (hg_stack_depth);
	T (hg_stack_push);
	T (hg_stack_pop);
	T (hg_stack_clear);
	T (hg_stack_index);
	T (hg_stack_roll);
	T (hg_stack_foreach);

	suite_add_tcase(s, tc);

	return s;
}
