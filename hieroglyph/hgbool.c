/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbool.c
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

#include "hgerror.h"
#include "hgbool.h"


static gsize _hg_object_bool_get_capsulated_size(void);
static void  _hg_object_bool_initialize         (hg_mem_t    *mem,
                                                 hg_object_t *object,
                                                 va_list      args);
static void  _hg_object_bool_free               (hg_mem_t    *mem,
                                                 hg_object_t *object);


HG_DEFINE_VTABLE (bool)

/*< private >*/
static gsize
_hg_object_bool_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_object_bool_t));
}

static void
_hg_object_bool_initialize(hg_mem_t    *mem,
			   hg_object_t *object,
			   va_list      args)
{
	hg_object_bool_t *o;
	hg_quark_t v;
	hg_object_type_t t;

	/* ignore later values even if multiple values are given */
	v = (hg_quark_t)va_arg(args, hg_quark_t);
	if (v == Qnil)
		return;
	t = hg_quark_get_type (v);
	hg_return_if_fail (t == HG_TYPE_BOOL);

	o = (hg_object_bool_t *)object;
	o->value = (hg_quark_get_value (v) == TRUE);
}

static void
_hg_object_bool_free(hg_mem_t    *mem,
		     hg_object_t *object)
{
}

/*< public >*/
