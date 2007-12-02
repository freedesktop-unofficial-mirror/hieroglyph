/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.c
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

#include <math.h>
#include <string.h>
#include <glib/gstring.h>
#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgoperator.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/vm.h>
#include "hgobject.h"


/*
 * Private functions
 */
static hg_object_t *
_hg_object_new(hg_vm_t  *vm,
	       gsize     data_size)
{
	hg_object_t *retval;
	guint32 total_size = hg_n_alignof (sizeof (hg_object_header_t) + sizeof (_hg_object_t) + data_size);
	hg_attribute_t attr;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_vm_malloc(vm, total_size);
	if (retval != NULL) {
#ifdef DEBUG
		/* We don't ensure whether or not the allocated object is sane
		 * to be used in C. any classes must have their own initializer
		 * as needed
		 */
		memset(HG_OBJECT_OBJECT (retval), -1, sizeof (_hg_object_t) + data_size);
#endif /* DEBUG */
		hg_vm_get_attributes(vm, &attr);
		HG_OBJECT_HEADER (retval)->token_type = hg_vm_get_object_format(vm);
		HG_OBJECT_HEADER (retval)->n_objects = 0xff;
		HG_OBJECT_HEADER (retval)->total_length = total_size;
		HG_OBJECT_OBJECT (retval)->attr.attributes = attr.attributes;
		HG_OBJECT_GET_TYPE (retval) = -1;
	} else {
		hg_vm_set_error(vm, HG_e_VMerror);
	}

	return retval;
}

/* name */
static gboolean
_hg_object_name_compare(const hg_object_t *object1,
			const hg_object_t *object2)
{
	guint16 i;
	gchar *p1, *p2;

	if (HG_OBJECT_ENCODING_NAME (object1)->representation == -1 &&
	    HG_OBJECT_ENCODING_NAME (object2)->representation == -1) {
		return HG_OBJECT_ENCODING_NAME (object1)->index == HG_OBJECT_ENCODING_NAME (object2)->index;
	} else if (HG_OBJECT_NAME (object1)->reserved1 == 0 &&
		   HG_OBJECT_NAME (object2)->reserved1 == 0) {
		if (HG_OBJECT_NAME (object1)->length != HG_OBJECT_NAME (object2)->length)
			return FALSE;

		p1 = HG_OBJECT_NAME_DATA (object1);
		p2 = HG_OBJECT_NAME_DATA (object2);
		for (i = 0; i < HG_OBJECT_NAME (object1)->length; i++) {
			if (p1[i] != p2[i])
				return FALSE;
		}
		return TRUE;
	}

	return FALSE;
}

/*
 * Public functions
 */
hg_object_t *
hg_object_new(hg_vm_t *vm,
	      guint16  n_objects)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (n_objects > 0, NULL);

	retval = _hg_object_new(vm, sizeof (_hg_object_t) * (n_objects - 1));
	if (retval != NULL) {
		/* initialize header */
		HG_OBJECT_HEADER (retval)->n_objects = n_objects;
	}

	return retval;
}

hg_object_t *
hg_object_sized_new(hg_vm_t *vm,
		    gsize    size)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = _hg_object_new(vm, size);
	if (retval != NULL) {
		/* initialize header */
		HG_OBJECT_HEADER (retval)->n_objects = 1;
	}

	return retval;
}

void
hg_object_free(hg_vm_t     *vm,
	       hg_object_t *object)
{
	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (object != NULL);

	switch (HG_OBJECT_GET_TYPE (object)) {
	    case HG_OBJECT_TYPE_NULL:
	    case HG_OBJECT_TYPE_INTEGER:
	    case HG_OBJECT_TYPE_REAL:
	    case HG_OBJECT_TYPE_NAME:
	    case HG_OBJECT_TYPE_BOOLEAN:
	    case HG_OBJECT_TYPE_STRING:
	    case HG_OBJECT_TYPE_EVAL:
	    case HG_OBJECT_TYPE_ARRAY:
	    case HG_OBJECT_TYPE_MARK:
	    case HG_OBJECT_TYPE_DICT:
	    case HG_OBJECT_TYPE_OPERATOR:
	    case HG_OBJECT_TYPE_END: /* uninitialized object */
		    hg_vm_mfree(vm, object);
		    break;
	    case HG_OBJECT_TYPE_FILE:
		    hg_object_file_free(vm, object);
		    break;
	    default:
		    g_warning("[BUG] Unknown object type `%d' during destroying", HG_OBJECT_GET_TYPE (object));
	}
}

hg_object_t *
hg_object_dup(hg_vm_t     *vm,
	      hg_object_t *object)
{
	hg_object_t *retval = NULL;
	gsize length;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (object != NULL, NULL);

	switch (HG_OBJECT_GET_TYPE (object)) {
	    case HG_OBJECT_TYPE_NULL:
	    case HG_OBJECT_TYPE_INTEGER:
	    case HG_OBJECT_TYPE_REAL:
	    case HG_OBJECT_TYPE_NAME:
	    case HG_OBJECT_TYPE_BOOLEAN:
	    case HG_OBJECT_TYPE_EVAL:
	    case HG_OBJECT_TYPE_MARK:
		    length = HG_OBJECT_HEADER (object)->total_length - sizeof (hg_object_header_t) - sizeof (_hg_object_t);
		    retval = hg_object_sized_new(vm, length);
		    if (retval != NULL) {
			    memcpy(retval, object, HG_OBJECT_HEADER (object)->total_length);
		    }
		    break;
	    case HG_OBJECT_TYPE_STRING:
		    retval = hg_object_string_substring_new(vm, object, 0, HG_OBJECT_STRING (object)->real_length);
		    break;
	    case HG_OBJECT_TYPE_ARRAY:
		    retval = hg_object_array_subarray_new(vm, object, 0, HG_OBJECT_ARRAY (object)->real_length);
		    break;
	    case HG_OBJECT_TYPE_DICT:
	    case HG_OBJECT_TYPE_FILE:
	    case HG_OBJECT_TYPE_OPERATOR:
		    /* XXX: perhaps we may want to create another container? */
		    retval = object;
		    break;
	    default:
		    g_warning("[BUG] Unknown object type `%d' during duplicating", HG_OBJECT_GET_TYPE (object));
		    break;
	}

	return retval;
}

hg_object_t *
hg_object_copy(hg_vm_t     *vm,
	       hg_object_t *object)
{
	hg_object_t *retval;
	gsize length;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (object != NULL, NULL);

	length = HG_OBJECT_HEADER (object)->total_length - sizeof (hg_object_header_t) - sizeof (_hg_object_t);
	retval = hg_object_sized_new(vm, length);
	if (retval != NULL) {
		memcpy(retval, object, HG_OBJECT_HEADER (object)->total_length);
	}

	return retval;
}

gboolean
hg_object_compare(hg_object_t *object1,
		  hg_object_t *object2)
{
	hg_return_val_if_fail (object1 != NULL, FALSE);
	hg_return_val_if_fail (object2 != NULL, FALSE);

	if (HG_OBJECT_GET_TYPE (object1) != HG_OBJECT_GET_TYPE (object2))
		return FALSE;
	if (object1 == object2)
		return TRUE;

	switch (HG_OBJECT_GET_TYPE (object1)) {
	    case HG_OBJECT_TYPE_NULL:
	    case HG_OBJECT_TYPE_MARK:
		    return TRUE;
	    case HG_OBJECT_TYPE_INTEGER:
		    return HG_OBJECT_INTEGER (object1) == HG_OBJECT_INTEGER (object2);
	    case HG_OBJECT_TYPE_REAL:
		    return HG_OBJECT_REAL_IS_EQUAL (object1, object2);
	    case HG_OBJECT_TYPE_NAME:
	    case HG_OBJECT_TYPE_EVAL:
		    return _hg_object_name_compare(object1, object2);
	    case HG_OBJECT_TYPE_BOOLEAN:
		    return HG_OBJECT_BOOLEAN (object1) == HG_OBJECT_BOOLEAN (object2);
	    case HG_OBJECT_TYPE_STRING:
		    return hg_object_string_compare(object1, object2);
	    case HG_OBJECT_TYPE_ARRAY:
		    return hg_object_array_compare(object1, object2);
	    case HG_OBJECT_TYPE_DICT:
		    return hg_object_dict_compare(object1, object2);
	    case HG_OBJECT_TYPE_FILE:
		    return hg_object_file_compare(object1, object2);
	    case HG_OBJECT_TYPE_OPERATOR:
		    return hg_object_operator_compare(object1, object2);
	    default:
		    g_warning("Unknown object type `%d' during comparing", HG_OBJECT_GET_TYPE (object1));
	}

	return FALSE;
}

gchar *
hg_object_dump(hg_object_t *object,
	       gboolean     verbose)
{
	GString *string;
	gsize i;
	gchar *p;
	hg_stringdata_t *sdata;
	static gchar nostringval[] = "--nostringval--";

	hg_return_val_if_fail (object != NULL, NULL);

	string = g_string_new(NULL);
	switch (HG_OBJECT_GET_TYPE (object)) {
	    case HG_OBJECT_TYPE_NULL:
		    if (verbose) {
			    g_string_append(string, "null");
		    } else {
			    g_string_append(string, nostringval);
		    }
		    break;
	    case HG_OBJECT_TYPE_MARK:
		    if (verbose) {
			    g_string_append(string, "-mark-");
		    } else {
			    g_string_append(string, nostringval);
		    }
		    break;
	    case HG_OBJECT_TYPE_INTEGER:
		    g_string_append_printf(string, "%d", HG_OBJECT_INTEGER (object));
		    break;
	    case HG_OBJECT_TYPE_REAL:
		    g_string_append_printf(string, "%.8f", HG_OBJECT_REAL (object));
		    break;
	    case HG_OBJECT_TYPE_EVAL:
		    if (verbose)
			    g_string_append_c(string, '/');
	    case HG_OBJECT_TYPE_NAME:
		    if (verbose)
			    g_string_append_c(string, '/');
		    if (HG_OBJECT_ENCODING_NAME (object)->representation == -1) {
			    g_string_append(string, hg_object_operator_get_name(HG_OBJECT_ENCODING_NAME (object)->index));
		    } else {
			    p = HG_OBJECT_NAME_DATA (object);

			    for (i = 0; i < HG_OBJECT_NAME (object)->length; i++) {
				    g_string_append_c(string, p[i]);
			    }
		    }
		    break;
	    case HG_OBJECT_TYPE_BOOLEAN:
		    g_string_append_printf(string, "%s", (HG_OBJECT_BOOLEAN (object) ? "true" : "false"));
		    break;
	    case HG_OBJECT_TYPE_STRING:
		    if (verbose)
			    g_string_append_c(string, '(');
		    sdata = HG_OBJECT_STRING_DATA (object);
		    for (i = 0; i < HG_OBJECT_STRING (object)->real_length; i++) {
			    g_string_append_c(string, ((gchar *)sdata->string)[i]);
		    }
		    if (verbose)
			    g_string_append_c(string, ')');
		    break;
	    case HG_OBJECT_TYPE_ARRAY:
		    return hg_object_array_dump(object, verbose);
	    case HG_OBJECT_TYPE_DICT:
		    return hg_object_dict_dump(object, verbose);
	    case HG_OBJECT_TYPE_FILE:
		    return hg_object_file_dump(object, verbose);
	    case HG_OBJECT_TYPE_OPERATOR:
		    return hg_object_operator_dump(object, verbose);
	    default:
		    g_warning("Unknown object type `%d' during dumping", HG_OBJECT_GET_TYPE (object));
	}

	return g_string_free(string, FALSE);
}

gsize
hg_object_get_hash(hg_object_t *object)
{
	gsize retval = 0;

	hg_return_val_if_fail (object != NULL, 0);

	switch (HG_OBJECT_GET_TYPE (object)) {
	    case HG_OBJECT_TYPE_NULL:
		    retval = 0;
		    break;
	    case HG_OBJECT_TYPE_INTEGER:
		    retval = HG_OBJECT_INTEGER (object);
		    break;
	    case HG_OBJECT_TYPE_REAL:
		    retval = HG_OBJECT_REAL (object);
		    break;
	    case HG_OBJECT_TYPE_NAME:
	    case HG_OBJECT_TYPE_EVAL:
		    G_STMT_START {
			    guint16 i, length;
			    const gchar *p;

			    if (HG_OBJECT_ENCODING_NAME (object)->representation == -1) {
				    p = hg_object_operator_get_name(HG_OBJECT_ENCODING_NAME (object)->index);
				    length = strlen(p);
			    } else {
				    p = HG_OBJECT_NAME_DATA (object);
				    length = HG_OBJECT_NAME (object)->length;
			    }
			    for (i = 0; i < length; i++) {
				    retval = (retval << 5) - retval + p[i];
			    }
		    } G_STMT_END;
		    break;
	    case HG_OBJECT_TYPE_BOOLEAN:
		    retval = HG_OBJECT_BOOLEAN (object);
		    break;
	    case HG_OBJECT_TYPE_STRING:
		    G_STMT_START {
			    guint16 i, length = HG_OBJECT_STRING (object)->real_length;
			    gchar *p = HG_OBJECT_STRING_DATA (object)->string;

			    for (i = 0; i < length; i++) {
				    retval = (retval << 5) - retval + p[i];
			    }
		    } G_STMT_END;
		    break;
	    case HG_OBJECT_TYPE_ARRAY:
	    case HG_OBJECT_TYPE_MARK:
	    case HG_OBJECT_TYPE_DICT:
	    case HG_OBJECT_TYPE_FILE:
	    case HG_OBJECT_TYPE_OPERATOR:
		    /* XXX */
	    default:
		    g_warning("Unknown object type `%d' during getting a hash", HG_OBJECT_GET_TYPE (object));
		    break;
	}

	return retval;
}

/* null */
hg_object_t *
hg_object_null_new(hg_vm_t *vm)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_new(vm, 1);
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_NULL;
	}

	return retval;
}

/* integer */
hg_object_t *
hg_object_integer_new(hg_vm_t *vm,
		      gint32   value)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_new(vm, 1);
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_INTEGER;
		HG_OBJECT_INTEGER (retval) = value;
	}

	return retval;
}

/* real */
hg_object_t *
hg_object_real_new(hg_vm_t *vm,
		   gfloat   value)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_new(vm, 1);
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_REAL;
		HG_OBJECT_REAL (retval) = value;
	}

	return retval;
}

/* name */
hg_object_t *
hg_object_name_new(hg_vm_t     *vm,
		   const gchar *value,
		   gboolean     is_evaluated)
{
	hg_object_t *retval;
	gsize size;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (value != NULL, NULL);

	size = strlen(value);
	retval = hg_object_sized_new(vm, size + 1);
	if (retval != NULL) {
		if (is_evaluated) {
			HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_EVAL;
		} else {
			HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_NAME;
		}
		/* initialize the reserved area to not confuse between
		 * the encoding named object and the named object.
		 */
		HG_OBJECT_NAME (retval)->reserved1 = 0;
		HG_OBJECT_NAME (retval)->length = size;
		memcpy(HG_OBJECT_DATA (retval), value, size);
	}

	return retval;
}

hg_object_t *
hg_object_system_encoding_new(hg_vm_t  *vm,
			      guint32   index,
			      gboolean  is_evaluated)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_new(vm, 1);
	if (retval != NULL) {
		if (is_evaluated) {
			HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_EVAL;
		} else {
			HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_NAME;
		}
		HG_OBJECT_ENCODING_NAME (retval)->representation = -1;
		HG_OBJECT_ENCODING_NAME (retval)->index = index;
	}

	return retval;
}

/* boolean */
hg_object_t *
hg_object_boolean_new(hg_vm_t  *vm,
		      gboolean  value)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_new(vm, 1);
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_BOOLEAN;
		HG_OBJECT_BOOLEAN (retval) = value;
	}

	return retval;
}

/* mark */
hg_object_t *
hg_object_mark_new(hg_vm_t *vm)
{
	hg_object_t *retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	retval = hg_object_new(vm, 1);
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_MARK;
	}

	return retval;
}
