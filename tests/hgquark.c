/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgquark.c
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
#include "hgquark.h"

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
TDEF (bits)
{
	fail_unless(HG_QUARK_TYPE_BIT_TYPE == 0, "not matching a bit type");
	fail_unless(HG_QUARK_TYPE_BIT_TYPE0 == 0, "not matching a bit type0");
	fail_unless(HG_QUARK_TYPE_BIT_TYPE1 == 1, "not matching a bit type1");
	fail_unless(HG_QUARK_TYPE_BIT_TYPE2 == 2, "not matching a bit type2");
	fail_unless(HG_QUARK_TYPE_BIT_TYPE3 == 3, "not matching a bit type3");
	fail_unless(HG_QUARK_TYPE_BIT_TYPE4 == 4, "not matching a bit type4");
	fail_unless(HG_QUARK_TYPE_BIT_TYPE_END == 4, "not matching a bit type end");
	fail_unless(HG_QUARK_TYPE_BIT_ACCESS == 5, "not matching a bit access");
	fail_unless(HG_QUARK_TYPE_BIT_ACCESS0 == 5, "not matching a bit access0");
	fail_unless(HG_QUARK_TYPE_BIT_ACCESS1 == 6, "not matching a bit access1");
	fail_unless(HG_QUARK_TYPE_BIT_ACCESS2 == 7, "not matching a bit access2");
	fail_unless(HG_QUARK_TYPE_BIT_ACCESS3 == 8, "not matching a bit access3");
	fail_unless(HG_QUARK_TYPE_BIT_ACCESS_END == 8, "not matching a bit access end");
	fail_unless(HG_QUARK_TYPE_BIT_EXECUTABLE == 5, "not matching a bit exec");
	fail_unless(HG_QUARK_TYPE_BIT_READABLE == 6, "not matching a bit read");
	fail_unless(HG_QUARK_TYPE_BIT_WRITABLE == 7, "not matching a bit write");
	fail_unless(HG_QUARK_TYPE_BIT_EDITABLE == 8, "not matching a bit editable");
	fail_unless(HG_QUARK_TYPE_BIT_MEM_ID == 9, "not matching a bit mem id");
	fail_unless(HG_QUARK_TYPE_BIT_MEM_ID0 == 9, "not matching a bit mem id0");
	fail_unless(HG_QUARK_TYPE_BIT_MEM_ID1 == 10, "not matching a bit mem id1");
	fail_unless(HG_QUARK_TYPE_BIT_MEM_ID_END == 10, "not matching a bit mem id end");
} TEND

TDEF (hg_quark_new)
{
	hg_quark_t q = hg_quark_new(HG_TYPE_INT, 0);
	fail_unless(q == 0x14100000000, "Unexpected result for integer quark: %lx", q);
	q = hg_quark_new(HG_TYPE_INT, (guint32)-1);
	fail_unless(q == 0x141ffffffff, "Unexpected result for integer quark: expect: %lx, actual: %lx", 0x141ffffffff, q);
	fail_unless(hg_quark_get_type(q) == HG_TYPE_INT, "Unexpected result to obtain the type for integer quark");
	fail_unless(hg_quark_is_simple_object(q), "Unexpected result to check if a integer quark is a simple object");
	q = hg_quark_new(HG_TYPE_STRING, 0xdeadbeaf);
	fail_unless(q == 0x1c5deadbeaf, "Unexpected result for string quark: 0x1c5deadbeaf, actual: %lx", q);
	fail_unless(hg_quark_get_type(q) == HG_TYPE_STRING, "Unexpected result ot obtain the type for string quark");
	fail_unless(!hg_quark_is_simple_object(q), "Unexpected result to check if a string quark is a complex object");
	q = hg_quark_new(HG_TYPE_NAME, 0);
	fail_unless(q == 0x14300000000, "Unexpected result for name quark: 0x14300000000, actual: %lx", q);
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

TDEF (hg_quark_set_executable)
{
} TEND

TDEF (hg_quark_is_executable)
{
	hg_quark_t q;

	q = hg_quark_new(HG_TYPE_INT, 10);
	fail_unless(q == 0x1410000000aLL, "Unexpected result for integer quark");
	hg_quark_set_executable(&q, TRUE);
	fail_unless(q == 0x1610000000aLL, "Unexpected result to set a exec bit");
	fail_unless(hg_quark_is_executable(q), "Unexpected result to check if one is executable");
	hg_quark_set_executable(&q, FALSE);
	fail_unless(q == 0x1410000000aLL, "Unexpected result to unset a exec bit");
	fail_unless(!hg_quark_is_executable(q), "Unexpected result to check if one isn't executable");
	hg_quark_set_readable(&q, TRUE);
	hg_quark_set_writable(&q, TRUE);
	hg_quark_set_executable(&q, TRUE);
	fail_unless(q == 0x1e10000000aLL, "Unexpected result to set all bits");
	fail_unless(hg_quark_is_executable(q), "Unexpected result to check if one is executable [take 2]");
	hg_quark_set_executable(&q, FALSE);
	fail_unless(q == 0x1c10000000aLL, "Unexpected result to unset a exec bit only");
	fail_unless(!hg_quark_is_executable(q), "Unexpected result to check if one isn't executable [take 2]");
} TEND

TDEF (hg_quark_set_readable)
{
} TEND

TDEF (hg_quark_is_readable)
{
	hg_quark_t q;

	q = hg_quark_new(HG_TYPE_INT, 10);
	fail_unless(q == 0x1410000000aLL, "Unexpected result for integer quark");
	hg_quark_set_readable(&q, TRUE);
	fail_unless(q == 0x1410000000aLL, "Unexpected result to set a read bit");
	fail_unless(hg_quark_is_readable(q), "Unexpected result to check if one is readable");
	hg_quark_set_readable(&q, FALSE);
	fail_unless(q == 0x1010000000aLL, "Unexpected result to unset a read bit");
	fail_unless(!hg_quark_is_readable(q), "Unexpected result to check if one isn't readable");
	hg_quark_set_readable(&q, TRUE);
	hg_quark_set_writable(&q, TRUE);
	hg_quark_set_executable(&q, TRUE);
	fail_unless(q == 0x1e10000000aLL, "Unexpected result to set all bits");
	fail_unless(hg_quark_is_readable(q), "Unexpected result to check if one is readable [take 2]");
	hg_quark_set_readable(&q, FALSE);
	fail_unless(q == 0x1a10000000aLL, "Unexpected result to unset a read bit only");
	fail_unless(!hg_quark_is_readable(q), "Unexpected result to check if one isn't readable [take 2]");
} TEND

TDEF (hg_quark_set_writable)
{
} TEND

TDEF (hg_quark_is_writable)
{
	hg_quark_t q;

	q = hg_quark_new(HG_TYPE_INT, 10);
	fail_unless(q == 0x1410000000aLL, "Unexpected result for integer quark");
	hg_quark_set_writable(&q, TRUE);
	fail_unless(q == 0x1c10000000aLL, "Unexpected result to set a write bit");
	fail_unless(hg_quark_is_writable(q), "Unexpected result to check if one is writable");
	hg_quark_set_writable(&q, FALSE);
	fail_unless(q == 0x1410000000aLL, "Unexpected result to unset a write bit");
	fail_unless(!hg_quark_is_writable(q), "Unexpected result to check if one isn't writable");
	hg_quark_set_readable(&q, TRUE);
	hg_quark_set_writable(&q, TRUE);
	hg_quark_set_executable(&q, TRUE);
	fail_unless(q == 0x1e10000000aLL, "Unexpected result to set all bits");
	fail_unless(hg_quark_is_writable(q), "Unexpected result to check if one is writable [take 2]");
	hg_quark_set_writable(&q, FALSE);
	fail_unless(q == 0x1610000000aLL, "Unexpected result to unset a write bit only");
	fail_unless(!hg_quark_is_writable(q), "Unexpected result to check if one isn't writable [take 2]");
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgquark.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (bits);
	T (hg_quark_new);
	T (hg_quark_get_type);
	T (hg_quark_get_value);
	T (hg_quark_is_simple_object);
	T (hg_quark_set_executable);
	T (hg_quark_is_executable);
	T (hg_quark_set_readable);
	T (hg_quark_is_readable);
	T (hg_quark_set_writable);
	T (hg_quark_is_writable);

	suite_add_tcase(s, tc);

	return s;
}
