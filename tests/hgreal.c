/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgreal.c
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
#include "hgreal.h"


/** common **/
void
setup(void)
{
}

void
teardown(void)
{
	gchar *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
}

/** test cases **/
TDEF (new)
{
	hg_quark_t q, q2;
	gdouble f;

	q = HG_QREAL(1.1);
	fail_unless(q == 0x3ff199999999999a, "Unexpected result to create a quark for real: expected: %lx, actual: %lx", 0x3ff199999999999a, q);
	f = HG_REAL(q);
	fail_unless(HG_REAL_EQUAL(f, 1.1), "Unexpected result to convert a quark to real: expected: %f, actual: %f", 1.1, f);
	q2 = HG_QREAL(1.1);
	fail_unless(q == q2, "Unexpected result to compare quarks for real.");
	q2 = HG_QREAL(1.11L);
	fail_unless(q != q2, "Unexpected result to compare different quarks for real.");
	q = HG_QREAL(-1.1);
	fail_unless(q == 0xbff199999999999a, "Unexpected result to create a quark for integer: expected: %lx, actual: %lx", 0xbff199999999999a, q);
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgreal.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (new);

	suite_add_tcase(s, tc);

	return s;
}
