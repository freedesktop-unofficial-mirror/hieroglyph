/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstring.c
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "hgstring.h"
#include "hgmem.h"

#define HG_STRING_ALLOC_SIZE	256

struct _HieroGlyphString {
	HgObject  object;
	gchar    *strings;
	gchar    *current;
	gint32    n_prealloc;
	gint32    allocated_size;
	guint     length;
	guint     substring_offset;
};


static void     _hg_string_real_set_flags(gpointer           data,
					  guint              flags);
static void     _hg_string_real_relocate (gpointer           data,
					  HgMemRelocateInfo *info);
static gpointer _hg_string_real_to_string(gpointer           data);


static HgObjectVTable __hg_string_vtable = {
	.free      = NULL,
	.set_flags = _hg_string_real_set_flags,
	.relocate  = _hg_string_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = _hg_string_real_to_string,
};

/*
 * Private Functions
 */
static void
_hg_string_real_set_flags(gpointer data,
			  guint    flags)
{
	HgString *string = data;
	HgMemObject *obj;

	if (string->strings) {
		/* down a restorable bit */
		flags &= ~HG_FL_RESTORABLE;
		hg_mem_get_object__inline(string->strings, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: String", string->strings);
		} else {
#ifdef DEBUG_GC
			G_STMT_START {
				if ((flags & HG_FL_MARK) != 0) {
					g_print("%s: marking string %p\n", __FUNCTION__, obj);
				}
			} G_STMT_END;
#endif /* DEBUG_GC */
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_string_real_relocate(gpointer           data,
			 HgMemRelocateInfo *info)
{
	HgString *string = data;

	if ((gsize)string->strings >= info->start &&
	    (gsize)string->strings <= info->end) {
		string->strings = (gchar *)((gsize)string->strings + info->diff);
		string->current = (gchar *)((gsize)string->strings + sizeof (gchar) * string->substring_offset);
	}
}

static gpointer
_hg_string_real_to_string(gpointer data)
{
	HgString *retval, *str = data;
	HgMemObject *obj;
	guint i;
	gchar buffer[8];

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_string_new(obj->pool, -1);
	if (retval == NULL)
		return NULL;
	if (!hg_object_is_readable(data)) {
		hg_string_append(retval, "-string-", 8);
		hg_string_fix_string_size(retval);

		return retval;
	}
	for (i = 0; i < str->allocated_size; i++) {
		if (isprint(str->current[i])) {
			hg_string_append_c(retval, str->current[i]);
		} else {
			switch (str->current[i]) {
			    case '\n':
				    hg_string_append(retval, "\\n", 2);
				    break;
			    case '\r':
				    hg_string_append(retval, "\\r", 2);
				    break;
			    case '\t':
				    hg_string_append(retval, "\\t", 2);
				    break;
			    case '\b':
				    hg_string_append(retval, "\\b", 2);
				    break;
			    case '\f':
				    hg_string_append(retval, "\\f", 2);
				    break;
			    default:
				    snprintf(buffer, 8, "%04o", str->current[i]);
				    hg_string_append_c(retval, '\\');
				    hg_string_append(retval, buffer, -1);
				    break;
			}
		}
	}
	hg_string_fix_string_size(retval);

	return retval;
}

/*
 * Public Functions
 */
HgString *
hg_string_new(HgMemPool *pool,
	      gint32     n_prealloc)
{
	HgString *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool,
					 sizeof (HgString),
					 HG_FL_COMPLEX);
	if (retval == NULL) {
		g_warning("Failed to create a string.");
		return NULL;
	}
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__hg_string_vtable;

	retval->n_prealloc = n_prealloc;
	if (n_prealloc < 0) {
		retval->allocated_size = HG_STRING_ALLOC_SIZE;
	} else {
		retval->allocated_size = n_prealloc;
	}
	retval->length = 0;
	/* initialize this first to avoid a warning message */
	retval->strings = NULL;
	retval->strings = hg_mem_alloc(pool, sizeof (gchar) * (retval->allocated_size + 1));
	retval->current = retval->strings;
	if (retval->strings == NULL) {
		hg_mem_free(retval);
		g_warning("Failed to create a string.");
		return NULL;
	}

	return retval;
}

guint
hg_string_length(HgString *string)
{
	g_return_val_if_fail (string != NULL, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), 0);

	return string->length;
}

guint
hg_string_maxlength(HgString *string)
{
	g_return_val_if_fail (string != NULL, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), 0);

	return string->allocated_size;
}

gboolean
hg_string_clear(HgString *string)
{
	g_return_val_if_fail (string != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)string), FALSE);

	string->length = 0;
	string->current[0] = 0;

	return TRUE;
}

gboolean
hg_string_append_c(HgString *string,
		   gchar     c)
{
	g_return_val_if_fail (string != NULL, FALSE);
	g_return_val_if_fail (string->strings == string->current, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)string), FALSE);

	if (string->length < string->allocated_size) {
		string->current[string->length++] = c;
	} else if (string->n_prealloc < 0) {
		/* resize */
		gpointer p = hg_mem_resize(string->strings, string->allocated_size + HG_STRING_ALLOC_SIZE + 1);

		if (p == NULL) {
			g_warning("Failed to resize a string.");
			return FALSE;
		} else {
			string->current = string->strings = p;
			string->allocated_size += HG_STRING_ALLOC_SIZE;
			string->strings[string->length++] = c;
		}
	}

	return TRUE;
}

gboolean
hg_string_append(HgString    *string,
		 const gchar *str,
		 gint         length)
{
	gint i;

	g_return_val_if_fail (string != NULL, FALSE);
	g_return_val_if_fail (str != NULL, FALSE);
	g_return_val_if_fail (string->strings == string->current, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)string), FALSE);

	if (length < 0)
		length = strlen(str);
	if (string->n_prealloc < 0 &&
	    (string->length + length) >= string->allocated_size) {
		/* resize */
		gsize n_unit = (string->length + length + HG_STRING_ALLOC_SIZE + 1) / HG_STRING_ALLOC_SIZE;
		gpointer p = hg_mem_resize(string->strings, n_unit * HG_STRING_ALLOC_SIZE);

		if (p == NULL) {
			g_warning("Failed to resize a string.");
			return FALSE;
		} else {
			string->current = string->strings = p;
			string->allocated_size = n_unit * HG_STRING_ALLOC_SIZE;
		}
	}
	for (i = 0; i < length && string->length < string->allocated_size; i++) {
		string->strings[string->length++] = str[i];
	}
	string->strings[string->length] = 0;

	return TRUE;
}

gboolean
hg_string_insert_c(HgString *string,
		   gchar     c,
		   guint     index)
{
	guint i;

	g_return_val_if_fail (string != NULL, FALSE);
	g_return_val_if_fail (index < string->allocated_size, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)string), FALSE);

	string->current[index] = c;
	/* update null-terminated string length */
	for (i = 0; i < string->allocated_size; i++) {
		if (string->current[i] == 0) {
			string->length = i;
			return TRUE;
		}
	}
	string->length = string->allocated_size;

	return TRUE;
}

gboolean
hg_string_concat(HgString *string1,
		 HgString *string2)
{
	g_return_val_if_fail (string1 != NULL, FALSE);
	g_return_val_if_fail (string2 != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string1), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)string1), FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string2), FALSE);

	return hg_string_append(string1, string2->current, string2->length);
}

gchar
hg_string_index(HgString *string,
		guint     index)
{
	g_return_val_if_fail (string != NULL, 0);
	g_return_val_if_fail (string->length > index, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);

	return string->current[index];
}

gchar *
hg_string_get_string(HgString *string)
{
	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);

	if (string->strings == string->current) {
		/* don't break strings on sub-string */
		string->current[string->length] = 0;
	}

	return string->current;
}

gboolean
hg_string_fix_string_size(HgString *string)
{
	g_return_val_if_fail (string != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)string), FALSE);

	if (string->n_prealloc < 0) {
		gpointer p;

		p = hg_mem_resize(string->strings, string->length + 1);
		if (p == NULL) {
			g_warning("Failed to resize a string.");
			return FALSE;
		}
		string->strings = p;
		string->allocated_size = string->n_prealloc = string->length;
	}

	return TRUE;
}

gboolean
hg_string_compare(const HgString *a,
		  const HgString *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)a), FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)b), FALSE);

	if (a->length != b->length)
		return FALSE;

	return memcmp(a->current, b->current, a->length) == 0;
}

gboolean
hg_string_compare_with_raw(const HgString *a,
			   const gchar    *b,
			   gint            length)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)a), FALSE);

	if (length < 0)
		length = strlen(b);
	if (a->length != length)
		return FALSE;

	return memcmp(a->current, b, length) == 0;
}

HgString *
hg_string_make_substring(HgMemPool *pool,
			 HgString  *string,
			 guint      start_index,
			 guint      end_index)
{
	HgString *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_string_new(pool, 0);
	if (!hg_string_copy_as_substring(string, retval, start_index, end_index))
		return NULL;

	return retval;
}

gboolean
hg_string_copy_as_substring(HgString *src,
			    HgString *dest,
			    guint     start_index,
			    guint     end_index)
{
	g_return_val_if_fail (src != NULL, FALSE);
	g_return_val_if_fail (dest != NULL, FALSE);
	g_return_val_if_fail (start_index < src->allocated_size, FALSE);
	g_return_val_if_fail (end_index < src->allocated_size, FALSE);
	g_return_val_if_fail (start_index <= end_index, FALSE);

	/* destroy the unnecessary destination strings */
	hg_mem_free(dest->strings);
	/* make a sub-string */
	dest->strings = src->strings;
	dest->current = (gpointer)((gsize)dest->strings + sizeof (gpointer) * start_index);
	dest->allocated_size = dest->n_prealloc = dest->length = end_index - start_index + 1;
	dest->substring_offset = start_index;

	return TRUE;
}

/* HgObject */
HgString *
hg_object_to_string(HgObject *object)
{
	HgString *retval;
	HgMemObject *obj;

	g_return_val_if_fail (object != NULL, NULL);

	if (object->vtable && object->vtable->to_string)
		return object->vtable->to_string(object);

	hg_mem_get_object__inline(object, obj);
	retval = hg_string_new(obj->pool, 16);
	hg_string_append(retval, "--nostringval--", -1);

	return retval;
}
