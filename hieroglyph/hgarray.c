/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.c
 * Copyright (C) 2005-2006 Akira TAGOH
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

#include <string.h>
#include "hgarray.h"
#include "hgstring.h"
#include "hgbtree.h"
#include "hgmem.h"
#include "hgvaluenode.h"
#include "hgfile.h"

#define HG_ARRAY_ALLOC_SIZE 65535


struct _HieroGlyphArray {
	HgObject      object;
	HgValueNode **current;
	HgValueNode **arrays;
	guint         n_arrays;
	gint32        total_arrays;
	gint32        allocated_arrays;
	gint32        removed_arrays;
	guint         subarray_offset;
	gboolean      is_subarray;
};


static void     _hg_array_real_set_flags(gpointer           data,
					 guint              flags);
static void     _hg_array_real_relocate (gpointer           data,
					 HgMemRelocateInfo *info);
static gpointer _hg_array_real_dup      (gpointer           data);
static gpointer _hg_array_real_copy     (gpointer           data);
static gpointer _hg_array_real_to_string(gpointer           data);


static HgObjectVTable __hg_array_vtable = {
	.free      = NULL,
	.set_flags = _hg_array_real_set_flags,
	.relocate  = _hg_array_real_relocate,
	.dup       = _hg_array_real_dup,
	.copy      = _hg_array_real_copy,
	.to_string = _hg_array_real_to_string,
};

/*
 * Private Functions
 */
static void
_hg_array_real_set_flags(gpointer data,
			 guint    flags)
{
	HgArray *array = data;
	HgMemObject *obj;
	guint i;

	for (i = 0; i < array->n_arrays; i++) {
		hg_mem_get_object__inline(array->current[i], obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: Array %d", array->current[i], i);
		} else {
#ifdef DEBUG_GC
			G_STMT_START {
				if ((flags & HG_FL_MARK) != 0) {
					if (!hg_mem_is_flags__inline(obj, flags)) {
						hg_value_node_debug_print(__hg_file_stderr, HG_DEBUG_GC_MARK, HG_TYPE_VALUE_ARRAY, array, array->current[i], GUINT_TO_POINTER (i));
					} else {
						hg_value_node_debug_print(__hg_file_stderr, HG_DEBUG_GC_ALREADYMARK, HG_TYPE_VALUE_ARRAY, array, array->current[i], GUINT_TO_POINTER (i));
					}
				}
			} G_STMT_END;
#endif /* DEBUG_GC */
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	if (array->arrays) {
		hg_mem_get_object__inline(array->arrays, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: Array", array->arrays);
		} else {
#ifdef DEBUG_GC
			G_STMT_START {
				if ((flags & HG_FL_MARK) != 0) {
					hg_value_node_debug_print(__hg_file_stderr, HG_DEBUG_GC_MARK, HG_TYPE_VALUE_ARRAY, array, array->arrays, GUINT_TO_POINTER (-1));
				}
			} G_STMT_END;
#endif /* DEBUG_GC */
			hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_array_real_relocate(gpointer           data,
			HgMemRelocateInfo *info)
{
	HgArray *array = data;
	guint i;

	if ((gsize)array->arrays >= info->start &&
	    (gsize)array->arrays <= info->end) {
		array->arrays = (HgValueNode **)((gsize)array->arrays + info->diff);
		array->current = (gpointer)((gsize)array->arrays + sizeof (gpointer) * array->removed_arrays + sizeof (gpointer) * array->subarray_offset);
	}
	for (i = 0; i < array->n_arrays; i++) {
		if ((gsize)array->current[i] >= info->start &&
		    (gsize)array->current[i] <= info->end) {
			array->current[i] = (HgValueNode *)((gsize)array->current[i] + info->diff);
		}
	}
}

static gpointer
_hg_array_real_dup(gpointer data)
{
	HgArray *array = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;

	retval = hg_array_new(obj->pool, array->n_arrays);
	if (retval == NULL) {
		g_warning("Failed to duplicate an array.");
		return NULL;
	}
	memcpy(retval->arrays, array->current, sizeof (HgValueNode *) * array->n_arrays);
	retval->n_arrays = array->n_arrays;

	return retval;
}

static gpointer
_hg_array_real_copy(gpointer data)
{
	HgArray *array = data, *retval;
	HgMemObject *obj;
	guint i;
	gpointer p;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;

	if (hg_mem_is_copying(obj)) {
		/* circular reference happened. */
		g_warning("Circular reference happened in %p (mem: %p). copying entire object is impossible.", data, obj);
		return array;
	}
	hg_mem_set_copying(obj);
	retval = hg_array_new(obj->pool, array->n_arrays);
	if (retval == NULL) {
		g_warning("Failed to copy an array.");
		hg_mem_unset_copying(obj);
		return NULL;
	}
	for (i = 0; i < array->n_arrays; i++) {
		p = hg_object_copy((HgObject *)array->current[i]);
		if (p == NULL) {
			hg_mem_unset_copying(obj);
			return NULL;
		}
		retval->arrays[retval->n_arrays++] = p;
	}
	hg_mem_unset_copying(obj);

	return retval;
}

static gpointer
_hg_array_real_to_string(gpointer data)
{
	HgArray *array = data;
	HgMemObject *obj;
	HgString *retval, *str;
	guint i;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_string_new(obj->pool, -1);
	if (retval == NULL)
		return NULL;
	if (!hg_object_is_readable(data)) {
		hg_string_append(retval, "-array-", 7);
		hg_string_fix_string_size(retval);

		return retval;
	}
	if (hg_mem_is_copying(obj)) {
		/* circular reference happened. */
		gchar *name = g_strdup_printf("--circle-%p--", array);

		hg_string_append(retval, name, -1);
		hg_string_fix_string_size(retval);
		g_free(name);

		return retval;
	}
	hg_mem_set_copying(obj);
	for (i = 0; i < array->n_arrays; i++) {
		if (hg_string_length(retval) > 0)
			hg_string_append_c(retval, ' ');
		str = hg_object_to_string((HgObject *)array->current[i]);
		if (str == NULL) {
			hg_mem_free(retval);
			hg_mem_unset_copying(obj);
			return NULL;
		}
		if (!hg_string_concat(retval, str)) {
			hg_mem_free(retval);
			hg_mem_unset_copying(obj);
			return NULL;
		}
	}
	hg_string_fix_string_size(retval);
	hg_mem_unset_copying(obj);

	return retval;
}

/*
 * Public Functions
 */
HgArray *
hg_array_new(HgMemPool *pool,
	     gint32     num)
{
	HgArray *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool,
					 sizeof (HgArray),
					 HG_FL_RESTORABLE | HG_FL_COMPLEX);
	if (retval == NULL)
		return NULL;
	retval->object.id = HG_OBJECT_ID;
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_array_vtable);

	retval->n_arrays = 0;
	retval->removed_arrays = 0;
	retval->subarray_offset = 0;
	retval->is_subarray = FALSE;
	retval->total_arrays = num;
	if (retval->total_arrays < 0)
		retval->allocated_arrays = HG_ARRAY_ALLOC_SIZE;
	else
		retval->allocated_arrays = num;
	/* initialize arrays with NULL first to avoid a crash.
	 * when the alloc size is too big and GC is necessary to be ran.
	 */
	retval->arrays = NULL;
	retval->arrays = hg_mem_alloc_with_flags(pool,
						 sizeof (HgValueNode *) * retval->allocated_arrays,
						 HG_FL_RESTORABLE);
	retval->current = retval->arrays;
	if (retval->arrays == NULL)
		return NULL;

	return retval;
}

gboolean
hg_array_append(HgArray     *array,
		HgValueNode *node)
{
	return hg_array_append_forcibly(array, node, FALSE);
}

gboolean
hg_array_append_forcibly(HgArray     *array,
			 HgValueNode *node,
			 gboolean     force)
{
	HgMemObject *obj, *nobj;

	g_return_val_if_fail (array != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)array), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)array), FALSE);

	hg_mem_get_object__inline(array, obj);
	g_return_val_if_fail (obj != NULL, FALSE);
	hg_mem_get_object__inline(node, nobj);
	g_return_val_if_fail (nobj != NULL, FALSE);

	if (!force) {
		if (!hg_mem_pool_is_own_object(obj->pool, node)) {
			g_warning("node %p isn't allocated from a pool %s\n", node, hg_mem_pool_get_name(obj->pool));

			return FALSE;
		}
	}
	if (obj->pool != nobj->pool)
		hg_mem_add_pool_reference(nobj->pool, obj->pool);
	if (array->removed_arrays > 0) {
		/* remove the nodes forever */
		memmove(array->arrays, array->current, sizeof (gpointer) * array->n_arrays);
		array->removed_arrays = 0;
		array->current = array->arrays;
	}
	if (array->total_arrays < 0 &&
	    array->n_arrays >= array->allocated_arrays) {
		/* resize */
		gpointer p = hg_mem_resize(array->arrays,
					   sizeof (HgValueNode *) * (array->allocated_arrays + HG_ARRAY_ALLOC_SIZE));

		if (p == NULL) {
			g_warning("Failed to resize an array.");
			return FALSE;
		} else {
			array->current = array->arrays = p;
			array->allocated_arrays += HG_ARRAY_ALLOC_SIZE;
		}
	}
	if (array->n_arrays < array->allocated_arrays) {
		array->arrays[array->n_arrays++] = node;
		/* influence mark to child object */
		if (hg_mem_is_gc_mark(obj) && !hg_mem_is_gc_mark(nobj))
			hg_mem_gc_mark(nobj);
	} else {
		return FALSE;
	}

	return TRUE;
}

gboolean
hg_array_replace(HgArray     *array,
		 HgValueNode *node,
		 guint        index)
{
	return hg_array_replace_forcibly(array, node, index, FALSE);
}

gboolean
hg_array_replace_forcibly(HgArray     *array,
			  HgValueNode *node,
			  guint        index,
			  gboolean     force)
{
	HgMemObject *obj, *nobj;

	g_return_val_if_fail (array != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (index < array->n_arrays, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)array), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)array), FALSE);

	hg_mem_get_object__inline(array, obj);
	g_return_val_if_fail (obj != NULL, FALSE);
	hg_mem_get_object__inline(node, nobj);
	g_return_val_if_fail (nobj != NULL, FALSE);

	if (!force) {
		if (!hg_mem_pool_is_own_object(obj->pool, node)) {
			g_warning("node %p isn't allocated from a pool %s\n", node, hg_mem_pool_get_name(obj->pool));

			return FALSE;
		}
	}
	if (obj->pool != nobj->pool)
		hg_mem_add_pool_reference(nobj->pool, obj->pool);
	array->current[index] = node;
	/* influence mark to child object */
	if (hg_mem_is_gc_mark(obj) && !hg_mem_is_gc_mark(nobj))
		hg_mem_gc_mark(nobj);

	return TRUE;
}

gboolean
hg_array_remove(HgArray *array,
		guint    index)
{
	guint i;

	g_return_val_if_fail (array != NULL, FALSE);
	g_return_val_if_fail (index < array->n_arrays, FALSE);
	g_return_val_if_fail (array->is_subarray != TRUE, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)array), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)array), FALSE);

	if (index == 0) {
		/* This looks ugly hack but need to spend less time
		 * to remove an array node.
		 */
		array->removed_arrays++;
		array->current = (gpointer)((gsize)array->arrays + sizeof (gpointer) * array->removed_arrays);
	} else {
		for (i = index; i <= array->n_arrays - 1; i++) {
			array->current[i] = array->current[i + 1];
		}
	}
	array->n_arrays--;

	return TRUE;
}

HgValueNode *
hg_array_index(HgArray *array,
	       guint    index)
{
	g_return_val_if_fail (array != NULL, NULL);
	g_return_val_if_fail (index < array->n_arrays, NULL);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)array), NULL);

	return array->current[index];
}

guint
hg_array_length(HgArray *array)
{
	g_return_val_if_fail (array != NULL, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)array), 0);

	return array->n_arrays;
}

gboolean
hg_array_fix_array_size(HgArray *array)
{
	g_return_val_if_fail (array != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)array), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)array), FALSE);

	if (array->total_arrays < 0) {
		gpointer p;

		if (array->removed_arrays > 0) {
			/* array needs to be correct first */
			memmove(array->arrays, array->current, sizeof (gpointer) * array->n_arrays);
			array->removed_arrays = 0;
		}
		p = hg_mem_resize(array->arrays, sizeof (gpointer) * array->n_arrays);
		if (p == NULL) {
			g_warning("Failed to resize an array.");
			return FALSE;
		}
		array->current = array->arrays = p;
		array->allocated_arrays = array->total_arrays = array->n_arrays;
	}

	return TRUE;
}

HgArray *
hg_array_make_subarray(HgMemPool *pool,
		       HgArray   *array,
		       guint      start_index,
		       guint      end_index)
{
	HgArray *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_array_new(pool, 0);
	if (!hg_array_copy_as_subarray(array, retval, start_index, end_index))
		return NULL;

	return retval;
}

gboolean
hg_array_copy_as_subarray(HgArray *src,
			  HgArray *dest,
			  guint    start_index,
			  guint    end_index)
{
	g_return_val_if_fail (src != NULL, FALSE);
	g_return_val_if_fail (dest != NULL, FALSE);
	g_return_val_if_fail (start_index < src->n_arrays, FALSE);
	g_return_val_if_fail (end_index < src->n_arrays, FALSE);
	g_return_val_if_fail (start_index <= end_index, FALSE);

	/* validate an array */
	if (src->removed_arrays > 0) {
		/* remove the nodes forever */
		memmove(src->arrays, src->current, sizeof (gpointer) * src->n_arrays);
		src->removed_arrays = 0;
		src->current = src->arrays;
	}
	/* destroy the unnecessary destination arrays */
	hg_mem_free(dest->arrays);
	/* make a sub-array */
	dest->arrays = src->arrays;
	dest->current = (gpointer)((gsize)dest->arrays + sizeof (gpointer) * start_index);
	dest->allocated_arrays = dest->total_arrays = dest->n_arrays = end_index - start_index + 1;
	dest->removed_arrays = 0;
	dest->subarray_offset = start_index;
	dest->is_subarray = TRUE;

	return TRUE;
}
