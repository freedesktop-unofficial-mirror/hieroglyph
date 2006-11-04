/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglog.c
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#include <config.h>
#endif

#include "hglog.h"
#include "hgallocator-bfit.h"
#include "hgdict.h"
#include "hgfile.h"
#include "hgmem.h"
#include "hgstring.h"
#include "hgvaluenode.h"


#define HG_LOG_POOL_SIZE 3000000

static void _hg_log_default_handler(HgLogType    log_type,
				    const gchar *domain,
				    const gchar *subtype,
				    const gchar *message,
				    gpointer     data);

static gboolean __hg_log_initialized = FALSE;
static HgDict *__hg_log_options_dict = NULL;
static HgMemPool *__hg_log_mem_pool = NULL;
static HgAllocator *__hg_log_allocator = NULL;
static gchar const *log_type_to_string[] = {
	"Info",
	"Debug",
	"Warning",
	"Error",
};
static HgLogFunc __hg_log_handler = _hg_log_default_handler;
static gpointer __hg_log_handler_data = NULL;

/*
 * Private Functions
 */
static void
_hg_log_default_handler(HgLogType    log_type,
			const gchar *domain,
			const gchar *subtype,
			const gchar *message,
			gpointer     data)
{
	gchar *header;

	header = hg_log_get_log_type_header(log_type, domain);
	if (!hg_file_is_initialized()) {
		g_printerr("%s ***%s%s%s %s\n",
			   header,
			   (subtype ? " " : ""),
			   (subtype ? subtype : ""),
			   (subtype ? ":" : ""),
			   message);
	} else {
		hg_stderr_printf("%s ***%s%s%s %s\n",
				 header,
				 (subtype ? " " : ""),
				 (subtype ? subtype : ""),
				 (subtype ? ":" : ""),
				 message);
	}
	g_free(header);
}

/*
 * Public Functions
 */
/* initializer */
gboolean
hg_log_init(void)
{
	if (!__hg_log_initialized) {
		__hg_log_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
		__hg_log_mem_pool = hg_mem_pool_new(__hg_log_allocator,
						    "Memory pool for HgLog",
						    HG_LOG_POOL_SIZE,
						    HG_MEM_GLOBAL);
		__hg_log_options_dict = hg_dict_new(__hg_log_mem_pool,
						    65535);
		__hg_log_handler = _hg_log_default_handler;

		__hg_log_initialized = TRUE;
	}

	return TRUE;
}

void
hg_log_finalize(void)
{
	if (__hg_log_initialized) {
		__hg_log_initialized = FALSE;
		__hg_log_options_dict = NULL;
		hg_mem_pool_destroy(__hg_log_mem_pool);
		hg_allocator_destroy(__hg_log_allocator);
		__hg_log_mem_pool = NULL;
		__hg_log_allocator = NULL;
	}
}

/* utilities */
void
hg_log_set_default_handler(HgLogFunc func,
			   gpointer  user_data)
{
	g_return_if_fail (func);

	__hg_log_handler = func;
	__hg_log_handler_data = user_data;
}

void
hg_log_set_flag(const gchar *key,
		gboolean     val)
{
	HgValueNode *k, *v;

	g_return_if_fail (__hg_log_initialized);
	g_return_if_fail (key != NULL);

	HG_VALUE_MAKE_NAME_STATIC (__hg_log_mem_pool, k, key);
	HG_VALUE_MAKE_BOOLEAN (__hg_log_mem_pool, v, val);
	hg_dict_insert(__hg_log_mem_pool, __hg_log_options_dict, k, v);
}

gchar *
hg_log_get_log_type_header(HgLogType    log_type,
			   const gchar *domain)
{
	gchar *retval;

	if (domain)
		retval = g_strdup_printf("%s-%s", domain, log_type_to_string[log_type]);
	else
		retval = g_strdup(log_type_to_string[log_type]);

	return retval;
}

void
hg_log(HgLogType    log_type,
       const gchar *domain,
       const gchar *subtype,
       const gchar *format,
       ...)
{
	va_list ap;

	va_start(ap, format);
	hg_logv(log_type, domain, subtype, format, ap);
	va_end(ap);
}

void
hg_logv(HgLogType    log_type,
	const gchar *domain,
	const gchar *subtype,
	const gchar *format,
	va_list      va_args)
{
	HgValueNode *node;
	HgString *string = NULL;

	g_return_if_fail (format != NULL);

	if (subtype == NULL) {
	  default_logger:;
		gchar *buffer = g_strdup_vprintf(format, va_args);

		/* just invoke a default handler */
		__hg_log_handler(log_type, domain, subtype, buffer, __hg_log_handler_data);
		g_free(buffer);
		return;
	}

	if (!__hg_log_initialized) {
#ifdef DEBUG
		goto default_logger;
#endif /* DEBUG */
	} else if ((node = hg_dict_lookup_with_string(__hg_log_options_dict, subtype)) != NULL) {
		string = hg_string_new(__hg_log_mem_pool, -1);
		hg_string_append_vprintf(string, format, va_args);

		if (HG_IS_VALUE_BOOLEAN (node)) {
			if (HG_VALUE_GET_BOOLEAN (node)) {
				/* just invoke a default handler */
				__hg_log_handler(log_type, domain, subtype, hg_string_get_string(string), __hg_log_handler_data);
			}
		} else if (HG_IS_VALUE_OPERATOR (node) ||
			   (HG_IS_VALUE_ARRAY (node) && hg_object_is_executable((HgObject *)node))) {
			hg_log_warning("FIXME: not yet supported.");
		} else {
			hg_log_warning("Invalid object specified for logger.");
		}

		if (string)
			hg_mem_free(string);
	}
}
