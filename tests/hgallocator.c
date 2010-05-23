/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator.c
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

#include "hgallocator.h"
#include "hgallocator-private.h"
#include "main.h"


hg_mem_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	vtable = hg_allocator_get_vtable();
}

void
teardown(void)
{
}

/** test cases **/
TDEF (initialize)
{
	hg_allocator_data_t *retval;
	hg_allocator_private_t *priv;

	retval = vtable->initialize();
	fail_unless(retval != NULL, "Unable to initialize the allocator.");
	fail_unless(retval->total_size == 0, "Unable to initialize total memory size.");
	fail_unless(retval->used_size == 0, "Unable to initialize used memory size.");

	priv = (hg_allocator_private_t *)retval;

	fail_unless(priv->bitmap == NULL, "Detected the garbage entry on the bitmap table.");
	fail_unless(priv->heap == NULL, "Detected the garbage entry on the heap.");

	vtable->finalize(retval);
} TEND

TDEF (finalize)
{
	/* can be done in initialize testcase. */
} TEND

TDEF (resize_heap)
{
	hg_allocator_data_t *retval;
	hg_allocator_private_t *priv;

	retval = vtable->initialize();
	fail_unless(retval != NULL, "Unable to initialize the allocator.");
	fail_unless(vtable->resize_heap(retval, 256), "Unable to initialize the heap.");
	priv = (hg_allocator_private_t *)retval;
	fail_unless(retval->total_size >= 256, "Unable to assign the heap more than requested size.");
	fail_unless(priv->bitmap != NULL, "Unable tto initialize the bitmap table.");
	fail_unless(priv->heap != NULL, "Unable to initialize the heap.");

	fail_unless(vtable->resize_heap(retval, 512), "Unable to resize the heap.");
	fail_unless(retval->total_size >= 512, "Unable to reassign the heap more than requested size.");
	fail_unless(priv->bitmap != NULL, "Unable tto resize the bitmap table.");
	fail_unless(priv->heap != NULL, "Unable to resize the heap.");

	vtable->finalize(retval);
} TEND

TDEF (alloc)
{
} TEND

TDEF (free)
{
} TEND

TDEF (lock_object)
{
} TEND

TDEF (unlock_object)
{
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_allocator_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (initialize);
	T (finalize);
	T (resize_heap);
	T (alloc);
	T (free);
	T (lock_object);
	T (unlock_object);

	suite_add_tcase(s, tc);

	return s;
}
