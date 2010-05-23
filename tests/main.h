/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * main.h
 * Copyright (C) 2007-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_TEST_MAIN_H__
#define __HIEROGLYPH_TEST_MAIN_H__

#include <check.h>
#include <glib.h>

G_BEGIN_DECLS

#define HIEROGLYPH_TEST_ERROR	hieroglyph_test_get_error_quark()
#define TDEF(fn)		START_TEST (test_ ## fn)
#define TEND			END_TEST
#define T(fn)			tcase_add_test(tc, test_ ## fn)
#define TNUL(obj)		fail_unless((obj) != NULL, "Failed to create an object")


void    setup                               (void);
void    teardown                            (void);
Suite  *hieroglyph_suite                    (void);
GQuark  hieroglyph_test_get_error_quark     (void);
gchar  *hieroglyph_test_pop_error           (void) G_GNUC_MALLOC;


G_END_DECLS

#endif /* __HIEROGLYPH_TEST_MAIN_H__ */
