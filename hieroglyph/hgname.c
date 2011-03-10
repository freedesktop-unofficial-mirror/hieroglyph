/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgname.c
 * Copyright (C) 2010-2011 Akira TAGOH
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

#include <string.h>
/* XXX: GLib is still needed for the hash table */
#include <glib.h>
#include "hgerror.h"
#include "hgquark.h"
#include "hgtypebit-private.h"
#include "hgutils.h"
#include "hgname.h"

#include "hgname.proto.h"

#define HG_NAME_BLOCK_SIZE	512

typedef struct _hg_name_t	hg_name_t;

struct _hg_name_t {
	volatile hg_int_t   ref_count;
	volatile hg_int_t   seq_id;
	GHashTable         *name_spool;
	hg_char_t         **quarks;
};

static hg_name_t __hg_name_pool = {0, 0, NULL, NULL};

/*< private >*/
HG_INLINE_FUNC hg_quark_t
_hg_name_new(const hg_char_t *string)
{
	hg_quark_t retval;
	hg_char_t *s;

	hg_return_val_if_fail (__hg_name_pool.seq_id < (1LL << HG_TYPEBIT_SHIFT), Qnil, HG_e_VMerror);
	hg_return_val_if_fail (__hg_name_pool.seq_id != 0, Qnil, HG_e_VMerror);

	if ((__hg_name_pool.seq_id - HG_enc_POSTSCRIPT_RESERVED_END) % HG_NAME_BLOCK_SIZE == 0)
		__hg_name_pool.quarks = (hg_char_t **)hg_realloc(__hg_name_pool.quarks,
								 sizeof (hg_char_t *) * (__hg_name_pool.seq_id - HG_enc_POSTSCRIPT_RESERVED_END + HG_NAME_BLOCK_SIZE));
	s = hg_strdup(string);
	retval = g_atomic_int_exchange_and_add(&__hg_name_pool.seq_id, 1);
	__hg_name_pool.quarks[retval - HG_enc_POSTSCRIPT_RESERVED_END] = s;
	g_hash_table_insert(__hg_name_pool.name_spool,
			    s, HGQUARK_TO_POINTER (retval));

	return retval;
}

/*< public >*/
/**
 * hg_name_init:
 *
 * FIXME
 */
void
hg_name_init(void)
{
	if (g_atomic_int_exchange_and_add(&__hg_name_pool.ref_count, 1) == 0) {
		__hg_name_pool.name_spool = g_hash_table_new_full(g_str_hash, g_str_equal, hg_free, NULL);
		__hg_name_pool.seq_id = HG_enc_POSTSCRIPT_RESERVED_END + 1;
		__hg_name_pool.quarks = (hg_char_t **)hg_malloc0(sizeof (hg_char_t *) * HG_NAME_BLOCK_SIZE);
	}
	if (!hg_encoding_init()) {
		hg_name_tini();
	}
}

/**
 * hg_name_tini:
 *
 * FIXME
 */
void
hg_name_tini(void)
{
	hg_int_t oldval;

	hg_return_if_fail (__hg_name_pool.ref_count > 0, HG_e_VMerror);

	oldval = g_atomic_int_exchange_and_add(&__hg_name_pool.ref_count, -1);
	if (oldval == 1) {
		g_hash_table_destroy(__hg_name_pool.name_spool);
		__hg_name_pool.name_spool = NULL;
		hg_free(__hg_name_pool.quarks);
		__hg_name_pool.quarks = NULL;
		hg_encoding_tini();
	}
}

/**
 * hg_name_new_with_encoding:
 * @name:
 * @encoding:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_name_new_with_encoding(hg_system_encoding_t  encoding)
{
	hg_return_val_if_fail (__hg_name_pool.ref_count > 0, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (encoding < HG_enc_POSTSCRIPT_RESERVED_END, Qnil, HG_e_VMerror);

	return hg_quark_new(HG_TYPE_NAME, encoding);
}

/**
 * hg_name_new_with_string:
 * @name:
 * @string:
 * @len:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_name_new_with_string(const hg_char_t *string,
			hg_size_t        len)
{
	hg_system_encoding_t enc;
	hg_quark_t retval;
	hg_char_t *s;

	hg_return_val_if_fail (__hg_name_pool.ref_count > 0, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (string != NULL, Qnil, HG_e_VMerror);

	if (len < 0)
		len = strlen(string);

	s = hg_strndup(string, len);
	enc = hg_encoding_lookup_system_encoding(s);
	if (enc == HG_enc_END) {
		hg_pointer_t p = NULL;

		/* string isn't a system encoding.
		 * try to look up on the name database.
		 */
		if (!g_hash_table_lookup_extended(__hg_name_pool.name_spool,
						  string,
						  NULL,
						  &p)) {
			/* No name registered for string */
			retval = _hg_name_new(s);
		} else {
			retval = HGPOINTER_TO_QUARK (p);
		}
	} else {
		retval = enc;
	}
	hg_free(s);

	return hg_quark_new(HG_TYPE_NAME, retval);
}

/**
 * hg_name_lookup:
 * @name:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
const hg_char_t *
hg_name_lookup(hg_quark_t  quark)
{
	const hg_char_t *retval;
	hg_quark_t value;
	hg_type_t type = hg_quark_get_type(quark);

	hg_return_val_if_fail (__hg_name_pool.ref_count > 0, NULL, HG_e_VMerror);
	hg_return_val_if_fail (quark != Qnil, NULL, HG_e_VMerror);
	hg_return_val_if_fail (type == HG_TYPE_NAME || type == HG_TYPE_EVAL_NAME, NULL, HG_e_VMerror);

	value = hg_quark_get_value(quark);
	if ((hg_system_encoding_t)value > HG_enc_POSTSCRIPT_RESERVED_END) {
		hg_quark_t q = value - HG_enc_POSTSCRIPT_RESERVED_END;

		retval = __hg_name_pool.quarks[q];
	} else {
		retval = hg_encoding_get_system_encoding_name(value);
	}

	return retval;
}
