/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgquark.c
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

#include "hgquark.h"
#include "main.h"

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
TDEF (hg_quark_new)
{
	hg_quark_t q = hg_quark_new(HG_TYPE_INT, 0);
	fail_unless(q == 0x100000000, "Unexpected result for integer quark");
	q = hg_quark_new(HG_TYPE_INT, -1);
	fail_unless(q == 0x1ffffffff, "Unexpected result for integer quark: expect: -1, actual: %lx", q);
	fail_unless(hg_quark_get_type(q) == 1, "Unexpected result to obtain the type for integer quark");
	fail_unless(hg_quark_is_simple_object(q), "Unexpected result to check if a integer quark is a simple object");
	q = hg_quark_new(HG_TYPE_STRING, 0xdeadbeaf);
	fail_unless(q == 0x5deadbeaf, "Unexpected result for string quark: 0x5deadbeaf, actual: %lx", q);
	fail_unless(hg_quark_get_type(q) == HG_TYPE_STRING, "Unexpected result ot obtain the type for string quark");
	fail_unless(!hg_quark_is_simple_object(q), "Unexpected result to check if a string quark is a complex object");
	q = hg_quark_new(HG_TYPE_NAME, 0);
	fail_unless(q == 0x300000000, "Unexpected result for name quark: 0x300000000, actual: %lx", q);
} TEND

TDEF (hg_quark_get_type)
{
} TEND

TDEF (hg_quark_get_value)
{
} TEND

TDEF (hg_quark_is_simple_object)
{
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgquark.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (hg_quark_new);
	T (hg_quark_get_type);
	T (hg_quark_get_value);
	T (hg_quark_is_simple_object);

	suite_add_tcase(s, tc);

	return s;
}
