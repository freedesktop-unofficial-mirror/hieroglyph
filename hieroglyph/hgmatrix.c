/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmatrix.c
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

#include <math.h>
#include "hgmatrix.h"
#include "hgmem.h"


/*
 * Private Functions
 */

/*
 * Public Functions
 */
HgMatrix *
hg_matrix_new(HgMemPool *pool,
	      gdouble    xx,
	      gdouble    yx,
	      gdouble    xy,
	      gdouble    yy,
	      gdouble    x0,
	      gdouble    y0)
{
	HgMatrix *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (HgMatrix));
	if (retval == NULL)
		return NULL;

	retval->xx = xx;
	retval->yx = yx;
	retval->xy = xy;
	retval->yy = yy;
	retval->x0 = x0;
	retval->y0 = y0;

	return retval;
}

HgMatrix *
hg_matrix_multiply(HgMemPool      *pool,
		   const HgMatrix *mtx1,
		   const HgMatrix *mtx2)
{
	gdouble xx, yx, xy, yy, x0, y0;

	g_return_val_if_fail (mtx1 != NULL, FALSE);
	g_return_val_if_fail (mtx2 != NULL, FALSE);

	xx = mtx1->xx * mtx2->xx + mtx1->yx * mtx2->xy;
	yx = mtx1->xx * mtx2->yx + mtx1->yx * mtx2->yy;
	xy = mtx1->xy * mtx2->xx + mtx1->yy * mtx2->xy;
	yy = mtx1->xy * mtx2->yx + mtx1->yy * mtx2->yy;
	x0 = mtx1->x0 * mtx2->xx + mtx1->y0 * mtx2->xy + mtx2->x0;
	y0 = mtx1->x0 * mtx2->yx + mtx1->y0 * mtx2->yy + mtx2->y0;

	return hg_matrix_new(pool, xx, yx, xy, yy, x0, y0);
}

HgMatrix *
hg_matrix_rotate(HgMemPool *pool,
		 gdouble    angle)
{
	gdouble c = cos(angle), s = sin(angle);

	return hg_matrix_new(pool,
			     c, s,
			     -s, c,
			     0.0, 0.0);
}

HgMatrix *
hg_matrix_scale(HgMemPool *pool,
		gdouble    x,
		gdouble    y)
{
	return hg_matrix_new(pool, x, 0.0, 0.0, y, 0.0, 0.0);
}

HgMatrix *
hg_matrix_translate(HgMemPool *pool,
		    gdouble    x,
		    gdouble    y)
{
	return hg_matrix_new(pool, 1.0, 0.0, 0.0, 1.0, x, y);
}
