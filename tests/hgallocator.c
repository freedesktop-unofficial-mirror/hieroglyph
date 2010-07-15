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
#include "hgquark.h"
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
	gchar *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
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
	hg_allocator_data_t *retval;
	hg_quark_t t, t2, t3;

	retval = vtable->initialize();
	fail_unless(retval != NULL, "Unable to initialize the allocator.");
	fail_unless(vtable->resize_heap(retval, 256), "Unable to initialize the heap.");

	t = vtable->alloc(retval, 128, NULL);
	fail_unless(t != Qnil, "Unable to allocate the memory.");
	t2 = vtable->alloc(retval, 64, NULL);
	fail_unless(t2 != Qnil, "Unable to allocate the memory.");
	t3 = vtable->alloc(retval, 128, NULL);
	fail_unless(t3 == Qnil, "Should be no free spaces available.");

	vtable->free(retval, t);
	t3 = vtable->alloc(retval, 128, NULL);
	fail_unless(t3 != Qnil, "Unable to allocate the memory.");

	vtable->finalize(retval);
} TEND

TDEF (realloc)
{
	hg_allocator_data_t *retval;
	hg_quark_t t, t2, t3;
	gpointer p, p2;

	retval = vtable->initialize();
	fail_unless(retval != NULL, "Unable to initialize the allocator.");
	fail_unless(vtable->resize_heap(retval, 256), "Unable to initialize the heap.");

	t = vtable->alloc(retval, 32, NULL);
	fail_unless(t != Qnil, "Unable to allocate the memory.");
	p = vtable->lock_object(retval, t);
	vtable->unlock_object(retval, t);
	t2 = vtable->realloc(retval, t, 64, NULL);
	fail_unless(t2 != Qnil, "Unable to re-allocate the memory.");
	fail_unless(t == t2, "Unexpected result to keep quark.");
	p2 = vtable->lock_object(retval, t2);
	vtable->unlock_object(retval, t2);
	fail_unless(p == p2, "Unexpected result to grow the space.");
	t3 = vtable->alloc(retval, 8, NULL);
	fail_unless(t3 != Qnil, "Unable to allocate the memory.");
	t2 = vtable->realloc(retval, t, 80, NULL);
	fail_unless(t2 != Qnil, "Unable to re-allocate the memory.");
	fail_unless(t == t2, "Unexpected result to keep quark.");
	p2 = vtable->lock_object(retval, t2);
	vtable->unlock_object(retval, t2);
	fail_unless(p != p2, "Unexpected result to re-allocate: original: %x, reallocated: %x", p, p2);
	t2 = vtable->realloc(retval, t, 512, NULL);
	fail_unless(t2 == Qnil, "Should be no free spaces available.");
	vtable->free(retval, t);
	t = vtable->alloc(retval, 8, NULL);
	p = vtable->lock_object(retval, t);
	t2 = vtable->realloc(retval, t, 16, NULL);
	fail_unless(t2 == Qnil, "Unexpected result to re-allocate the locked memory.");
	g_free(hieroglyph_test_pop_error());
	t = vtable->alloc(retval, 32, &p);
	t2 = vtable->realloc(retval, t, 64, &p2);
	fail_unless(t2 == Qnil, "Unexpected result to re-allocate the indirect locked memory.");
	g_free(hieroglyph_test_pop_error());
	vtable->unlock_object(retval, t);
	t2 = vtable->realloc(retval, t, 64, &p2);
	fail_unless(t2 == t, "Unable to re-allocate the memory after unlocking the indirect locked memory.");

	vtable->finalize(retval);
} TEND

TDEF (free)
{
	/* can be done in alloc testcase */
} TEND

TDEF (lock_object)
{
	hg_allocator_data_t *retval;
	hg_quark_t t;
	gpointer p, p2;
	hg_allocator_block_t *b;

	retval = vtable->initialize();
	fail_unless(retval != NULL, "Unable to initialize the allocator.");
	fail_unless(vtable->resize_heap(retval, 256), "Unable to initialize the heap.");

	t = vtable->alloc(retval, 128, &p2);
	fail_unless(t != Qnil, "Unable to allocate the memory.");
	b = hg_get_allocator_block (p2);
	fail_unless(b->lock_count == 1, "Detected inconsistency in the lock count");

	p = vtable->lock_object(retval, t);
	b = hg_get_allocator_block (p);
	fail_unless(p != NULL, "Unable to obtain the object address.");
	fail_unless(p == p2, "Obtaining the different object address.");
	fail_unless(b->lock_count == 2, "Failed to lock the object.");

	p = vtable->lock_object(retval, t);
	b = hg_get_allocator_block (p);
	fail_unless(p != NULL, "Unable to obtain the object address.");
	fail_unless(b->lock_count == 3, "Failed to lock the object.");

	vtable->unlock_object(retval, t);
	fail_unless(b->lock_count == 2, "Failed to unlock the object.");

	vtable->unlock_object(retval, t);
	fail_unless(b->lock_count == 1, "Failed to unlock the object.");

	vtable->finalize(retval);
} TEND

TDEF (unlock_object)
{
	/* can be done in lock_object testcase */
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
	T (realloc);
	T (lock_object);
	T (unlock_object);

	suite_add_tcase(s, tc);

	return s;
}
