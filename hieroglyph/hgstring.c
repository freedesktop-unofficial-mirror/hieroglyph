/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.c
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
#include <hieroglyph/hgobject.h>
#include "hgstring.h"


/*
 * private functions
 */

/*
 * public functions
 */
hg_object_t *
hg_object_string_sized_new(hg_vm_t *vm,
			   guint16  length)
{
	hg_object_t *retval;
	hg_stringdata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_stringdata_t) + length + 1));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_STRING;
		HG_OBJECT_STRING (retval)->length = length;
		HG_OBJECT_STRING (retval)->real_length = length;
		data = HG_OBJECT_STRING_DATA (retval);
		if (length > 0)
			data->string = (gpointer)data->data;
		else
			data->string = NULL;
	}

	return retval;
}

hg_object_t *
hg_object_string_new(hg_vm_t     *vm,
		     const gchar *value)
{
	hg_object_t *retval;
	gchar *string;
	gsize size;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (value != NULL, NULL);

	size = strlen(value);
	retval = hg_object_string_sized_new(vm, size);
	if (retval != NULL) {
		string = HG_OBJECT_STRING_DATA (retval)->string;
		memcpy(string, value, size);
	}

	return retval;
}

hg_object_t *
hg_object_string_substring_new(hg_vm_t     *vm,
			       hg_object_t *object,
			       guint16      start_index,
			       guint16      length)
{
	hg_object_t *retval;
	gchar *p;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_STRING (object), NULL);
	hg_return_val_if_fail (HG_OBJECT_STRING (object)->real_length > start_index, NULL);
	hg_return_val_if_fail (HG_OBJECT_STRING (object)->real_length >= (start_index + length), NULL);

	retval = hg_object_string_sized_new(vm, 0);
	if (retval != NULL) {
		p = HG_OBJECT_STRING_DATA (object)->string;
		HG_OBJECT_STRING_DATA (retval)->string = &p[start_index];
		HG_OBJECT_STRING (retval)->real_length = length;
	}

	return retval;
}

gboolean
hg_object_string_compare(hg_object_t *object1,
			 hg_object_t *object2)
{
	guint16 i;
	gchar *p1, *p2;

	hg_return_val_if_fail (object1 != NULL, FALSE);
	hg_return_val_if_fail (object2 != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_STRING (object1), FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_STRING (object2), FALSE);

	if (HG_OBJECT_STRING (object1)->real_length != HG_OBJECT_STRING (object2)->real_length)
		return FALSE;

	p1 = HG_OBJECT_STRING_DATA (object1)->string;
	p2 = HG_OBJECT_STRING_DATA (object2)->string;
	for (i = 0; i < HG_OBJECT_STRING (object1)->real_length; i++) {
		if (p1[i] != p2[i])
			return FALSE;
	}

	return TRUE;
}
