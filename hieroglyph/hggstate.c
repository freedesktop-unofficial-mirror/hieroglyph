/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggsate.c
 * Copyright (C) 2006-2011 Akira TAGOH
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

#include <string.h>
#include "hgarray.h"
#include "hgint.h"
#include "hgmem.h"
#include "hgpath.h"
#include "hgreal.h"
#include "hgutils.h"
#include "hggstate.h"

#include "hggstate.proto.h"

HG_DEFINE_VTABLE_WITH (gstate, NULL, NULL, NULL);

/*< private >*/
static hg_usize_t
_hg_object_gstate_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_gstate_t));
}

static hg_uint_t
_hg_object_gstate_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_gstate_initialize(hg_object_t *object,
			     va_list      args)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;

	gstate->qpath = Qnil;
	gstate->qclippath = Qnil;
	gstate->qdashpattern = Qnil;

	return TRUE;
}

static hg_quark_t
_hg_object_gstate_copy(hg_object_t             *object,
		       hg_quark_iterate_func_t  func,
		       hg_pointer_t             user_data,
		       hg_pointer_t            *ret)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object, *g = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (object->type == HG_TYPE_GSTATE, Qnil, HG_e_typecheck);

	if (object->on_copying != Qnil)
		return object->on_copying;

	object->on_copying = retval = hg_gstate_new(gstate->o.mem, (hg_pointer_t *)&g);
	if (retval != Qnil) {
		memcpy(&g->ctm, &gstate->ctm, sizeof (hg_gstate_t) - sizeof (hg_object_t) - (sizeof (hg_quark_t) * 3));
		if (gstate->qpath == Qnil) {
			g->qpath = Qnil;
		} else {
			g->qpath = func(gstate->qpath, user_data, NULL);
			if (g->qpath == Qnil) {
				hg_debug(HG_MSGCAT_GSTATE, "Unable to copy the path object");
				goto bail;
			}
			hg_mem_unref(gstate->o.mem,
				     g->qpath);
		}
		if (gstate->qclippath == Qnil) {
			g->qclippath = Qnil;
		} else {
			g->qclippath = func(gstate->qclippath, user_data, NULL);
			if (g->qclippath == Qnil) {
				hg_debug(HG_MSGCAT_GSTATE, "Unable to copy the clippath object");
				goto bail;
			}
			hg_mem_unref(gstate->o.mem,
				     g->qclippath);
		}
		if (gstate->qdashpattern == Qnil) {
			g->qdashpattern = Qnil;
		} else {
			g->qdashpattern = func(gstate->qdashpattern, user_data, NULL);
			if (g->qdashpattern == Qnil) {
				hg_debug(HG_MSGCAT_GSTATE, "Unable to copy the dashpattern object");
				goto bail;
			}
			hg_mem_unref(gstate->o.mem,
				     g->qdashpattern);
		}

		if (ret)
			*ret = g;
		else
			hg_mem_unlock_object(g->o.mem, retval);
	}
	goto finalize;
  bail:
	if (g) {
		hg_object_free(g->o.mem, retval);

		retval = Qnil;
	}
  finalize:
	object->on_copying = Qnil;

	return retval;
}

static hg_char_t *
_hg_object_gstate_to_cstr(hg_object_t             *object,
			  hg_quark_iterate_func_t  func,
			  hg_pointer_t             user_data)
{
	return hg_strdup("-gstate-");
}

static hg_bool_t
_hg_object_gstate_gc_mark(hg_object_t *object)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;

	if (!hg_mem_gc_mark(gstate->o.mem, gstate->qpath))
		return FALSE;
	if (!hg_mem_gc_mark(gstate->o.mem, gstate->qclippath))
		return FALSE;
	if (!hg_mem_gc_mark(gstate->o.mem, gstate->qdashpattern))
		return FALSE;

	return TRUE;
}

static hg_bool_t
_hg_object_gstate_compare(hg_object_t             *o1,
			  hg_object_t             *o2,
			  hg_quark_compare_func_t  func,
			  hg_pointer_t             user_data)
{
	return FALSE;
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
hg_gstate_new(hg_mem_t     *mem,
	      hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_gstate_t *gstate = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);

	retval = hg_object_new(mem, (hg_pointer_t *)&gstate, HG_TYPE_GSTATE, 0);

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
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (matrix != NULL, HG_e_typecheck);

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
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (matrix != NULL, HG_e_typecheck);

	memcpy(matrix, &gstate->ctm, sizeof (hg_matrix_t));
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
	hg_return_val_if_fail (gstate != NULL, Qnil, HG_e_typecheck);

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
	hg_uint_t mem_id;

	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (HG_IS_QPATH (qpath), HG_e_typecheck);

	mem_id = hg_quark_get_mem_id(gstate->o.self);

	hg_return_if_fail (hg_quark_has_mem_id(qpath, mem_id), HG_e_VMerror);

	gstate->qclippath = qpath;
	hg_mem_unref(gstate->o.mem, qpath);
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
	hg_return_val_if_fail (gstate != NULL, Qnil, HG_e_typecheck);

	return gstate->qclippath;
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
		       hg_real_t    red,
		       hg_real_t    green,
		       hg_real_t    blue)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

	gstate->color.type = HG_COLOR_RGB;
	gstate->color.is.rgb.red = red;
	gstate->color.is.rgb.green = green;
	gstate->color.is.rgb.blue = blue;
}

/**
 * hg_gstate_set_hsbcolor:
 * @gstate:
 * @hue:
 * @saturation:
 * @brightness:
 *
 * FIXME
 */
void
hg_gstate_set_hsbcolor(hg_gstate_t *gstate,
		       hg_real_t    hue,
		       hg_real_t    saturation,
		       hg_real_t    brightness)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

	gstate->color.type = HG_COLOR_HSB;
	gstate->color.is.hsb.hue = hue;
	gstate->color.is.hsb.saturation = saturation;
	gstate->color.is.hsb.brightness = brightness;
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
			hg_real_t    gray)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

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
			hg_real_t    width)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

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
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (linecap >= 0 && linecap < HG_LINECAP_END, HG_e_rangecheck);

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
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (linejoin >= 0 && linejoin < HG_LINEJOIN_END, HG_e_rangecheck);

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
hg_bool_t
hg_gstate_set_miterlimit(hg_gstate_t *gstate,
			 hg_real_t    miterlen)
{
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);

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
hg_bool_t
hg_gstate_set_dash(hg_gstate_t *gstate,
		   hg_quark_t   qpattern,
		   hg_real_t    offset)
{
	hg_array_t *a;
	hg_mem_t *mem;
	hg_uint_t id;
	hg_usize_t i;
	hg_bool_t is_zero = TRUE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (HG_IS_QARRAY (qpattern), FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	id = hg_quark_get_mem_id(qpattern);

	hg_return_val_if_fail ((mem = hg_mem_spool_get(id)) != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_lock_fail (a, mem, qpattern, FALSE);

	if (hg_array_length(a) > 11) {
		hg_debug(HG_MSGCAT_GSTATE, "Array size for dash pattern is too big");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_limitcheck);
		goto finalize;
	}
	if (hg_array_length(a) > 0) {
		for (i = 0; i < hg_array_length(a); i++) {
			hg_quark_t q = hg_array_get(a, i);

			if (!HG_ERROR_IS_SUCCESS0 ())
				return FALSE;
			if (HG_IS_QINT (q)) {
				if (HG_INT (q) != 0)
					is_zero = FALSE;
			} else if (HG_IS_QREAL (q)) {
				if (!HG_REAL_IS_ZERO (q))
					is_zero = FALSE;
			} else {
				hg_debug(HG_MSGCAT_GSTATE, "Dash pattern contains non-numeric.");
				hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_typecheck);
				goto finalize;
			}
		}
		if (is_zero) {
			hg_debug(HG_MSGCAT_GSTATE, "No patterns in Array");
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_rangecheck);
			goto finalize;
		}
	}

	gstate->qdashpattern = qpattern;
	gstate->dash_offset = offset;
  finalize:
	hg_mem_unlock_object(mem, qpattern);

	return HG_ERROR_IS_SUCCESS0 ();
}

/* hg_path_t wrappers */

/**
 * hg_gstate_newpath:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_newpath(hg_gstate_t *gstate)
{
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	gstate->qpath = hg_path_new(gstate->o.mem, NULL);

	return gstate->qpath != Qnil;
}

/**
 * hg_gstate_closepath:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_closepath(hg_gstate_t *gstate)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_close(p);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_moveto:
 * @gstate:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_moveto(hg_gstate_t *gstate,
		 hg_real_t    x,
		 hg_real_t    y)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	if (gstate->qpath == Qnil) {
		gstate->qpath = hg_path_new(gstate->o.mem, NULL);
		if (gstate->qpath == Qnil)
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
	}
	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_moveto(p, x, y);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_rmoveto:
 * @gstate:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_rmoveto(hg_gstate_t *gstate,
		  hg_real_t    rx,
		  hg_real_t    ry)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_rmoveto(p, rx, ry);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_lineto:
 * @gstate:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_lineto(hg_gstate_t *gstate,
		 hg_real_t    x,
		 hg_real_t    y)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_lineto(p, x, y);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_rlineto:
 * @gstate:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_rlineto(hg_gstate_t *gstate,
		  hg_real_t    rx,
		  hg_real_t    ry)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_rlineto(p, rx, ry);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_curveto:
 * @gstate:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @x3:
 * @y3:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_curveto(hg_gstate_t *gstate,
		  hg_real_t    x1,
		  hg_real_t    y1,
		  hg_real_t    x2,
		  hg_real_t    y2,
		  hg_real_t    x3,
		  hg_real_t    y3)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_curveto(p, x1, y1, x2, y2, x3, y3);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_rcurveto:
 * @gstate:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @x3:
 * @y3:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_rcurveto(hg_gstate_t *gstate,
		   hg_real_t    rx1,
		   hg_real_t    ry1,
		   hg_real_t    rx2,
		   hg_real_t    ry2,
		   hg_real_t    rx3,
		   hg_real_t    ry3)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_rcurveto(p, rx1, ry1, rx2, ry2, rx3, ry3);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_arc:
 * @gstate:
 * @x:
 * @y:
 * @r:
 * @angle1:
 * @angle2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_arc(hg_gstate_t *gstate,
	      hg_real_t    x,
	      hg_real_t    y,
	      hg_real_t    r,
	      hg_real_t    angle1,
	      hg_real_t    angle2)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	if (gstate->qpath == Qnil) {
		gstate->qpath = hg_path_new(gstate->o.mem, NULL);
		if (gstate->qpath == Qnil)
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
	}
	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_arc(p, x, y, r, angle1, angle2);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_arcn:
 * @gstate:
 * @x:
 * @y:
 * @r:
 * @angle1:
 * @angle2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_arcn(hg_gstate_t *gstate,
	       hg_real_t    x,
	       hg_real_t    y,
	       hg_real_t    r,
	       hg_real_t    angle1,
	       hg_real_t    angle2)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	if (gstate->qpath == Qnil) {
		gstate->qpath = hg_path_new(gstate->o.mem, NULL);
		if (gstate->qpath == Qnil)
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
	}
	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_arcn(p, x, y, r, angle1, angle2);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_arcto:
 * @gstate:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @r:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_arcto(hg_gstate_t *gstate,
		hg_real_t    x1,
		hg_real_t    y1,
		hg_real_t    x2,
		hg_real_t    y2,
		hg_real_t    r,
		hg_real_t    ret[4])
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_arcto(p, x1, y1, x2, y2, r, ret);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_pathbbox:
 * @gstate:
 * @ignore_last_moveto:
 * @bbox:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_pathbbox(hg_gstate_t    *gstate,
		   hg_bool_t       ignore_last_moveto,
		   hg_path_bbox_t *bbox)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_get_bbox(p,
					  ignore_last_moveto,
					  bbox);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}

/**
 * hg_gstate_initclip:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_initclip(hg_gstate_t *gstate)
{
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	/* expecting the current path is properly set */
	if (gstate->qpath == Qnil)
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);

	gstate->qclippath = gstate->qpath;
	gstate->qpath = hg_path_new(gstate->o.mem, NULL);
	if (gstate->qpath == Qnil)
		hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);

	hg_error_return (HG_STATUS_SUCCESS, 0);
}

/**
 * hg_gstate_clippath:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_clippath(hg_gstate_t *gstate)
{
	hg_quark_t q;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	q = hg_object_quark_copy(gstate->o.mem, gstate->qclippath, NULL);
	if (q == Qnil)
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);

	gstate->qpath = q;

	hg_error_return (HG_STATUS_SUCCESS, 0);
}

/**
 * hg_gstate_get_current_point:
 * @gstate:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_get_current_point(hg_gstate_t *gstate,
			    hg_real_t   *x,
			    hg_real_t   *y)
{
	hg_path_t *p;
	hg_bool_t retval = FALSE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (gstate->o.type == HG_TYPE_GSTATE, FALSE, HG_e_typecheck);

	p = HG_MEM_LOCK (gstate->o.mem, gstate->qpath);
	if (p) {
		retval = hg_path_get_current_point(p, x, y);
		hg_mem_unlock_object(gstate->o.mem, gstate->qpath);
	}

	return retval;
}
