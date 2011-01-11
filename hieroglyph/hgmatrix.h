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
#ifndef __HIEROGLYPH_HGMATRIX_H__
#define __HIEROGLYPH_HGMATRIX_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

union _hg_matrix_t {
	struct {
		gdouble xx;
		gdouble yx;
		gdouble xy;
		gdouble yy;
		gdouble x0;
		gdouble y0;
	} mtx;
	gdouble d[6];
};

void     hg_matrix_init         (hg_matrix_t *matrix,
                                 gdouble      xx,
                                 gdouble      xy,
                                 gdouble      yx,
                                 gdouble      yy,
                                 gdouble      x0,
                                 gdouble      y0);
void     hg_matrix_init_identity(hg_matrix_t *matrix);
gboolean hg_matrix_invert       (hg_matrix_t *matrix1,
                                 hg_matrix_t *matrix2);
void     hg_matrix_multiply     (hg_matrix_t *matrix1,
                                 hg_matrix_t *matrix2,
                                 hg_matrix_t *matrix3);
void     hg_matrix_translate    (hg_matrix_t *matrix,
                                 gdouble      tx,
                                 gdouble      ty);
void     hg_matrix_rotate       (hg_matrix_t *matrix,
                                 gdouble      angle);
void     hg_matrix_get_affine   (hg_matrix_t *matrix,
                                 gdouble     *xx,
                                 gdouble     *xy,
                                 gdouble     *yx,
                                 gdouble     *yy,
                                 gdouble     *x0,
                                 gdouble     *y0);

G_END_DECLS

#endif /* __HIEROGLYPH_HGMATRIX_H__ */
