/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgnull.c
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
#include "hgnull.h"


static gsize _hg_object_null_get_capsulated_size(void);
static void  _hg_object_null_initialize         (hg_mem_t    *mem,
                                                 hg_object_t *object,
                                                 va_list      args);
static void  _hg_object_null_free               (hg_mem_t    *mem,
                                                 hg_object_t *object);


static hg_object_vtable_t __hg_object_null_vtable = {
	.get_capsulated_size = _hg_object_null_get_capsulated_size,
	.initialize          = _hg_object_null_initialize,
	.free                = _hg_object_null_free,
};

/*< private >*/
static gsize
_hg_object_null_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_object_null_t));
}

static void
_hg_object_null_initialize(hg_mem_t    *mem,
			  hg_object_t *object,
			  va_list      args)
{
	/* nothing */
}

static void
_hg_object_null_free(hg_mem_t    *mem,
		    hg_object_t *object)
{
	/* nothing */
}

/*< public >*/
hg_object_vtable_t *
hg_object_null_get_vtable(void)
{
	return &__hg_object_null_vtable;
}
