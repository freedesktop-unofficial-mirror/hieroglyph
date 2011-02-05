/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmessage.c
 * Copyright (C) 2006-2011 Akira TAGOH
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

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hgmessages.h"

#include "hgmessages.proto.h"

static hg_message_func_t __hg_message_default_handler = _hg_message_default_handler;
static hg_pointer_t __hg_message_default_handler_data = NULL;
static hg_message_func_t __hg_message_handler[HG_MSG_END];
static hg_pointer_t __hg_message_handler_data[HG_MSG_END];

/*< private >*/
static hg_char_t *
_hg_message_get_prefix(hg_message_type_t     type,
		       hg_message_category_t category)
{
	static const hg_char_t *type_string[HG_MSG_END + 1] = {
		NULL,
		"*** ",
		"E: ",
		"W: ",
		"I: ",
		"D: ",
		NULL
	};
	static const hg_char_t *category_string[HG_MSGCAT_END + 1] = {
		NULL,
		"TRACE",
		"BTMAP",
		"ALLOC",
		"   GC",
		"SNAPS",
		NULL
	};
	static const hg_char_t unknown_type[] = "?: ";
	static const hg_char_t unknown_cat[] = "???";
	static const hg_char_t no_cat[] = "";
	const hg_char_t *ts, *cs;
	hg_char_t *retval = NULL, *catstring = NULL;
	hg_usize_t tlen = 0, clen = 0, len;

	if (type >= HG_MSG_END)
		type = HG_MSG_END;
	if (category >= HG_MSGCAT_END)
		category = HG_MSGCAT_END;
	if (type_string[type]) {
		ts = type_string[type];
	} else {
		ts = unknown_type;
	}
	tlen = strlen(ts);
	if (category_string[category]) {
		cs = category_string[category];
	} else if (category == 0) {
		cs = no_cat;
	} else {
		cs = unknown_cat;
	}
	clen = strlen(cs);
	if (clen > 0) {
		catstring = malloc(sizeof (hg_char_t) * (clen + 6));
		snprintf(catstring, clen + 6, "[%s]: ", cs);
		clen = strlen(catstring);
	}
	len = tlen + clen + 1;
	retval = malloc(sizeof (hg_char_t) * len);
	if (retval) {
		snprintf(retval, len, "%s%s ", ts, catstring ? catstring : "");
	}
	if (catstring)
		free(catstring);

	return retval;
}

static void
_hg_message_stacktrace(void)
{
	void *traces[1024];
	char **strings;
	int size, i;

	size = backtrace(traces, 1024);
	if (size > 0) {
		strings = backtrace_symbols(traces, size);
		hg_debug(HG_MSGCAT_TRACE, "Stacktrace:");
		/* 0.. here.
		 * 1.. _hg_message_default_handler
		 * 2.. hg_message_vprintf
		 * 3.. hg_message_printf
		 * 4.. hg_* macros
		 */
		for (i = 4; i < size; i++) {
			hg_debug(HG_MSGCAT_TRACE, "  %d. %s", i - 3, strings[i]);
		}
		free(strings);
	}
}

static void
_hg_message_default_handler(hg_message_type_t      type,
			    hg_message_flags_t     flags,
			    hg_message_category_t  category,
			    const hg_char_t       *message,
			    hg_pointer_t           user_data)
{
	hg_char_t *prefix = NULL;

	if (flags == 0 || (flags & HG_MSG_FLAG_NO_PREFIX) == 0)
		prefix = _hg_message_get_prefix(type, category);
	fprintf(stderr, "%s%s%s", prefix ? prefix : "", message, flags == 0 || (flags & HG_MSG_FLAG_NO_LINEFEED) == 0 ? "\n" : "");
	if (type != HG_MSG_DEBUG && category != HG_MSGCAT_TRACE)
		_hg_message_stacktrace();

	if (prefix)
		free(prefix);
}

/*< public >*/

/**
 * hg_message_set_default_handler:
 * @func:
 * @user_data:
 *
 * FIXME
 *
 * Returns:
 */
hg_message_func_t
hg_message_set_default_handler(hg_message_func_t func,
			       hg_pointer_t      user_data)
{
	hg_message_func_t retval = __hg_message_default_handler;

	__hg_message_default_handler = func;
	__hg_message_default_handler_data = user_data;

	return retval;
}

/**
 * hg_message_set_handler:
 * @type:
 * @func:
 * @user_data:
 *
 * FIXME
 *
 * Returns:
 */
hg_message_func_t
hg_message_set_handler(hg_message_type_t type,
		       hg_message_func_t func,
		       hg_pointer_t      user_data)
{
	hg_message_func_t retval;

	if (type >= HG_MSG_END) {
		fprintf(stderr, "[BUG] invalid message type: %d\n", type);
		return NULL;
	}

	retval = __hg_message_handler[type];
	__hg_message_handler[type] = func;
	__hg_message_handler_data[type] = user_data;

	return retval;
}

/**
 * hg_message_is_masked:
 * @category:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_message_is_enabled(hg_message_category_t category)
{
	static hg_bool_t cache = FALSE;
	static hg_int_t mask = 0;
	const hg_char_t *env;

	if (!cache) {
		env = getenv("HG_DEBUG");
		if (env)
			mask = atoi(env);
		cache = TRUE;
	}

	return ((1 << (category - 1)) & mask) != 0;
}

/**
 * hg_message_printf:
 * @type:
 * @category:
 * @format:
 *
 * FIXME
 */
void
hg_message_printf(hg_message_type_t      type,
		  hg_message_flags_t     flags,
		  hg_message_category_t  category,
		  const hg_char_t       *format,
		  ...)
{
	va_list args;

	va_start(args, format);

	hg_message_vprintf(type, flags, category, format, args);

	va_end(args);
}

/**
 * hg_message_vprintf:
 * @type:
 * @category:
 * @format:
 * @args:
 *
 * FIXME
 */
void
hg_message_vprintf(hg_message_type_t      type,
		   hg_message_flags_t     flags,
		   hg_message_category_t  category,
		   const hg_char_t       *format,
		   va_list                args)
{
	hg_char_t buffer[4096];

	if (type >= HG_MSG_END) {
		fprintf(stderr, "[BUG] Invalid message type: %d\n", type);
		return;
	}
	if (category >= HG_MSGCAT_END) {
		fprintf(stderr, "[BUG] Invalid category type: %d\n", category);
		return;
	}
	if (type == HG_MSG_DEBUG) {
		if (!hg_message_is_enabled(category))
			return;
	}

	vsnprintf(buffer, 4096, format, args);
	if (__hg_message_handler[type]) {
		__hg_message_handler[type](type, flags, category, buffer, __hg_message_handler_data[type]);
	} else if (__hg_message_default_handler) {
		__hg_message_default_handler(type, flags, category, buffer, __hg_message_default_handler_data);
	}
}

/**
 * hg_return_if_fail_warning:
 * @pretty_function:
 * @expression:
 *
 * FIXME
 */
void
hg_return_if_fail_warning(const hg_char_t *pretty_function,
			  const hg_char_t *expression)
{
	hg_message_printf(HG_MSG_CRITICAL,
			  0,
			  HG_MSG_FLAG_NONE,
			  "%s: assertion `%s' failed",
			  pretty_function,
			  expression);
}
