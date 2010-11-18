/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpath.c
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

#include "hgmem.h"
#include "hgpath.h"

#define HG_PATH_MAX	1500

HG_DEFINE_VTABLE_WITH (path, NULL, NULL, NULL);

/*< private >*/
static gsize
_hg_object_path_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_path_t));
}

static guint
_hg_object_path_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT | HG_MEM_DROP_ON_RESTORE;
}

static gboolean
_hg_object_path_initialize(hg_object_t *object,
			   va_list      args)
{
	hg_path_t *path = (hg_path_t *)object;

	path->length = 0;
	path->qnode = hg_mem_alloc_with_flags(path->o.mem,
					      sizeof (hg_path_node_t) * HG_PATH_MAX,
					      HG_MEM_FLAGS_DEFAULT | HG_MEM_DROP_ON_RESTORE,
					      NULL);
	if (path->qnode == Qnil)
		return FALSE;

	return TRUE;
}

static hg_quark_t
_hg_object_path_copy(hg_object_t              *object,
		     hg_quark_iterate_func_t   func,
		     gpointer                  user_data,
		     gpointer                 *ret,
		     GError                  **error)
{
	hg_path_t *path = (hg_path_t *)object, *p = NULL;
	hg_path_node_t *on, *nn;
	hg_quark_t retval;
	GError *err = NULL;

	hg_return_val_if_fail (object->type == HG_TYPE_PATH, Qnil);

	if (object->on_copying != Qnil)
		return object->on_copying;

	object->on_copying = retval = hg_path_new(path->o.mem, (gpointer *)&p);
	if (retval != Qnil) {
		p->length = path->length;
		on = HG_MEM_LOCK (path->o.mem, path->qnode, &err);
		nn = HG_MEM_LOCK (path->o.mem, p->qnode, &err);
		if (on == NULL ||
		    nn == NULL) {
			goto finalize;
		}
		memcpy(nn, on, sizeof (hg_path_node_t) * path->length);
		hg_mem_unlock_object(path->o.mem, path->qnode);
		hg_mem_unlock_object(path->o.mem, p->qnode);

		if (ret)
			*ret = p;
		else
			hg_mem_unlock_object(path->o.mem, retval);
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
		hg_object_free(p->o.mem, retval);
		retval = Qnil;
	}
	
	return retval;
}

static gchar *
_hg_object_path_to_cstr(hg_object_t              *object,
			hg_quark_iterate_func_t   func,
			gpointer                  user_data,
			GError                  **error)
{
	return NULL;
}

static gboolean
_hg_object_path_gc_mark(hg_object_t           *object,
			hg_gc_iterate_func_t   func,
			gpointer               user_data,
			GError               **error)
{
	hg_path_t *path = (hg_path_t *)object;
	GError *err = NULL;
	gboolean retval = TRUE;

	hg_return_val_if_fail (object->type == HG_TYPE_PATH, FALSE);

#if defined(HG_DEBUG) && defined(HG_GC_DEBUG)
	g_print("GC: (path) marking node\n");
#endif
	if (!hg_mem_gc_mark(path->o.mem, path->qnode, &err))
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
_hg_object_path_compare(hg_object_t             *o1,
			hg_object_t             *o2,
			hg_quark_compare_func_t  func,
			gpointer                 user_data)
{
	return FALSE;
}

static gboolean
_hg_path_add(hg_path_t      *path,
	     hg_path_type_t  type,
	     gdouble         x,
	     gdouble         y)
{
	hg_path_node_t *node;
	gdouble cx, cy;
	gboolean retval = TRUE;

	hg_return_val_if_fail (path != NULL, FALSE);
	hg_return_val_if_fail (type < HG_PATH_END, FALSE);
	hg_return_val_if_fail (path->length < HG_PATH_MAX, FALSE);

	hg_return_val_if_lock_fail (node,
				    path->o.mem,
				    path->qnode,
				    FALSE);

	if (path->length == 0) {
		switch (type) {
		    case HG_PATH_CURVETO:
		    case HG_PATH_LINETO:
		    case HG_PATH_RCURVETO:
		    case HG_PATH_RMOVETO:
		    case HG_PATH_RLINETO:
			    /* /nocurrentpoint would be raised. */
			    retval = FALSE;
			    goto finalize;
		    case HG_PATH_CLOSEPATH:
			    /* no errors */
			    goto finalize;
		    default:
			    break;
		}
	}

	cx = node[path->length - 1].cx;
	cy = node[path->length - 1].cy;

	switch (type) {
	    case HG_PATH_SETBBOX:
		    break;
	    case HG_PATH_CURVETO:
	    case HG_PATH_LINETO:
	    case HG_PATH_MOVETO:
		    node[path->length].dx = x;
		    node[path->length].dy = y;
		    node[path->length].cx = x;
		    node[path->length].cy = y;
		    break;
	    case HG_PATH_RCURVETO:
	    case HG_PATH_RMOVETO:
	    case HG_PATH_RLINETO:
		    node[path->length].dx = x;
		    node[path->length].dy = y;
		    node[path->length].cx = cx + x;
		    node[path->length].cy = cy + y;
		    break;
	    case HG_PATH_CLOSEPATH:
		    if (node[path->length - 1].type == HG_PATH_CLOSEPATH) {
			    /* nothing to do */
			    goto finalize;
		    }
		    node[path->length].cx = node[0].dx;
		    node[path->length].cy = node[0].dy;
		    break;
	    case HG_PATH_UCACHE:
		    break;
	    default:
		    g_warning("%s: Unknown path type: %d",
			      __PRETTY_FUNCTION__, type);
		    retval = FALSE;
		    goto finalize;
	}

	node[path->length].type = type;
	path->length++;
  finalize:
	hg_mem_unlock_object(path->o.mem, path->qnode);

	return retval;
}

/*< public >*/
/**
 * hg_path_new:
 * @mem:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_path_new(hg_mem_t *mem,
	    gpointer *ret)
{
	hg_quark_t retval;
	hg_path_t *path = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil);

	retval = hg_object_new(mem, (gpointer *)&path, HG_TYPE_PATH, 0);

	if (ret)
		*ret = path;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_path_get_current_point:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_path_get_current_point(hg_path_t *path,
			  gdouble   *x,
			  gdouble   *y)
{
	hg_path_node_t *node;

	hg_return_val_if_fail (path != NULL, FALSE);

	if (path->length == 0)
		return FALSE;

	hg_return_val_if_lock_fail (node,
				    path->o.mem,
				    path->qnode,
				    FALSE);

	if (x)
		*x = node[path->length - 1].cx;
	if (y)
		*y = node[path->length - 1].cy;

	hg_mem_unlock_object(path->o.mem, path->qnode);

	return TRUE;
}

/**
 * hg_path_reverse:
 * @path:
 *
 * FIXME
 *
 * Returns:
 */
#if 0
hg_quark_t
hg_path_reverse(hg_path_t *path,
		gpointer  *ret)
{
	hg_quark_t retval;
	hg_path_t *new_path;
	hg_path_node_t *node, *new_node;
	gboolean closed = FALSE;

	hg_return_val_if_fail (path != NULL, Qnil);
	hg_return_val_if_fail (path->length > 0, Qnil);

	retval = hg_path_new(path->o.mem, (gpointer *)&new_path);
	if (retval != Qnil) {
		gssize i;

		node = HG_MEM_LOCK (path->o.mem, path->qnode, NULL);
		new_node = HG_MEM_LOCK (new_path->o.mem, new_path->qnode, NULL);
		if (node == NULL || new_node == NULL) {
			retval = Qnil;
			goto finalize;
		}

		if (node[path->length - 1].type == HG_PATH_CLOSEPATH)
			closed = TRUE;

		i = path->length - 1;
		new_node[new_path->length].type = HG_PATH_MOVETO;
		new_node[new_path->length].dx = node[i].cx;
		new_node[new_path->length].dy = node[i].cy;
		new_node[new_path->length].cx = node[i].cx;
		new_node[new_path->length].cy = node[i].cy;
		new_path->length++;

		for (; i >= 0; i++) {
			new_node[new_path->length].type = node[i].type;

			switch (node[i].type) {
			    case HG_PATH_MOVETO:
			    case HG_PATH_LINETO:
				    new_node[new_path->length].dx = node[i - 1].cx;
				    new_node[new_path->length].dy = node[i - 1].cy;
				    new_node[new_path->length].cx = node[i - 1].cx;
				    new_node[new_path->length].cy = node[i - 1].cy;
				    break;
			    case HG_PATH_RMOVETO:
			    case HG_PATH_RLINETO:
				    new_node[new_path->length].dx = -node[i].dx;
				    new_node[new_path->length].dy = -node[i].dy;
				    new_node[new_path->length].cx = node[i].cx - node[i].dx;
				    new_node[new_path->length].cy = node[i].cx - node[i].dy;
				    break;
			    case HG_PATH_CURVETO:
		}

		if (ret) {
			*ret = new_path;
		} else {
		  finalize:
			hg_mem_unlock_object(path->o.mem, retval);
		}
		hg_mem_unlock_object(path->o.mem, path->qnode);
		hg_mem_unlock_object(new_path->o.mem, new_path->qnode);
	}

	return retval;
}
#endif

/**
 * hg_path_close:
 * @path:
 *
 * FIXME
 */
gboolean
hg_path_close(hg_path_t *path)
{
	return _hg_path_add(path, HG_PATH_CLOSEPATH, 0.0, 0.0);
}

/**
 * hg_path_moveto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_path_moveto(hg_path_t *path,
	       gdouble    x,
	       gdouble    y)
{
	return _hg_path_add(path, HG_PATH_MOVETO, x, y);
}

/**
 * hg_path_rmoveto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_path_rmoveto(hg_path_t *path,
		gdouble    x,
		gdouble    y)
{
	gboolean retval = hg_path_get_current_point(path, NULL, NULL);

	if (retval)
		return _hg_path_add(path, HG_PATH_RMOVETO, x, y);

	return retval;
}

/**
 * hg_path_lineto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_path_lineto(hg_path_t *path,
	       gdouble    x,
	       gdouble    y)
{
	gboolean retval = hg_path_get_current_point(path, NULL, NULL);

	if (retval)
		return _hg_path_add(path, HG_PATH_LINETO, x, y);

	return retval;
}

/**
 * hg_path_rlineto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_path_rlineto(hg_path_t *path,
		gdouble    x,
		gdouble    y)
{
	gboolean retval = hg_path_get_current_point(path, NULL, NULL);

	if (retval)
		return _hg_path_add(path, HG_PATH_RLINETO, x, y);

	return retval;
}

/**
 * hg_path_curveto:
 * @path:
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
gboolean
hg_path_curveto(hg_path_t *path,
		gdouble    x1,
		gdouble    y1,
		gdouble    x2,
		gdouble    y2,
		gdouble    x3,
		gdouble    y3)
{
	gboolean retval = !hg_path_get_current_point(path, NULL, NULL);

	if (!retval) {
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, x1, y1);
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, x2, y2);
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, x3, y3);
	}

	return !retval;
}

/**
 * hg_path_rcurveto:
 * @path:
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
gboolean
hg_path_rcurveto(hg_path_t *path,
		 gdouble    x1,
		 gdouble    y1,
		 gdouble    x2,
		 gdouble    y2,
		 gdouble    x3,
		 gdouble    y3)
{
	gboolean retval = !hg_path_get_current_point(path, NULL, NULL);

	if (!retval) {
		retval |= !_hg_path_add(path, HG_PATH_RCURVETO, x1, y1);
		retval |= !_hg_path_add(path, HG_PATH_RCURVETO, x2, y2);
		retval |= !_hg_path_add(path, HG_PATH_RCURVETO, x3, y3);
	}

	return !retval;
}

/**
 * hg_path_arc:
 * @path:
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
gboolean
hg_path_arc(hg_path_t *path,
	    gdouble    x,
	    gdouble    y,
	    gdouble    r,
	    gdouble    angle1,
	    gdouble    angle2)
{
	gdouble r_sin_a1, r_cos_a1;
	gdouble r_sin_a2, r_cos_a2;
	gdouble h;
	gboolean retval = hg_path_get_current_point(path, NULL, NULL);

	if (angle1 < 0.0)
		angle1 = 2 * G_PI - angle1;
	if (angle2 < 0.0)
		angle2 = 2 * G_PI - angle2;
	while (angle2 < angle1)
		angle2 += 2 * G_PI;

	r_sin_a1 = r * sin(angle1);
	r_cos_a1 = r * cos(angle1);
	r_sin_a2 = r * sin(angle2);
	r_cos_a2 = r * cos(angle2);
	h = 4.0 / 3.0 * tan((angle2 - angle1) / 4.0);

	if (retval) {
		retval = !_hg_path_add(path, HG_PATH_LINETO, r_cos_a1 + x, r_sin_a1 + y);
	} else {
		retval = !_hg_path_add(path, HG_PATH_MOVETO, r_cos_a1 + x, r_sin_a1 + y);
	}

	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a1 + x - h * r_sin_a1, r_sin_a1 + y + h * r_cos_a1);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x + h * r_sin_a2, r_sin_a2 + y - h * r_cos_a2);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x, r_sin_a2 + y);

	return !retval;
}

/**
 * hg_path_arcn:
 * @path:
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
gboolean
hg_path_arcn(hg_path_t *path,
	     gdouble    x,
	     gdouble    y,
	     gdouble    r,
	     gdouble    angle1,
	     gdouble    angle2)
{
	gdouble r_sin_a1, r_cos_a1;
	gdouble r_sin_a2, r_cos_a2;
	gdouble h;
	gboolean retval = hg_path_get_current_point(path, NULL, NULL);

	if (angle1 < 0.0)
		angle1 = 2 * G_PI - angle1;
	if (angle2 < 0.0)
		angle2 = 2 * G_PI - angle2;
	while (angle2 > angle1)
		angle2 -= 2 * G_PI;

	r_sin_a1 = r * sin(angle1);
	r_cos_a1 = r * cos(angle1);
	r_sin_a2 = r * sin(angle2);
	r_cos_a2 = r * cos(angle2);
	h = 4.0 / 3.0 * tan((angle1 - angle2) / 4.0);

	if (retval) {
		retval = !_hg_path_add(path, HG_PATH_LINETO, r_cos_a1 + x, r_sin_a1 + y);
	} else {
		retval = !_hg_path_add(path, HG_PATH_MOVETO, r_cos_a1 + x, r_sin_a1 + y);
	}

	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a1 + x - h * r_sin_a1, r_sin_a1 + y + h * r_cos_a1);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x + h * r_sin_a2, r_sin_a2 + y - h * r_cos_a2);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x, r_sin_a2 + y);

	return !retval;
}

/**
 * hg_path_arcto:
 * @path:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @r:
 * @tp:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_path_arcto(hg_path_t *path,
	      gdouble    x1,
	      gdouble    y1,
	      gdouble    x2,
	      gdouble    y2,
	      gdouble    r,
	      gdouble    tp[4])
{
	gdouble x0 = 0.0, y0 = 0.0;
	gboolean retval = !hg_path_get_current_point(path, &x0, &y0);

	if (!retval) {
		/* calculate the tangent points */
		/* a = <x0,y0>, b = <x1,y1>, c = <x2,y2> */
		gdouble d_ab_x = x0 - x1, d_ab_y = y0 - y1;
		gdouble d_bc_x = x2 - x1, d_bc_y = y2 - y1;
		gdouble _ab_ = sqrt(d_ab_x * d_ab_x + d_ab_y * d_ab_y);
		gdouble _bc_ = sqrt(d_bc_x * d_bc_x + d_bc_y * d_bc_y);
		gdouble a_o_c = d_ab_x * d_bc_x + d_ab_y * d_bc_y;
		gdouble a_x_c = d_ab_y * d_bc_x - d_bc_y * d_ab_x;
		gdouble common = r * (_ab_ * _bc_ + a_o_c);
		gdouble _t1b_ = common * _bc_ / (_ab_ * a_x_c);
		gdouble _t2b_ = common * _ab_ / (_bc_ * a_x_c);
		gdouble ca = fabs(_t1b_ / _bc_);
		gdouble cc = fabs(_t2b_ / _ab_);
		gdouble t1x = d_ab_x * ca + x1;
		gdouble t1y = d_ab_y * ca + y1;
		gdouble t2x = d_bc_x * cc + x1;
		gdouble t2y = d_bc_y * cc + y1;

		if (tp) {
			tp[0] = t1x;
			tp[1] = t1y;
			tp[2] = t2x;
			tp[3] = t2y;
		}

		if (HG_REAL_IS_ZERO (t1y * x0 - y0 * t1x)) {
			/* points to the same place */
		} else {
			retval = !_hg_path_add(path, HG_PATH_LINETO, t1x, t1y);
		}
		/* XXX: two the control points */
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, t2x, t2y);
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, t2x, t2y);
		/* XXX: end */
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, t2x, t2y);
	}

	return !retval;
}
