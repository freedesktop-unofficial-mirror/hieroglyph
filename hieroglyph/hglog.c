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
#include "hgdict.h"
#include "hgfile.h"
#include "hgmem.h"
#include "hgstring.h"
#include "hgvaluenode.h"
#include "vm.h"


static void _hg_log_default_handler(HgLogType    log_type,
				    const gchar *subtype,
				    const gchar *message);

static gboolean __hg_log_initialized = FALSE;
static HgDict *__hg_log_options_dict = NULL;
static HgVM *__hg_log_vm = NULL;
static gchar const *log_type_to_string[] = {
	"Info",
	"Debug",
	"Warning",
	"Error",
};
static HgLogFunc __hg_log_handler = _hg_log_default_handler;

/*
 * Private Functions
 */
static void
_hg_log_default_handler(HgLogType    log_type,
			const gchar *subtype,
			const gchar *message)
{
	HgFileObject *file;

	if (__hg_log_vm) {
		file = hg_vm_get_io(__hg_log_vm, VM_IO_STDERR);
	} else {
		if (!hg_file_is_initialized())
			hg_file_init();
		file = __hg_file_stderr;
	}
	hg_file_object_printf(file, "%s ***%s%s%s %s\n",
			      log_type_to_string[log_type],
			      (subtype ? " " : ""),
			      (subtype ? subtype : ""),
			      (subtype ? ":" : ""),
			      message);
}

/*
 * Public Functions
 */
/* initializer */
gboolean
hg_log_init(HgVM   *vm,
	    HgDict *dict)
{
	HgMemObject *obj;

	g_return_val_if_fail (vm != NULL, FALSE);
	g_return_val_if_fail (dict != NULL, FALSE);

	if (__hg_log_options_dict == NULL) {
		hg_mem_get_object__inline(dict, obj);
		if (obj == NULL) {
			hg_log_warning("Invalid object %p to be set as Log Dict.", dict);
			return FALSE;
		}
		hg_mem_set_lock(obj);
		__hg_log_options_dict = dict;

		hg_mem_get_object__inline(vm, obj);
		if (obj == NULL) {
			hg_log_warning("Invalid object %p to be set as Log VM.", vm);
			return FALSE;
		}
		hg_mem_set_lock(obj);
		__hg_log_vm = vm;
		__hg_log_handler = _hg_log_default_handler;

		__hg_log_initialized = TRUE;
	}

	return TRUE;
}

void
hg_log_finalize(void)
{
	HgMemObject *obj;

	if (__hg_log_initialized) {
		hg_mem_get_object__inline(__hg_log_options_dict, obj);
		if (obj != NULL) {
			hg_mem_set_unlock(obj);
		}
		__hg_log_options_dict = NULL;
		hg_mem_get_object__inline(__hg_log_vm, obj);
		if (obj != NULL) {
			hg_mem_set_unlock(obj);
		}
		__hg_log_vm = NULL;
		__hg_log_initialized = FALSE;
	}
}

/* utilities */
void
hg_log_set_default_handler(HgLogFunc func)
{
	g_return_if_fail (func);

	__hg_log_handler = func;
}

void
hg_log(HgLogType    log_type,
       const gchar *subtype,
       const gchar *format,
       ...)
{
	va_list ap;

	va_start(ap, format);
	hg_logv(log_type, subtype, format, ap);
	va_end(ap);
}

void
hg_logv(HgLogType    log_type,
	const gchar *subtype,
	const gchar *format,
	va_list      va_args)
{
	HgValueNode *node;
	HgString *string = NULL;

	g_return_if_fail (format != NULL);

	if (subtype == NULL) {
		gchar *buffer = g_strdup_vprintf(format, va_args);

		/* just invoke a default handler */
		__hg_log_handler(log_type, subtype, buffer);
		g_free(buffer);
		return;
	}

	g_return_if_fail (__hg_log_initialized);

	if ((node = hg_dict_lookup_with_string(__hg_log_options_dict, subtype)) != NULL) {
		HgMemPool *pool = hg_vm_get_current_pool(__hg_log_vm);

		string = hg_string_new(pool, -1);
		hg_string_append_vprintf(string, format, va_args);

		if (HG_IS_VALUE_BOOLEAN (node)) {
			if (HG_VALUE_GET_BOOLEAN (node)) {
				/* just invoke a default handler */
				__hg_log_handler(log_type, subtype, hg_string_get_string(string));
			}
		} else if (HG_IS_VALUE_OPERATOR (node) ||
			   (HG_IS_VALUE_ARRAY (node) && hg_object_is_executable((HgObject *)node))) {
			HgValueNode *strnode = NULL, *subnode, *lognode;
			HG_VALUE_MAKE_STRING (strnode, string);
			lognode = hg_vm_get_name_node(__hg_log_vm, log_type_to_string[log_type]);
			subnode = hg_vm_get_name_node(__hg_log_vm, subtype);
			hg_log_warning("FIXME: not yet supported.");

			if (strnode)
				hg_mem_free(strnode);
		} else {
			hg_log_warning("Invalid object specified for logger.");
		}

		if (string)
			hg_mem_free(string);
	}
}
