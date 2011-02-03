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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hgmessage.h"

#include "hgmessage.proto.h"

static hg_message_func_t __hg_message_default_handler = _hg_message_default_handler;
static hg_pointer_t __hg_message_default_handler_data = NULL;
static hg_message_func_t __hg_message_handler[HG_MSG_END];
static hg_pointer_t __hg_message_handler_data[HG_MSG_END];

/*< private >*/
static hg_char_t *
_hg_message_get_prefix(hg_message_type_t     type,
		       hg_message_category_t category)
{
	const hg_char_t *type_string[HG_MSG_END + 1] = {
		NULL,
		"*** ",
		"E: ",
		"W: ",
		"I: ",
		"D: ",
		NULL
	};
	const hg_char_t *category_string[HG_MSGCAT_END + 1] = {
		NULL,
		NULL
	};
	const hg_char_t unknown_type[] = "?: ";
	const hg_char_t unknown_cat[] = "???";
	const hg_char_t *ts, *cs;
	hg_char_t *retval = NULL;
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
	} else {
		cs = unknown_cat;
	}
	clen = strlen(cs);
	len = tlen + clen + 6;
	retval = malloc(sizeof (hg_char_t) * len);
	if (retval) {
		snprintf(retval, len, "%s[%s]: ", ts, cs);
	}

	return retval;
}

static void
_hg_message_default_handler(hg_message_type_t      type,
			    hg_message_category_t  category,
			    const hg_char_t       *message,
			    hg_pointer_t           user_data)
{
	hg_char_t *prefix = NULL;

	if (type == HG_MSG_DEBUG) {
		const hg_char_t *env = getenv("HG_DEBUG");
		hg_int_t mask = 0;

		if (env)
			mask = atoi(env);

		if (((1 << (category - 1)) & mask) == 0)
			return;
	}
	prefix = _hg_message_get_prefix(type, category);
	fprintf(stderr, "%s%s\n", prefix, message);

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
 * hg_message_printf:
 * @type:
 * @category:
 * @format:
 *
 * FIXME
 */
void
hg_message_printf(hg_message_type_t      type,
		  hg_message_category_t  category,
		  const hg_char_t       *format,
		  ...)
{
	va_list args;

	va_start(args, format);

	hg_message_vprintf(type, category, format, args);

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

	vsnprintf(buffer, 4096, format, args);
	if (__hg_message_handler[type]) {
		__hg_message_handler[type](type, category, buffer, __hg_message_handler_data[type]);
	} else if (__hg_message_default_handler) {
		__hg_message_default_handler(type, category, buffer, __hg_message_default_handler_data);
	}
}
