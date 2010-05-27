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
#include "hgencoding.h"
#include "hgerror.h"
#include "hgmem.h"
#include "hgutils.h"
#include "hgname.h"


static hg_quark_t _hg_object_name_new(hg_mem_t              *mem,
				      gpointer              *ret,
				      hg_object_name_type_t  type,
				      ...);

HG_DEFINE_VTABLE (name)

/*< private >*/
static gsize
_hg_object_name_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_object_name_t));
}

static void
_hg_object_name_initialize(hg_mem_t    *mem,
			   hg_object_t *object,
			   va_list      args)
{
	/* nothing */
}

static void
_hg_object_name_free(hg_mem_t    *mem,
		     hg_object_t *object)
{
	/* nothing */
}

static hg_quark_t
_hg_object_name_new(hg_mem_t              *mem,
		    gpointer              *ret,
		    hg_object_name_type_t  type,
		    ...)
{
	hg_object_name_t *n = NULL;
	va_list ap;
	gsize prealloc = 0;
	const gchar *name = NULL;
	hg_system_encoding_t enc = HG_enc_END;
	hg_quark_t retval;

	va_start(ap, type);
	/* pre-process */
	switch (type) {
	    case HG_name_offset:
		    name = va_arg(ap, const gchar *);
		    prealloc = va_arg(ap, gsize) + 1;
		    break;
	    case HG_name_index:
		    enc = va_arg(ap, hg_system_encoding_t);
		    break;
	    default:
		    return Qnil;
	}
	retval = hg_object_new(mem, (gpointer)&n, HG_TYPE_NAME, prealloc, Qnil);
	if (retval != Qnil) {
		n->representation = type;
		/* post-process */
		switch (type) {
		    case HG_name_offset:
			    n->representation = prealloc - 1;
			    n->v.offset = sizeof (hg_object_name_t);
			    memcpy((gchar *)n + n->v.offset, name, prealloc);
			    break;
		    case HG_name_index:
			    n->v.index = enc;
			    break;
		    default:
			    break;
		}
		if (ret)
			*ret = n;
	}

	va_end(ap);

	return retval;
}

/*< public >*/
/**
 * hg_object_name_new_with_encoding:
 * @mem:
 * @encoding:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_object_name_new_with_encoding(hg_mem_t             *mem,
				 hg_system_encoding_t  encoding,
				 gpointer             *ret)
{
	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (encoding < HG_enc_POSTSCRIPT_RESERVED_END, Qnil);

	return _hg_object_name_new(mem, ret, HG_name_index, encoding);
}

/**
 * hg_object_name_new_with_string:
 * @mem:
 * @name:
 * @len:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_object_name_new_with_string(hg_mem_t    *mem,
			       const gchar *name,
			       gssize       len,
			       gpointer    *ret)
{
	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (name != NULL, Qnil);

	if (len < 0)
		len = strlen(name);

	return _hg_object_name_new(mem, ret, HG_name_offset, name, len);
}

/**
 * hg_object_name_get_name:
 * @mem:
 * @index:
 *
 * FIXME
 *
 * Returns:
 */
const gchar *
hg_object_name_get_name(hg_mem_t   *mem,
			hg_quark_t  index)
{
	hg_object_name_t *n;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (index != Qnil, NULL);
	hg_return_val_if_fail (hg_quark_get_type (index) == HG_TYPE_NAME, NULL);

	n = (hg_object_name_t *)hg_mem_lock_object(mem, index);
	if (n->representation > 0) {
		return (const gchar *)n + n->v.offset;
	} else if (n->representation == -1) {
		return hg_encoding_get_system_encoding_name(n->v.index);
	} else {
	}

	return NULL;
}
