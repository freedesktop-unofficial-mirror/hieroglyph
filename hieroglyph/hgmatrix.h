/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmatrix.h
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGMATRIX_H__
#define __HIEROGLYPH_HGMATRIX_H__

#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

typedef union _hg_matrix_t	hg_matrix_t;

union _hg_matrix_t {
	struct {
		hg_real_t xx;
		hg_real_t yx;
		hg_real_t xy;
		hg_real_t yy;
		hg_real_t x0;
		hg_real_t y0;
	} mtx;
	hg_real_t d[6];
};

void      hg_matrix_init         (hg_matrix_t *matrix,
                                  hg_real_t    xx,
                                  hg_real_t    xy,
                                  hg_real_t    yx,
                                  hg_real_t    yy,
                                  hg_real_t    x0,
                                  hg_real_t    y0);
void      hg_matrix_init_identity(hg_matrix_t *matrix);
hg_bool_t hg_matrix_invert       (hg_matrix_t *matrix1,
                                  hg_matrix_t *matrix2);
void      hg_matrix_multiply     (hg_matrix_t *matrix1,
                                  hg_matrix_t *matrix2,
                                  hg_matrix_t *matrix3);
void      hg_matrix_translate    (hg_matrix_t *matrix,
                                  hg_real_t    tx,
                                  hg_real_t    ty);
void      hg_matrix_rotate       (hg_matrix_t *matrix,
                                  hg_real_t    angle);
void      hg_matrix_get_affine   (hg_matrix_t *matrix,
                                  hg_real_t   *xx,
                                  hg_real_t   *xy,
                                  hg_real_t   *yx,
                                  hg_real_t   *yy,
                                  hg_real_t   *x0,
                                  hg_real_t   *y0);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGMATRIX_H__ */
