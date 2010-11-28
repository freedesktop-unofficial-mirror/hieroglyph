/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggsate.c
 * Copyright (C) 2006-2010 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hgarray.h"
#include "hgint.h"
#include "hgmem.h"
#include "hggstate.h"


HG_DEFINE_VTABLE_WITH (gstate, NULL, NULL, NULL);

/*< private >*/
static gsize
_hg_object_gstate_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_gstate_t));
}

static guint
_hg_object_gstate_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static gboolean
_hg_object_gstate_initialize(hg_object_t *object,
			     va_list      args)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;

	gstate->qpath = Qnil;
	gstate->qclippath = Qnil;

	return TRUE;
}

static hg_quark_t
_hg_object_gstate_copy(hg_object_t              *object,
		       hg_quark_iterate_func_t   func,
		       gpointer                  user_data,
		       gpointer                 *ret,
		       GError                  **error)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object, *g;
	hg_quark_t retval;
	GError *err = NULL;

	hg_return_val_if_fail (object->type == HG_TYPE_GSTATE, Qnil);

	if (object->on_copying != Qnil)
		return object->on_copying;

	object->on_copying = retval = hg_gstate_new(gstate->o.mem, (gpointer *)&g);
	if (retval != Qnil) {
		memcpy(&g->ctm, &gstate->ctm, sizeof (hg_gstate_t) - sizeof (hg_object_t) - (sizeof (hg_quark_t) * 3));
		g->qpath = func(gstate->qpath, user_data, NULL, &err);
		if (err)
			goto finalize;
		g->qclippath = func(gstate->qclippath, user_data, NULL, &err);
		if (err)
			goto finalize;
		g->qdashpattern = func(gstate->qdashpattern, user_data, NULL, &err);
		if (err)
			goto finalize;

		if (ret)
			*ret = g;
		else
			hg_mem_unlock_object(g->o.mem, retval);
	}
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
		hg_object_free(g->o.mem, retval);

		retval = Qnil;
	}
	object->on_copying = Qnil;

	return retval;
}

static gchar *
_hg_object_gstate_to_cstr(hg_object_t              *object,
			  hg_quark_iterate_func_t   func,
			  gpointer                  user_data,
			  GError                  **error)
{
	return g_strdup("-gstate-");
}

static gboolean
_hg_object_gstate_gc_mark(hg_object_t           *object,
			  hg_gc_iterate_func_t   func,
			  gpointer               user_data,
			  GError               **error)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;
	GError *err = NULL;
	gboolean retval = TRUE;

	if (!func(gstate->qpath, user_data, &err))
		goto finalize;
	if (!func(gstate->qclippath, user_data, &err))
		goto finalize;

  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

static gboolean
_hg_object_gstate_compare(hg_object_t             *o1,
			  hg_object_t             *o2,
			  hg_quark_compare_func_t  func,
			  gpointer                 user_data)
{
	return FALSE;
}

static hg_quark_t
_hg_gstate_iterate_copy(hg_quark_t   qdata,
			gpointer     user_data,
			gpointer    *ret,
			GError     **error)
{
	hg_quark_t retval;
	hg_object_t *o;
	hg_gstate_t *gstate = (hg_gstate_t *)user_data;

	if (hg_quark_is_simple_object(qdata))
		return qdata;

	hg_return_val_with_gerror_if_lock_fail (o, gstate->o.mem, qdata, error, Qnil);

	retval = hg_object_copy(o, _hg_gstate_iterate_copy, user_data, NULL, error);

	hg_mem_unlock_object(gstate->o.mem, qdata);

	return retval;
}

/*< public >*/
/**
 * hg_gstate_new:
 * @mem:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_new(hg_mem_t *mem,
	      gpointer *ret)
{
	hg_quark_t retval;
	hg_gstate_t *gstate = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil);

	retval = hg_object_new(mem, (gpointer *)&gstate, HG_TYPE_GSTATE, 0);

	if (ret)
		*ret = gstate;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_gstate_set_ctm:
 * @gstate:
 * @matrix:
 *
 * FIXME
 */
void
hg_gstate_set_ctm(hg_gstate_t *gstate,
		  hg_matrix_t *matrix)
{
	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (matrix != NULL);

	memcpy(&gstate->ctm, matrix, sizeof (hg_matrix_t));
}

/**
 * hg_gstate_get_ctm:
 * @gstate:
 * @matrix:
 *
 * FIXME
 */
void
hg_gstate_get_ctm(hg_gstate_t *gstate,
		  hg_matrix_t *matrix)
{
	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (matrix != NULL);

	memcpy(matrix, &gstate->ctm, sizeof (hg_matrix_t));
}

/**
 * hg_gstate_set_path:
 * @gstate:
 * @qpath:
 *
 * FIXME
 */
void
hg_gstate_set_path(hg_gstate_t *gstate,
		   hg_quark_t   qpath)
{
	guint mem_id;

	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (HG_IS_QPATH (qpath));

	mem_id = hg_quark_get_mem_id(gstate->o.self);

	hg_return_if_fail (hg_quark_has_same_mem_id(qpath, mem_id));

	gstate->qpath = qpath;
	hg_mem_reserved_spool_remove(gstate->o.mem, qpath);
}

/**
 * hg_gstate_get_path:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_get_path(hg_gstate_t *gstate)
{
	hg_return_val_if_fail (gstate != NULL, Qnil);

	return gstate->qpath;
}

/**
 * hg_gstate_set_clippath:
 * @gstate:
 * @qpath:
 *
 * FIXME
 */
void
hg_gstate_set_clippath(hg_gstate_t *gstate,
		       hg_quark_t   qpath)
{
	guint mem_id;

	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (HG_IS_QPATH (qpath));

	mem_id = hg_quark_get_mem_id(gstate->o.self);

	hg_return_if_fail (hg_quark_has_same_mem_id(qpath, mem_id));

	gstate->qclippath = qpath;
	hg_mem_reserved_spool_remove(gstate->o.mem, qpath);
}

/**
 * hg_gstate_get_clippath:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_get_clippath(hg_gstate_t *gstate)
{
	hg_return_val_if_fail (gstate != NULL, Qnil);

	return gstate->qclippath;
}

/**
 * hg_gstate_save:
 * @gstate:
 * @is_snapshot:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_save(hg_gstate_t *gstate,
	       hg_quark_t   is_snapshot)
{
	hg_quark_t retval;
	GError *err = NULL;
	hg_gstate_t *g;

	hg_return_val_if_fail (gstate != NULL, Qnil);

	retval = hg_object_copy((hg_object_t *)gstate,
				_hg_gstate_iterate_copy,
				gstate, (gpointer *)&g, &err);
	if (retval == Qnil)
		goto error;

	g->is_snapshot = is_snapshot;

	hg_mem_unlock_object(gstate->o.mem, retval);
  error:
	if (err) {
		g_warning("%s: %s (code: %d)",
			  __PRETTY_FUNCTION__,
			  err->message,
			  err->code);
		g_error_free(err);

		hg_object_free(gstate->o.mem,
			       retval);
		retval = Qnil;
	}

	return retval;
}

/**
 * hg_gstate_set_rgbcolor:
 * @gstate:
 * @red:
 * @green:
 * @blue:
 *
 * FIXME
 */
void
hg_gstate_set_rgbcolor(hg_gstate_t *gstate,
		       gdouble      red,
		       gdouble      green,
		       gdouble      blue)
{
	hg_return_if_fail (gstate != NULL);

	gstate->color.type = HG_COLOR_RGB;
	gstate->color.is.rgb.red = red;
	gstate->color.is.rgb.green = green;
	gstate->color.is.rgb.blue = blue;
}

/**
 * hg_gstate_set_gray:
 * @gstate:
 * @gray:
 *
 * FIXME
 */
void
hg_gstate_set_graycolor(hg_gstate_t *gstate,
			gdouble      gray)
{
	hg_return_if_fail (gstate != NULL);

	gstate->color.type = HG_COLOR_GRAY;
	gstate->color.is.rgb.red = gray;
	gstate->color.is.rgb.green = gray;
	gstate->color.is.rgb.blue = gray;
}

/**
 * hg_gstate_set_linewidth:
 * @gstate:
 * @width:
 *
 * FIXME
 */
void
hg_gstate_set_linewidth(hg_gstate_t *gstate,
			gdouble      width)
{
	hg_return_if_fail (gstate != NULL);

	gstate->linewidth = width;
}

/**
 * hg_gstate_set_linecap:
 * @gstate:
 * @linecap:
 *
 * FIXME
 */
void
hg_gstate_set_linecap(hg_gstate_t  *gstate,
		      hg_linecap_t  linecap)
{
	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (linecap >= 0 && linecap < HG_LINECAP_END);

	gstate->linecap = linecap;
}

/**
 * hg_gstate_set_linejoin:
 * @gstate:
 * @linejoin:
 *
 * FIXME
 */
void
hg_gstate_set_linejoin(hg_gstate_t   *gstate,
		       hg_linejoin_t  linejoin)
{
	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (linejoin >= 0 && linejoin < HG_LINEJOIN_END);

	gstate->linejoin = linejoin;
}

/**
 * hg_gstate_set_miterlimit:
 * @gstate:
 * @miterlen:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_gstate_set_miterlimit(hg_gstate_t *gstate,
			 gdouble      miterlen)
{
	hg_return_val_if_fail (gstate != NULL, FALSE);

	if (miterlen < 1.0)
		return FALSE;

	gstate->miterlen = miterlen;

	return TRUE;
}

/**
 * hg_gstate_set_dash:
 * @gstate:
 * @qpattern:
 * @offset:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_gstate_set_dash(hg_gstate_t  *gstate,
		   hg_quark_t    qpattern,
		   gdouble       offset,
		   GError      **error)
{
	hg_array_t *a;
	hg_mem_t *mem;
	guint id;
	gsize i;
	gboolean is_zero = TRUE;

	hg_return_val_with_gerror_if_fail (gstate != NULL, FALSE, error, HG_VM_e_VMerror);
	hg_return_val_with_gerror_if_fail (HG_IS_QARRAY (qpattern), FALSE, error, HG_VM_e_typecheck);
	hg_return_val_if_fail(error != NULL, FALSE);

	id = hg_quark_get_mem_id(qpattern);

	hg_return_val_with_gerror_if_fail ((mem = hg_mem_get(id)) != NULL, FALSE, error, HG_VM_e_VMerror);
	hg_return_val_with_gerror_if_lock_fail (a, mem, qpattern, error, FALSE);

	if (hg_array_length(a) > 11) {
		g_set_error(error, HG_ERROR, HG_VM_e_limitcheck,
			    "pattern array is too big.");
		goto finalize;
	}
	if (hg_array_length(a) > 0) {
		for (i = 0; i < hg_array_length(a); i++) {
			hg_quark_t q = hg_array_get(a, i, error);

			if (*error != NULL)
				return FALSE;
			if (HG_IS_QINT (q)) {
				if (HG_INT (q) != 0)
					is_zero = FALSE;
			} else if (HG_IS_QREAL (q)) {
				if (!HG_REAL_IS_ZERO (q))
					is_zero = FALSE;
			} else {
				g_set_error(error, HG_ERROR, HG_VM_e_typecheck,
					    "pattern contains non-numeric.");
				goto finalize;
			}
		}
		if (is_zero) {
			g_set_error(error, HG_ERROR, HG_VM_e_rangecheck,
				    "no patterns");
			goto finalize;
		}
	}

	gstate->qdashpattern = qpattern;
	gstate->dash_offset = offset;
  finalize:
	hg_mem_unlock_object(mem, qpattern);

	return *error == NULL;
}
