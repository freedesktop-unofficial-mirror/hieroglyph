/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgname.c
 * Copyright (C) 2010 Akira TAGOH
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

#include "hgmem.h"
#include "hgname.h"
#include "main.h"


hg_mem_t *mem = NULL;
hg_object_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	hg_object_init();
	mem = hg_mem_new(512);
	vtable = hg_object_name_get_vtable();
}

void
teardown(void)
{
	gchar *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
	hg_mem_destroy(mem);
	hg_object_fini();
}

/** test cases **/
TDEF (get_capsulated_size)
{
	gsize size;

	size = vtable->get_capsulated_size();
	fail_unless(size == sizeof (hg_object_name_t), "Obtaining the different size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, sizeof (hg_object_name_t), size);
} TEND

TDEF (hg_object_name_new_with_encoding)
{
	hg_quark_t q;
	hg_object_name_t *v;
	const gchar *n;

	q = hg_object_name_new_with_encoding(mem, HG_enc_END, (gpointer *)&v);
	fail_unless(q == Qnil, "Giving encoding index is supposed to be out of the range. but seems not.");
	q = hg_object_name_new_with_encoding(mem, HG_enc_abs, (gpointer *)&v);
	fail_unless(q != Qnil, "Unable to create the name object.");
	fail_unless(hg_quark_get_type(q) == HG_TYPE_NAME, "No type information in the quark");
	n = hg_object_name_get_name(mem, q);
	fail_unless(n != NULL, "Unable to obtain the real pointer of the name");
	fail_unless(strcmp(n, "abs") == 0, "Obtaining the wrong name: expect: abs actual: %s.", n);
} TEND

TDEF (hg_object_name_new_with_string)
{
} TEND

TDEF (hg_object_name_get_name)
{
} TEND

TDEF (object_to)
{
	/* can be done in initialize testcase */
} TEND

TDEF (object_from)
{
	/* can be done in initialize testcase */
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_object_name_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (get_capsulated_size);
	T (hg_object_name_new_with_string);
	T (hg_object_name_new_with_encoding);
	T (hg_object_name_get_name);
	T (object_to);
	T (object_from);

	suite_add_tcase(s, tc);

	return s;
}
