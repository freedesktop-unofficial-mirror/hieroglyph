/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * iarray.c
 * Copyright (C) 2006 Akira TAGOH
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

#include "iarray.h"
#include "hgmem.h"

#define HG_INT_ARRAY_ALLOC_SIZE		256

struct _HieroGlyphIntArray {
	HgObject  object;
	gint32   *arrays;
	guint     n_arrays;
	gint32    allocated_arrays;
	gboolean  is_fixed_size;
};


static void _hg_int_array_real_set_flags(gpointer           data,
					 guint              flags);
static void _hg_int_array_real_relocate (gpointer           data,
					 HgMemRelocateInfo *info);


static HgObjectVTable __hg_int_array_vtable = {
	.free      = NULL,
	.set_flags = _hg_int_array_real_set_flags,
	.relocate  = _hg_int_array_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_hg_int_array_real_set_flags(gpointer data,
			     guint    flags)
{
	HgIntArray *iarray = data;
	HgMemObject *obj;

	if (iarray->arrays) {
		hg_mem_get_object__inline(iarray->arrays, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: IntArray", iarray->arrays);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_int_array_real_relocate(gpointer           data,
			    HgMemRelocateInfo *info)
{
	HgIntArray *iarray = data;

	if ((gsize)iarray->arrays >= info->start &&
	    (gsize)iarray->arrays <= info->end) {
		iarray->arrays = (gint32 *)((gsize)iarray->arrays + info->diff);
	}
}

/*
 * Public Functions
 */
HgIntArray *
hg_int_array_new(HgMemPool *pool,
		 gint32     num)
{
	HgIntArray *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool,
					 sizeof (HgIntArray),
					 HG_FL_HGOBJECT | HG_FL_COMPLEX);
	if (retval == NULL)
		return NULL;

	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_int_array_vtable);

	retval->n_arrays = 0;
	if (num < 0) {
		retval->allocated_arrays = HG_INT_ARRAY_ALLOC_SIZE;
		retval->is_fixed_size = FALSE;
	} else {
		retval->allocated_arrays = num;
		retval->is_fixed_size = TRUE;
	}
	/* initialize arrays with NULL first to avoid a crash
	 * when the alloc size is too big and GC is necessary to be ran.
	 */
	retval->arrays = NULL;
	retval->arrays = hg_mem_alloc(pool, sizeof (gint32) * retval->allocated_arrays);
	if (retval->arrays == NULL)
		return NULL;

	return retval;
}

gboolean
hg_int_array_append(HgIntArray *iarray,
		    gint32      i)
{
	HgMemObject *obj;

	g_return_val_if_fail (iarray != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)iarray), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)iarray), FALSE);

	hg_mem_get_object__inline(iarray, obj);
	g_return_val_if_fail (obj != NULL, FALSE);

	if (!iarray->is_fixed_size &&
	    iarray->n_arrays >= iarray->allocated_arrays) {
		gpointer p = hg_mem_resize(iarray->arrays,
					   sizeof (gint32) * (iarray->allocated_arrays + HG_INT_ARRAY_ALLOC_SIZE));

		if (p == NULL) {
			g_warning("Failed to resize an int array.");
			return FALSE;
		} else {
			iarray->arrays = p;
			iarray->allocated_arrays += HG_INT_ARRAY_ALLOC_SIZE;
		}
	}
	if (iarray->n_arrays < iarray->allocated_arrays) {
		iarray->arrays[iarray->n_arrays++] = i;
	} else {
		return FALSE;
	}

	return TRUE;
}

gboolean
hg_int_array_replace(HgIntArray *iarray,
		     gint32      i,
		     guint       index)
{
	HgMemObject *obj;

	g_return_val_if_fail (iarray != NULL, FALSE);
	g_return_val_if_fail (index < iarray->n_arrays, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)iarray), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)iarray), FALSE);

	hg_mem_get_object__inline(iarray, obj);
	g_return_val_if_fail (obj != NULL, FALSE);

	iarray->arrays[index] = i;

	return TRUE;
}

gboolean
hg_int_array_remove(HgIntArray *iarray,
		    guint       index)
{
	guint i;

	g_return_val_if_fail (iarray != NULL, FALSE);
	g_return_val_if_fail (index < iarray->n_arrays, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)iarray), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)iarray), FALSE);

	for (i = index; i <= iarray->n_arrays - 1; i++) {
		iarray->arrays[i] = iarray->arrays[i + 1];
	}
	iarray->n_arrays--;

	return TRUE;
}

gint32
hg_int_array_index(const HgIntArray *iarray,
		   guint             index)
{
	g_return_val_if_fail (iarray != NULL, 0);
	g_return_val_if_fail (index < iarray->n_arrays, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)iarray), 0);

	return iarray->arrays[index];
}

guint
hg_int_array_length(HgIntArray *iarray)
{
	g_return_val_if_fail (iarray != NULL, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)iarray), 0);

	return iarray->n_arrays;
}
