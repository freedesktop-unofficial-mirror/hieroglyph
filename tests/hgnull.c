/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgnull.c
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
#include "hgnull.h"
#include "main.h"


hg_mem_t *mem = NULL;
hg_object_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	hg_object_init();
	mem = hg_mem_new(512);
	vtable = hg_object_null_get_vtable();
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
	fail_unless(size == sizeof (hg_object_null_t), "Obtaining the different size: expect: %" G_GSIZE_FORMAT " actual: %" G_GSIZE_FORMAT, sizeof (hg_object_null_t), size);
} TEND

TDEF (initialize)
{
	hg_object_null_t *v = NULL, *v2 = NULL;
	hg_quark_t q, q2;
	hg_quark_t qv;

	q = hg_object_null_new(mem, (gpointer *)&v);
	fail_unless(q != Qnil, "Unable to create the null object.");
	qv = hg_object_null_to_qnull(v);
	fail_unless(qv == 0x0000000000000000, "Unexpected result to convert the object to the quark value.");
	q2 = hg_qnull_to_object_null(mem, qv, (gpointer *)&v2);
	fail_unless(q2 != Qnil, "Unable to create the null object (copying)");
	fail_unless(v2 != NULL, "Unable to obtain the pointer of hg_object_null_t");
} TEND

TDEF (free)
{
	/* can be done in initialize testcase */
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
	Suite *s = suite_create("hg_object_null_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (get_capsulated_size);
	T (initialize);
	T (free);
	T (object_to);
	T (object_from);

	suite_add_tcase(s, tc);

	return s;
}
