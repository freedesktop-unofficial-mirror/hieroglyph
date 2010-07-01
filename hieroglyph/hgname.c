/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgname.c
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

#include <string.h>
#include "hgerror.h"
#include "hgquark.h"
#include "hgname.h"


#define HG_NAME_BLOCK_SIZE	512

struct _hg_name_t {
	GHashTable  *name_spool;
	gchar      **quarks;
	hg_quark_t   seq_id;
};

G_INLINE_FUNC hg_quark_t _hg_name_new(hg_name_t   *name,
                                      const gchar *string);


/*< private >*/
G_INLINE_FUNC hg_quark_t
_hg_name_new(hg_name_t   *name,
	     const gchar *string)
{
	hg_quark_t retval;
	gchar *s;

	hg_return_val_if_fail (name->seq_id < (1LL << HG_QUARK_TYPE_BIT_SHIFT), Qnil);
	hg_return_val_if_fail (name->seq_id != 0, Qnil);

	if ((name->seq_id - HG_enc_POSTSCRIPT_RESERVED_END) % HG_NAME_BLOCK_SIZE == 0)
		name->quarks = g_renew(gchar *, name->quarks, name->seq_id - HG_enc_POSTSCRIPT_RESERVED_END + HG_NAME_BLOCK_SIZE);
	s = g_strdup(string);
	retval = name->seq_id++;
	name->quarks[retval - HG_enc_POSTSCRIPT_RESERVED_END] = s;
	g_hash_table_insert(name->name_spool,
			    s, HGQUARK_TO_POINTER (retval));

	return retval;
}

/*< public >*/
/**
 * hg_name_init:
 *
 * FIXME
 *
 * Returns:
 */
hg_name_t *
hg_name_init(void)
{
	hg_name_t *retval = g_new0(hg_name_t, 1);

	hg_return_val_if_fail(retval != NULL, NULL);
	retval->name_spool = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	retval->seq_id = HG_enc_POSTSCRIPT_RESERVED_END + 1;
	retval->quarks = g_new0(gchar *, HG_NAME_BLOCK_SIZE);

	if (!hg_encoding_init()) {
		hg_name_tini(retval);

		return NULL;
	}

	return retval;
}

/**
 * hg_name_tini:
 * @name:
 *
 * FIXME
 */
void
hg_name_tini(hg_name_t *name)
{
	if (name == NULL)
		return;

	g_hash_table_destroy(name->name_spool);
	g_free(name->quarks);
	g_free(name);
	hg_encoding_tini();
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
hg_name_new_with_encoding(hg_name_t            *name,
			  hg_system_encoding_t  encoding)
{
	hg_return_val_if_fail (name != NULL, Qnil);
	hg_return_val_if_fail (encoding < HG_enc_POSTSCRIPT_RESERVED_END, Qnil);

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
hg_name_new_with_string(hg_name_t   *name,
			const gchar *string,
			gssize       len)
{
	hg_system_encoding_t enc;
	hg_quark_t retval;
	gchar *s;

	hg_return_val_if_fail (name != NULL, Qnil);
	hg_return_val_if_fail (string != NULL, Qnil);

	if (len < 0)
		len = strlen(string);

	s = g_strndup(string, len);
	enc = hg_encoding_lookup_system_encoding(s);
	if (enc == HG_enc_END) {
		/* string isn't a system encoding.
		 * try to look up on the name database.
		 */
		retval = HGPOINTER_TO_QUARK (g_hash_table_lookup(name->name_spool, string));
		if (retval == Qnil) {
			/* No name registered for string */
			retval = _hg_name_new(name, s);
		}
	} else {
		retval = enc;
	}
	g_free(s);

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
const gchar *
hg_name_lookup(hg_name_t  *name,
	       hg_quark_t  quark)
{
	const gchar *retval;
	hg_quark_t value;

	hg_return_val_if_fail (name != NULL, NULL);
	hg_return_val_if_fail (quark != Qnil, NULL);
	hg_return_val_if_fail (hg_quark_get_type (quark) == HG_TYPE_NAME, NULL);

	value = hg_quark_get_value(quark);
	if ((hg_system_encoding_t)value > HG_enc_POSTSCRIPT_RESERVED_END) {
		hg_quark_t q = value - HG_enc_POSTSCRIPT_RESERVED_END;

		retval = name->quarks[q];
	} else {
		retval = hg_encoding_get_system_encoding_name(value);
	}

	return retval;
}
