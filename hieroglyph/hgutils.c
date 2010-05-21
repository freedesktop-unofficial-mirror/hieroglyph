/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgutils.c
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

#include <math.h>
#include "hgutils.h"


/*< private >*/

/*< public >*/
/**
 * hg_get_quark_mask:
 * @pointer:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
guint32
hg_get_quark_mask(gsize size)
{
	return (1 << ((guint32)log2((double)size) + 1)) - 1;
}

/**
 * hg_get_quark:
 * @pointer:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_get_quark(gpointer pointer,
	     gsize    size)
{
	return (gsize)pointer & ~hg_get_quark_mask(size);
}
