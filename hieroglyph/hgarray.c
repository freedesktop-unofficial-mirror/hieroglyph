/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
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

#include <string.h>
#include <glib/gstrfuncs.h>
#include <glib/gthread.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/vm.h>
#include "hgarray.h"


G_LOCK_DEFINE_STATIC (hgarray);

/*
 * private functions
 */

/*
 * public functions
 */
hg_object_t *
hg_object_array_new(hg_vm_t *vm,
		    guint16  length)
{
	hg_object_t *retval;
	hg_arraydata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_arraydata_t) + sizeof (hg_object_t) * length));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_ARRAY;
		HG_OBJECT_ARRAY (retval)->length = length;
		HG_OBJECT_ARRAY (retval)->real_length = length;
		data = HG_OBJECT_ARRAY_DATA (retval);
		data->name[0] = 0;
		if (length > 0) {
			data->array = (hg_object_t *)data->data;
			memset(data->array, 0, sizeof (hg_object_t) * length);
		} else {
			data->array = NULL;
		}
	}

	return retval;
}

hg_object_t *
hg_object_array_subarray_new(hg_vm_t     *vm,
			     hg_object_t *object,
			     guint16      start_index,
			     guint16      length)
{
	hg_object_t *retval, **p;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object), NULL);
	hg_return_val_if_fail (HG_OBJECT_ARRAY (object)->real_length > start_index, NULL);
	hg_return_val_if_fail (HG_OBJECT_ARRAY (object)->real_length >= (start_index + length), NULL);

	retval = hg_object_array_new(vm, 0);
	if (retval != NULL) {
		p = HG_OBJECT_ARRAY_DATA (object)->array;
		HG_OBJECT_ARRAY_DATA (retval)->array = &p[start_index];
		HG_OBJECT_ARRAY (retval)->real_length = length;
	}

	return retval;
}

gboolean
hg_object_array_put(hg_vm_t     *vm,
		    hg_object_t *object,
		    hg_object_t *data,
		    guint16      index)
{
	hg_object_t **array;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (object != NULL, FALSE);
	hg_return_val_if_fail (data != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object), FALSE);
	hg_return_val_if_fail (HG_OBJECT_ARRAY (object)->real_length > index, FALSE);
	/* XXX: need to check the memory spool type */

	array = HG_OBJECT_ARRAY_DATA (object)->array;
	array[index] = data;

	return TRUE;
}

hg_object_t *
hg_object_array_get(hg_vm_t           *vm,
		    const hg_object_t *object,
		    guint16            index)
{
	hg_object_t **array, *retval, *p;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object), NULL);
	hg_return_val_if_fail (HG_OBJECT_ARRAY (object)->real_length > index, NULL);

	array = HG_OBJECT_ARRAY_DATA (object)->array;
	p = array[index];

	if (!HG_OBJECT_IS_COMPLEX (p)) {
		/* create a copy to prevent destructive modifications */
		retval = hg_object_copy(vm, p);
	} else {
		/* complex objects are allowed to be modified */
		retval = p;
	}

	return retval;
}

hg_object_t *
hg_object_array_get_const(const hg_object_t *object,
			  guint16            index)
{
	hg_object_t **array;

	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object), NULL);
	hg_return_val_if_fail (HG_OBJECT_ARRAY (object)->real_length > index, NULL);

	array = HG_OBJECT_ARRAY_DATA (object)->array;
	return array[index];
}

gboolean
hg_object_array_compare(hg_object_t *object1,
			hg_object_t *object2)
{
	guint16 i;
	hg_object_t *a, *b;
	gboolean retval = TRUE;

	hg_return_val_if_fail (object1 != NULL, FALSE);
	hg_return_val_if_fail (object2 != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object1), FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object2), FALSE);

	if (object1 == object2)
		return TRUE;
	if (HG_OBJECT_ARRAY (object1)->real_length != HG_OBJECT_ARRAY (object2)->real_length)
		return FALSE;

	G_LOCK (hgarray);
	if (HG_OBJECT_OBJECT (object1)->attr.bit.is_checked) {
		retval = (HG_OBJECT_OBJECT (object1)->attr.bit.is_checked == HG_OBJECT_OBJECT (object2)->attr.bit.is_checked);
		G_UNLOCK (hgarray);

		return retval;
	}
	/* for checking the circular reference */
	HG_OBJECT_OBJECT (object1)->attr.bit.is_checked = TRUE;
	HG_OBJECT_OBJECT (object2)->attr.bit.is_checked = TRUE;
	G_UNLOCK (hgarray);

	for (i = 0; i < HG_OBJECT_ARRAY (object1)->real_length; i++) {
		a = hg_object_array_get_const(object1, i);
		b = hg_object_array_get_const(object2, i);
		if (!hg_object_compare(a, b)) {
			retval = FALSE;
			break;
		}
	}

	/* cleaning up */
	G_LOCK (hgarray);
	HG_OBJECT_OBJECT (object1)->attr.bit.is_checked = FALSE;
	HG_OBJECT_OBJECT (object2)->attr.bit.is_checked = FALSE;
	G_UNLOCK (hgarray);

	return retval;
}

gchar *
hg_object_array_dump(hg_object_t *object,
		     gboolean     verbose)
{
	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_ARRAY (object), NULL);

	return g_strdup("--array--");
}
