/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgarray.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGARRAY_H__
#define __HIEROGLYPH_HGARRAY_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

#define HG_ARRAY_INIT						\
	(hg_object_register(HG_TYPE_ARRAY,			\
			    hg_object_array_get_vtable()))
#define HG_IS_QARRAY(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_ARRAY)

typedef struct _hg_bs_array_t	hg_bs_array_t;
typedef struct _hg_array_t	hg_array_t;

struct _hg_bs_array_t {
	hg_bs_template_t t;
	guint16          length;
	guint32          offset;
};
struct _hg_array_t {
	hg_object_t o;
	hg_quark_t  qcontainer;
	hg_quark_t  qname;
	guint16     offset;
	guint16     length;
	gsize       allocated_size;
	gboolean    is_fixed_size:1;
	gboolean    is_subarray:1;
};


hg_object_vtable_t *hg_object_array_get_vtable(void) G_GNUC_CONST;
hg_quark_t          hg_array_new              (hg_mem_t                  *mem,
                                               gsize                      size,
                                               gpointer                  *ret);
void                hg_array_free             (hg_array_t                *array);
gboolean            hg_array_set              (hg_array_t                *array,
                                               hg_quark_t                 quark,
                                               gsize                      index,
					       gboolean                   force,
					       GError                   **error);
hg_quark_t          hg_array_get              (hg_array_t                *array,
                                               gsize                      index,
                                               GError                   **error);
gboolean            hg_array_insert           (hg_array_t                *array,
                                               hg_quark_t                 quark,
                                               gssize                     pos,
					       GError                   **error);
gboolean            hg_array_remove           (hg_array_t                *array,
                                               gsize                      pos);
gsize               hg_array_length           (hg_array_t                *array);
gsize               hg_array_maxlength        (hg_array_t                *array);
void                hg_array_foreach          (hg_array_t                *array,
                                               hg_array_traverse_func_t   func,
                                               gpointer                   data,
                                               GError                   **error);
void                hg_array_set_name         (hg_array_t                *array,
					       const gchar               *name);
hg_quark_t          hg_array_make_subarray    (hg_array_t                *array,
					       gsize                      start_index,
					       gsize                      end_index,
					       gpointer                  *ret,
					       GError                   **error);
gboolean            hg_array_copy_as_subarray (hg_array_t                *src,
					       hg_array_t                *dest,
					       gsize                      start_index,
					       gsize                      end_index,
					       GError                   **error);
hg_quark_t          hg_array_matrix_new       (hg_mem_t                  *mem,
					       gpointer                  *ret);
gboolean            hg_array_is_matrix        (hg_array_t                *array);
gboolean            hg_array_from_matrix      (hg_array_t                *array,
					       hg_matrix_t               *matrix);
gboolean            hg_array_to_matrix        (hg_array_t                *array,
					       hg_matrix_t               *matrix);
gboolean            hg_array_matrix_ident     (hg_array_t                *matrix);
gboolean            hg_array_matrix_multiply  (hg_array_t                *matrix1,
					       hg_array_t                *matrix2,
					       hg_array_t                *ret);
gboolean            hg_array_matrix_invert    (hg_array_t                *matrix,
					       hg_array_t                *ret);
gboolean            hg_array_matrix_translate (hg_array_t                *matrix,
					       gdouble                    tx,
					       gdouble                    ty);


/* hg_matrix_t */

G_INLINE_FUNC void hg_matrix_multiply (hg_matrix_t *matrix1,
				       hg_matrix_t *matrix2,
				       hg_matrix_t *ret);
G_INLINE_FUNC void hg_matrix_translate(hg_matrix_t *matrix,
				       gdouble      tx,
				       gdouble      ty);

/**
 * hg_matrix_multiply:
 * @matrix1:
 * @matrix2:
 * @ret:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_matrix_multiply(hg_matrix_t *matrix1,
		   hg_matrix_t *matrix2,
		   hg_matrix_t *ret)
{
	hg_matrix_t m3;

	hg_return_if_fail (matrix1 != NULL);
	hg_return_if_fail (matrix2 != NULL);
	hg_return_if_fail (ret != NULL);

	m3.mtx.xx = matrix1->mtx.xx * matrix2->mtx.xx + matrix1->mtx.yx * matrix2->mtx.xy;
	m3.mtx.yx = matrix1->mtx.xx * matrix2->mtx.yx + matrix1->mtx.yx * matrix2->mtx.yy;
	m3.mtx.xy = matrix1->mtx.xy * matrix2->mtx.xx + matrix1->mtx.yy * matrix2->mtx.xy;
	m3.mtx.yy = matrix1->mtx.xy * matrix2->mtx.yx + matrix1->mtx.yy * matrix2->mtx.yy;
	m3.mtx.x0 = matrix1->mtx.x0 * matrix2->mtx.xx + matrix1->mtx.y0 * matrix2->mtx.xy + matrix2->mtx.x0;
	m3.mtx.y0 = matrix1->mtx.x0 * matrix2->mtx.yx + matrix1->mtx.y0 * matrix2->mtx.yy + matrix2->mtx.y0;

	memcpy(ret, &m3, sizeof (hg_matrix_t));
}

/**
 * hg_matrix_translate:
 * @matrix:
 * @tx:
 * @ty:
 *
 * FIXME
 */
G_INLINE_FUNC void
hg_matrix_translate(hg_matrix_t *matrix,
		    gdouble      tx,
		    gdouble      ty)
{
	hg_matrix_t t;

	hg_return_if_fail (matrix != NULL);

	t.mtx.xx = 1;
	t.mtx.xy = 0;
	t.mtx.yx = 0;
	t.mtx.yy = 1;
	t.mtx.x0 = tx;
	t.mtx.y0 = ty;

	hg_matrix_multiply(matrix, &t, matrix);
}

G_END_DECLS

#endif /* __HIEROGLYPH_HGARRAY_H__ */
