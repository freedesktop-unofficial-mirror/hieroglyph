/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.c
 * Copyright (C) 2011 Akira TAGOH
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
#include "hgmem.h"

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
TDEF (hg_mem_spool_get)
{
	hg_mem_t *m;

	hg_errno = 0;
	fail_unless(hg_mem_spool_get(0) == NULL, "Unexpected behavior on obtaining the memory spool");
	fail_unless(hg_errno != 0, "Not set hg_errno");

	m = hg_mem_spool_new(HG_MEM_TYPE_GLOBAL, 10);
	fail_unless(hg_mem_spool_get(0) != NULL, "Unable to obtain the memory spool");
	fail_unless(HG_ERROR_IS_SUCCESS0 (), "Not clear hg_errno");

	hg_mem_spool_destroy(m);
} TEND

TDEF (hg_mem_spool_new)
{
	/* should be done in hg_mem_spool_new_with_allocator */
} TEND

TDEF (hg_mem_spool_new_with_allocator)
{
} TEND

TDEF (hg_mem_spool_destroy)
{
} TEND

TDEF (hg_mem_spool_expand_heap)
{
} TEND

TDEF (hg_mem_spool_set_resizable)
{
} TEND

TDEF (hg_mem_spool_add_gc_marker)
{
} TEND

TDEF (hg_mem_spool_remove_gc_marker)
{
} TEND

TDEF (hg_mem_spool_add_gc_finalizer)
{
} TEND

TDEF (hg_mem_spool_remove_gc_finalizer)
{
} TEND

TDEF (hg_mem_spool_get_type)
{
} TEND

TDEF (hg_mem_spool_enable_gc)
{
} TEND

TDEF (hg_mem_spool_set_gc_procedure)
{
} TEND

TDEF (hg_mem_spool_run_gc)
{
} TEND

TDEF (hg_mem_spool_get_total_size)
{
} TEND

TDEF (hg_mem_spool_get_used_size)
{
} TEND

TDEF (hg_mem_alloc)
{
} TEND

TDEF (hg_mem_alloc_with_flags)
{
} TEND

TDEF (hg_mem_realloc)
{
} TEND

TDEF (hg_mem_free)
{
} TEND

TDEF (hg_mem_set_gc_marker)
{
} TEND

TDEF (hg_mem_set_finalizer)
{
} TEND

TDEF (hg_mem_lock_object)
{
} TEND

TDEF (hg_mem_unlock_object)
{
} TEND

TDEF (hg_mem_gc_mark)
{
} TEND

TDEF (hg_mem_get_id)
{
} TEND

TDEF (hg_mem_ref)
{
} TEND

TDEF (hg_mem_unref)
{
} TEND

TDEF (hg_mem_foreach)
{
} TEND

TDEF (hg_mem_save_snapshot)
{
} TEND

TDEF (hg_mem_restore_snapshot)
{
} TEND

TDEF (hg_mem_restore_mark)
{
} TEND

TDEF (hg_mem_snapshot_free)
{
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_mem_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (hg_mem_spool_get);
	T (hg_mem_spool_new);
	T (hg_mem_spool_new_with_allocator);
	T (hg_mem_spool_destroy);
	T (hg_mem_spool_expand_heap);
	T (hg_mem_spool_set_resizable);
	T (hg_mem_spool_add_gc_marker);
	T (hg_mem_spool_remove_gc_marker);
	T (hg_mem_spool_add_gc_finalizer);
	T (hg_mem_spool_remove_gc_finalizer);
	T (hg_mem_spool_get_type);
	T (hg_mem_spool_enable_gc);
	T (hg_mem_spool_set_gc_procedure);
	T (hg_mem_spool_run_gc);
	T (hg_mem_spool_get_total_size);
	T (hg_mem_spool_get_used_size);
	T (hg_mem_alloc);
	T (hg_mem_alloc_with_flags);
	T (hg_mem_realloc);
	T (hg_mem_free);
	T (hg_mem_set_gc_marker);
	T (hg_mem_set_finalizer);
	T (hg_mem_lock_object);
	T (hg_mem_unlock_object);
	T (hg_mem_gc_mark);
	T (hg_mem_get_id);
	T (hg_mem_ref);
	T (hg_mem_unref);
	T (hg_mem_foreach);
	T (hg_mem_save_snapshot);
	T (hg_mem_restore_snapshot);
	T (hg_mem_restore_mark);
	T (hg_mem_snapshot_free);

	suite_add_tcase(s, tc);

	return s;
}
