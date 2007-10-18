/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.c
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
#include <hieroglyph/hgfile.h>
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

/* file object */
TDEF (hg_object_file_new)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_file_new_from_string)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_file_new_with_custom)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_file_free)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_file_notify_error)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_file_compare)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

TDEF (hg_object_file_dump)
{
	g_print("FIXME: %s\n", __FUNCTION__);
}
TEND

/************************************************************/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_file_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	T (hg_object_file_new);
	T (hg_object_file_new_from_string);
	T (hg_object_file_new_with_custom);
	T (hg_object_file_free);
	T (hg_object_file_notify_error);
	T (hg_object_file_compare);
	T (hg_object_file_dump);

	suite_add_tcase(s, tc);

	return s;
}
