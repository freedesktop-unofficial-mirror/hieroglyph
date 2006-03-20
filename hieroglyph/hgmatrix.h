/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmatrix.h
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
#ifndef __HG_MATRIX_H__
#define __HG_MATRIX_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

HgMatrix *hg_matrix_new      (HgMemPool      *pool,
			      gdouble         xx,
			      gdouble         yx,
			      gdouble         xy,
			      gdouble         yy,
			      gdouble         x0,
			      gdouble         y0);
HgMatrix *hg_matrix_multiply (HgMemPool      *pool,
			      const HgMatrix *mtx1,
			      const HgMatrix *mtx2);
HgMatrix *hg_matrix_rotate   (HgMemPool      *pool,
			      gdouble         angle);
HgMatrix *hg_matrix_scale    (HgMemPool      *pool,
			      gdouble         x,
			      gdouble         y);
HgMatrix *hg_matrix_translate(HgMemPool      *pool,
			      gdouble         x,
			      gdouble         y);


G_END_DECLS

#endif /* __HG_MATRIX_H__ */
