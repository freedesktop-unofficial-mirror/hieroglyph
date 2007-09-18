/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.c
 * Copyright (C) 2005-2007 Akira TAGOH
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

#include <glib/gstrfuncs.h>
#include <hieroglyph/hgobject.h>
#include "hgdict.h"
#include "hgdictprime.h"

#define _get_hash(_hg_d_,_hg_o_)	((hg_object_get_hash(_hg_o_) << 8) % __hg_dict_primes[HG_OBJECT_DICT (_hg_d_)->length])


/*
 * private functions
 */
static gboolean
_hg_object_dict_lookup(hg_object_t *dict,
		       hg_object_t *key,
		       guint16     *retval)
{
	guint16 h, hash;
	hg_dictdata_t *data = HG_OBJECT_DICT_DATA (dict);

	hg_return_val_if_fail (retval != NULL, FALSE);

	h = hash = _get_hash(dict, key);
	while (1) {
		if (data[hash].key != NULL && data[hash].value != NULL) {
			if (hg_object_compare(key, data[hash].key)) {
				*retval = hash;

				return TRUE;
			}
		} else {
			/* probably no such keys are registered into dict */
			break;
		}
		/* try to find out next slot */
		if ((hash + 1) == h) {
			/* probably no such keys are registered int odict */
			break;
		}
		hash++;
		if (hash > __hg_dict_primes[HG_OBJECT_DICT (dict)->length]) {
			/* looking at the first slot */
			hash = 0;
		}
	}
	*retval = 0;

	return FALSE;
}

static gboolean
_hg_object_dict_insert(hg_object_t *dict,
		       hg_object_t *key,
		       hg_object_t *value,
		       gboolean     with_replace)
{
	gboolean retval = FALSE;
	guint16 h, hash;
	hg_dictdata_t *data = HG_OBJECT_DICT_DATA (dict);

	hg_return_val_if_fail (HG_OBJECT_DICT (dict)->used < HG_OBJECT_DICT (dict)->length, FALSE);

	h = hash = _get_hash(dict, key);
	while (1) {
		if (data[hash].key == NULL && data[hash].value == NULL) {
			data[hash].key = key;
			data[hash].value = value;
			HG_OBJECT_DICT (dict)->used++;
			retval = TRUE;
			break;
		} else if (hg_object_compare(key, data[hash].key)) {
			if (with_replace) {
				data[hash].key = key;
				data[hash].value = value;
				retval = TRUE;
			}
			break;
		} else {
			/* try to find out an empty slot */
			if ((hash + 1) == h) {
				/* probably no empty slots.
				 * but guess this won't happen because of checking
				 */
				g_warning("[BUG] no empty slots in the dict.");

				return FALSE;
			}
			hash++;
			if (hash > __hg_dict_primes[HG_OBJECT_DICT (dict)->length]) {
				/* try to find out from first */
				hash = 0;
			}
		}
	}

	return FALSE;
}

/*
 * public functions
 */
hg_object_t *
hg_object_dict_new(hg_vm_t *vm,
		   guint16  n_nodes)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_dictdata_t) * __hg_dict_primes[n_nodes]));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_DICT;
		HG_OBJECT_DICT (retval)->length = n_nodes;
		HG_OBJECT_DICT (retval)->used = 0;
	}

	return retval;
}

gboolean
hg_object_dict_compare(hg_object_t *object1,
		       hg_object_t *object2)
{
	hg_return_val_if_fail (object1 != NULL, FALSE);
	hg_return_val_if_fail (object2 != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (object1), FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (object2), FALSE);

	/* XXX: no copy and dup functionalities are available so far.
	 * so just comparing a pointer should works enough.
	 */
	return object1 == object2;
}

gchar *
hg_object_dict_dump(hg_object_t *object,
		    gboolean     verbose)
{
	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (object), NULL);

	return g_strdup("--dict--");
}

gboolean
hg_object_dict_insert(hg_object_t *dict,
		      hg_object_t *key,
		      hg_object_t *value)
{
	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (key != NULL, FALSE);
	hg_return_val_if_fail (value != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (dict), FALSE);

	if (HG_OBJECT_ATTR_IS_GLOBAL (dict) &&
	    !HG_OBJECT_ATTR_IS_GLOBAL (value)) {
		return FALSE;
	}

	return _hg_object_dict_insert(dict, key, value, FALSE);
}

gboolean
hg_object_dict_insert_without_consistency(hg_object_t *dict,
					  hg_object_t *key,
					  hg_object_t *value)
{
	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (key != NULL, FALSE);
	hg_return_val_if_fail (value != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (dict), FALSE);

	return _hg_object_dict_insert(dict, key, value, FALSE);
}

gboolean
hg_object_dict_replace(hg_object_t *dict,
		       hg_object_t *key,
		       hg_object_t *value)
{
	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (key != NULL, FALSE);
	hg_return_val_if_fail (value != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (dict), FALSE);

	if (HG_OBJECT_ATTR_IS_GLOBAL (dict) &&
	    !HG_OBJECT_ATTR_IS_GLOBAL (value)) {
		return FALSE;
	}

	return _hg_object_dict_insert(dict, key, value, TRUE);
}

gboolean
hg_object_dict_replace_without_consistency(hg_object_t *dict,
					   hg_object_t *key,
					   hg_object_t *value)
{
	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (key != NULL, FALSE);
	hg_return_val_if_fail (value != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (dict), FALSE);

	return _hg_object_dict_insert(dict, key, value, TRUE);
}

gboolean
hg_object_dict_remove(hg_object_t *dict,
		      hg_object_t *key)
{
	guint16 hash;

	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (key != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (dict), FALSE);

	if (_hg_object_dict_lookup(dict, key, &hash)) {
		hg_dictdata_t *data = HG_OBJECT_DICT_DATA (dict);

		data[hash].key = NULL;
		data[hash].value = NULL;

		return TRUE;
	}

	return FALSE;
}

hg_object_t *
hg_object_dict_lookup(hg_object_t *dict,
		      hg_object_t *key)
{
	guint16 hash;

	hg_return_val_if_fail (dict != NULL, NULL);
	hg_return_val_if_fail (key != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_DICT (dict), NULL);

	if (_hg_object_dict_lookup(dict, key, &hash)) {
		hg_dictdata_t *data = HG_OBJECT_DICT_DATA (dict);

		return data[hash].value;
	}

	return NULL;
}
