/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
 * Copyright (C) 2007 Akira TAGOH
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
#include <hieroglyph/hgarray.h>
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

/* array object */
TDEF (hg_object_array_new)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_array_subarray_new)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_array_put)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_array_get)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_array_get_const)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_array_compare)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_array_dump)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_array_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_object_array_new);
	T (hg_object_array_subarray_new);
	T (hg_object_array_put);
	T (hg_object_array_get);
	T (hg_object_array_get_const);
	T (hg_object_array_compare);
	T (hg_object_array_dump);

	suite_add_tcase(s, tc);

	return s;
}
