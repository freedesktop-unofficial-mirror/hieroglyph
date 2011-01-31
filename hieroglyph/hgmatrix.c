/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmatrix.c
 * Copyright (C) 2005-2011 Akira TAGOH
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
#include <config.h>
#endif

#include <math.h>
#include <string.h>
#include "hgerror.h"
#include "hgmatrix.h"

#include "hgmatrix.proto.h"

/*< private >*/

/*< public >*/

/**
 * hg_matrix_init:
 * @matrix:
 * @xx:
 * @xy:
 * @yx:
 * @yy:
 * @x0:
 * @y0:
 *
 * FIXME
 */
void
hg_matrix_init(hg_matrix_t *matrix,
	       hg_real_t    xx,
	       hg_real_t    xy,
	       hg_real_t    yx,
	       hg_real_t    yy,
	       hg_real_t    x0,
	       hg_real_t    y0)
{
	hg_return_if_fail (matrix != NULL);

	matrix->mtx.xx = xx;
	matrix->mtx.xy = xy;
	matrix->mtx.yx = yx;
	matrix->mtx.yy = yy;
	matrix->mtx.x0 = x0;
	matrix->mtx.y0 = y0;
}

/**
 * hg_matrix_init_identity:
 * @matrix:
 *
 * FIXME
 */
void
hg_matrix_init_identity(hg_matrix_t *matrix)
{
	hg_matrix_init(matrix, 1, 0, 0, 1, 0, 0);
}

/**
 * hg_matrix_invert:
 * @matrix1:
 * @matrix2:
 *
 * FIXME
 */
hg_bool_t
hg_matrix_invert(hg_matrix_t *matrix1,
		 hg_matrix_t *matrix2)
{
	hg_real_t det;

	hg_return_val_if_fail (matrix1 != NULL, FALSE);
	hg_return_val_if_fail (matrix2 != NULL, FALSE);

	/*
	 * detA = a11a22a33 + a21a32a13 + a31a12a23
	 *      - a11a32a23 - a31a22a13 - a21a12a33
	 *
	 *      xx xy 0
	 * A = {yx yy 0}
	 *      x0 y0 1
	 * detA = xx * yy * 1 + yx * y0 * 0 + x0 * xy * 0 - xx * y0 * 0 - x0 * yy * 0 - yx * xy * 1
	 * detA = xx * yy - yx * xy
	 */
	det = matrix1->mtx.xx * matrix1->mtx.yy - matrix1->mtx.yx * matrix1->mtx.xy;
	if (det == 0)
		return FALSE;

	/*
	 *                    d      -b   0
	 * A-1 = 1/detA {    -c       a   0}
	 *               -dx0+by0 cx0-ay0 1
	 */
	matrix2->mtx.xx =  matrix1->mtx.yy / det;
	matrix2->mtx.xy = -matrix1->mtx.xy / det;
	matrix2->mtx.yx = -matrix1->mtx.yx / det;
	matrix2->mtx.yy =  matrix1->mtx.xx / det;
	matrix2->mtx.x0 = (-matrix1->mtx.yy * matrix1->mtx.x0 + matrix1->mtx.xy * matrix1->mtx.y0) / det;
	matrix2->mtx.y0 = ( matrix1->mtx.yx * matrix1->mtx.x0 - matrix1->mtx.xx * matrix1->mtx.y0) / det;

	return TRUE;
}

/**
 * hg_matrix_multiply:
 * @matrix1:
 * @matrix2:
 * @matrix3:
 *
 * FIXME
 */
void
hg_matrix_multiply(hg_matrix_t *matrix1,
		   hg_matrix_t *matrix2,
		   hg_matrix_t *matrix3)
{
	hg_matrix_t ret;

	hg_return_if_fail (matrix1 != NULL);
	hg_return_if_fail (matrix2 != NULL);
	hg_return_if_fail (matrix3 != NULL);

	ret.mtx.xx = matrix1->mtx.xx * matrix2->mtx.xx + matrix1->mtx.yx * matrix2->mtx.xy;
	ret.mtx.yx = matrix1->mtx.xx * matrix2->mtx.yx + matrix1->mtx.yx * matrix2->mtx.yy;
	ret.mtx.xy = matrix1->mtx.xy * matrix2->mtx.xx + matrix1->mtx.yy * matrix2->mtx.xy;
	ret.mtx.yy = matrix1->mtx.xy * matrix2->mtx.yx + matrix1->mtx.yy * matrix2->mtx.yy;
	ret.mtx.x0 = matrix1->mtx.x0 * matrix2->mtx.xx + matrix1->mtx.y0 * matrix2->mtx.xy + matrix2->mtx.x0;
	ret.mtx.y0 = matrix1->mtx.x0 * matrix2->mtx.yx + matrix1->mtx.y0 * matrix2->mtx.yy + matrix2->mtx.y0;

	memcpy(matrix3, &ret, sizeof (hg_matrix_t));
}

/**
 * hg_matrix_translate:
 * @matrix:
 * @tx:
 * @ty:
 *
 * FIXME
 */
void
hg_matrix_translate(hg_matrix_t *matrix,
		    hg_real_t    tx,
		    hg_real_t    ty)
{
	hg_matrix_t t;

	hg_return_if_fail (matrix != NULL);

	hg_matrix_init(&t, 1, 0, 0, 1, tx, ty);

	hg_matrix_multiply(&t, matrix, matrix);
}

/**
 * hg_matrix_rotate:
 * @matrix:
 * @angle:
 *
 * FIXME
 */
void
hg_matrix_rotate(hg_matrix_t *matrix,
		 hg_real_t    angle)
{
	hg_matrix_t t;
	hg_real_t cos_a = cos(angle / 180 * M_PI);
	hg_real_t sin_a = sin(angle / 180 * M_PI);

	hg_return_if_fail (matrix != NULL);

	hg_matrix_init(&t, cos_a, sin_a, -sin_a, cos_a, 0, 0);

	hg_matrix_multiply(&t, matrix, matrix);
}

/**
 * hg_matrix_get_affine:
 * @matrix:
 * @xx:
 * @xy:
 * @yx:
 * @yy:
 * @x0:
 * @y0:
 *
 * FIXME
 */
void
hg_matrix_get_affine(hg_matrix_t *matrix,
		     hg_real_t   *xx,
		     hg_real_t   *xy,
		     hg_real_t   *yx,
		     hg_real_t   *yy,
		     hg_real_t   *x0,
		     hg_real_t   *y0)
{
	hg_return_if_fail (matrix != NULL);

	if (xx)
		*xx = matrix->mtx.xx;
	if (xy)
		*xy = matrix->mtx.xy;
	if (yx)
		*yx = matrix->mtx.yx;
	if (yy)
		*yy = matrix->mtx.yy;
	if (x0)
		*x0 = matrix->mtx.x0;
	if (y0)
		*y0 = matrix->mtx.y0;
}
