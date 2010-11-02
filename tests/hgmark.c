/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmark.c
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

#include "hgmark.h"
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
TDEF (new)
{
	hg_quark_t q;

	q = HG_QMARK;
	fail_unless(q == 0x14a00000000, "Unexpected result to create a quark for mark: expected %lx, actual:%lx", 0x14a00000000, q);
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgmark.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (new);

	suite_add_tcase(s, tc);

	return s;
}
