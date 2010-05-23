/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * main.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <check.h>
#include "main.h"

extern Suite *hieroglyph_suite(void);

static GLogFunc old_logger = NULL;
static GError *error = NULL;

/*
 * Private functions
 */
static void
logger(const gchar    *log_domain,
       GLogLevelFlags  log_level,
       const gchar    *message,
       gpointer        user_data)
{
	GError *err = NULL;
	gchar *prev = NULL;

	if (error) {
		prev = g_strdup_printf("\n    %s", error->message);
		g_clear_error(&error);
	}
	g_set_error(&err, HIEROGLYPH_TEST_ERROR, log_level,
		    "%s%s", message,
		    (prev ? prev : ""));
	error = err;
	g_free(prev);
}

static void
init(int argc, char **argv)
{
	old_logger = g_log_set_default_handler(logger, NULL);
}

static void
fini(void)
{
	if (old_logger)
		g_log_set_default_handler(old_logger, NULL);
}

/*
 * Public functions
 */
GQuark
hieroglyph_test_get_error_quark(void)
{
	GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string("hieroglyph-test-error");

	return quark;
}

gchar *
hieroglyph_test_pop_error(void)
{
	gchar *retval = NULL;

	if (error) {
		retval = g_strdup(error->message);
		g_clear_error(&error);
	}

	return retval;
}

int
main(int argc, char **argv)
{
	int number_failed;
	Suite *s = hieroglyph_suite();
	SRunner *sr = srunner_create(s);

	init(argc, argv);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	fini();

	return (number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
