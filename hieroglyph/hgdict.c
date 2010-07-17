/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.c
 * Copyright (C) 2005-2010 Akira TAGOH
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

#include "hgerror.h"
#include "hgmem.h"
#include "hgstring.h"
#include "hgdict.h"

#define HG_DICT_BTREE_NODE_SIZE	5
#define HG_DICT_MAX_SIZE	65535


HG_DEFINE_VTABLE (dict)

/*< private >*/
static gsize
_hg_object_dict_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_dict_t));
}

static gboolean
_hg_object_dict_initialize(hg_object_t *object,
			   va_list      args)
{
	hg_dict_t *dict = (hg_dict_t *)object;

	dict->qdict = hg_btree_new(dict->o.mem, HG_DICT_BTREE_NODE_SIZE, NULL);
	if (dict->qdict == Qnil)
		return FALSE;

	dict->allocated_size = va_arg(args, gsize);
	dict->length = 0;

	return TRUE;
}

static void
_hg_object_dict_free(hg_object_t *object)
{
	hg_dict_t *dict = (hg_dict_t *)object;

	if (dict->qdict != Qnil)
		hg_btree_destroy(dict->o.mem, dict->qdict);
}

static hg_quark_t
_hg_object_dict_copy(hg_object_t *object,
		     gpointer    *ret)
{
	return Qnil;
}

static hg_quark_t
_hg_object_dict_to_string(hg_object_t *object,
			  gpointer    *ret)
{
	return Qnil;
}

/*< public >*/
/**
 * hg_dict_new:
 * @mem:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_dict_new(hg_mem_t *mem,
	    gsize     size,
	    gpointer *ret)
{
	hg_quark_t retval;
	hg_dict_t *dict;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (size < (HG_DICT_MAX_SIZE + 1), Qnil);

	retval = hg_object_new(mem, (gpointer *)&dict, HG_TYPE_DICT, 0, size);
	if (ret)
		*ret = dict;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_dict_add:
 * @dict:
 * @qkey:
 * @qval:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_dict_add(hg_dict_t *dict,
	    hg_quark_t qkey,
	    hg_quark_t qval)
{
	hg_btree_t *tree;
	GError *err = NULL;

	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (!HG_IS_QSTRING (qkey), FALSE);
	hg_return_val_if_lock_fail (tree,
				    dict->o.mem,
				    dict->qdict,
				    FALSE);

	hg_btree_add(tree, qkey, qval, &err);

	hg_mem_unlock_object(dict->o.mem, dict->qdict);

	if (err) {
		g_warning("%s (code: %d)", err->message, err->code);
		g_error_free(err);

		return FALSE;
	}
	dict->length++;

	return TRUE;
}

/**
 * hg_dict_remove:
 * @dict:
 * @qkey:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_dict_remove(hg_dict_t  *dict,
	       hg_quark_t  qkey)
{
	hg_btree_t *tree;
	GError *err = NULL;
	gboolean retval;

	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (!HG_IS_QSTRING (qkey), FALSE);
	hg_return_val_if_lock_fail (tree,
				    dict->o.mem,
				    dict->qdict,
				    FALSE);

	retval = hg_btree_remove(tree, qkey, &err);

	hg_mem_unlock_object(dict->o.mem, dict->qdict);

	if (err) {
		g_warning("%s (code: %d)", err->message, err->code);
		g_error_free(err);

		return FALSE;
	}
	if (retval)
		dict->length--;

	return retval;
}

/**
 * hg_dict_lookup:
 * @dict:
 * @qkey:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_dict_lookup(hg_dict_t  *dict,
	       hg_quark_t  qkey)
{
	hg_quark_t retval;
	hg_btree_t *tree;
	GError *err = NULL;

	hg_return_val_if_fail (dict != NULL, Qnil);
	hg_return_val_if_fail (!HG_IS_QSTRING (qkey), Qnil);
	hg_return_val_if_lock_fail (tree,
				    dict->o.mem,
				    dict->qdict,
				    Qnil);

	retval = hg_btree_find(tree, qkey, &err);

	hg_mem_unlock_object(dict->o.mem, dict->qdict);

	if (err) {
		g_warning("%s (code: %d)", err->message, err->code);
		g_error_free(err);

		return Qnil;
	}

	return retval;
}

/**
 * hg_dict_length:
 * @dict:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_dict_length(hg_dict_t *dict)
{
	hg_return_val_if_fail (dict != NULL, 0);

	return dict->length;
}

/**
 * hg_dict_maxlength:
 * @dict:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_dict_maxlength(hg_dict_t *dict)
{
	hg_return_val_if_fail (dict != NULL, 0);

	return dict->allocated_size;
}

/**
 * hg_dict_foreach:
 * @dict:
 * @func:
 * @data:
 * @error:
 *
 * FIXME
 */
void
hg_dict_foreach(hg_dict_t                 *dict,
		hg_btree_traverse_func_t   func,
		gpointer                   data,
		GError                   **error)
{
	hg_btree_t *tree;

	hg_return_with_gerror_if_fail (dict != NULL, error);
	hg_return_with_gerror_if_fail (func != NULL, error);
	hg_return_with_gerror_if_lock_fail (tree,
					    dict->o.mem,
					    dict->qdict,
					    error);

	hg_btree_foreach(tree, func, data, error);

	hg_mem_unlock_object(dict->o.mem, dict->qdict);
}
