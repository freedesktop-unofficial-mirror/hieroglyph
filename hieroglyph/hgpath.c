/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpath.c
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
#include <string.h>
#include "hgpath.h"
#include "hgmem.h"


static void     _hg_path_node_real_set_flags(gpointer           data,
					     guint              flags);
static void     _hg_path_node_real_relocate (gpointer           data,
					     HgMemRelocateInfo *info);
static gpointer _hg_path_node_real_copy     (gpointer           data);
static void     _hg_path_real_free          (gpointer           data);
static void     _hg_path_real_set_flags     (gpointer           data,
					     guint              flags);
static void     _hg_path_real_relocate      (gpointer           data,
					     HgMemRelocateInfo *info);
static gpointer _hg_path_real_copy          (gpointer           data);


static HgObjectVTable __hg_path_vtable = {
	.free      = _hg_path_real_free,
	.set_flags = _hg_path_real_set_flags,
	.relocate  = _hg_path_real_relocate,
	.dup       = NULL,
	.copy      = _hg_path_real_copy,
	.to_string = NULL,
};
static HgObjectVTable __hg_path_node_vtable = {
	.free      = NULL,
	.set_flags = _hg_path_node_real_set_flags,
	.relocate  = _hg_path_node_real_relocate,
	.dup       = NULL,
	.copy      = _hg_path_node_real_copy,
	.to_string = NULL,
};

/*
 * Private Functions
 */
static void
_hg_path_node_real_set_flags(gpointer data,
			     guint    flags)
{
	HgPathNode *node = data;
	HgMemObject *obj;

	if (node->next) {
		hg_mem_get_object__inline(node->next, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: HgPathNode", node->next);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_path_node_real_relocate(gpointer           data,
			    HgMemRelocateInfo *info)
{
	HgPathNode *node = data;

	if ((gsize)node->next >= info->start &&
	    (gsize)node->next <= info->end) {
		node->next = (gpointer)((gsize)node->next + info->diff);
	}
}

static gpointer
_hg_path_node_real_copy(gpointer data)
{
	HgPathNode *node = data;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;

	return hg_path_node_new(obj->pool, node->type, node->x, node->y);
}

static void
_hg_path_real_free(gpointer data)
{
	HgPath *path = data;

	if (path->node)
		hg_path_node_free(path->node);
}

static void
_hg_path_real_set_flags(gpointer data,
			guint    flags)
{
	HgPath *path = data;
	HgMemObject *obj;

	if (path->node) {
		hg_mem_get_object__inline(path->node, obj);
		if (obj == NULL) {
			g_warning("[BUG] Invalid object %p to be marked: HgPathNode (in HgPath)", path->node);
		} else {
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_path_real_relocate(gpointer           data,
		       HgMemRelocateInfo *info)
{
	HgPath *path = data;

	if ((gsize)path->node >= info->start &&
	    (gsize)path->node <= info->end) {
		path->node = (gpointer)((gsize)path->node + info->diff);
	}
	if ((gsize)path->last_node >= info->start &&
	    (gsize)path->last_node <= info->end) {
		path->last_node = (gpointer)((gsize)path->last_node + info->diff);
	}
}

static gpointer
_hg_path_real_copy(gpointer data)
{
	HgPath *path = data, *retval;
	HgMemObject *obj;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;

	retval = hg_path_new(obj->pool);
	if (retval == NULL)
		return NULL;

	if (!hg_path_copy(path, retval))
		return NULL;

	return retval;
}

static HgPathNode *
_hg_path_node_last(HgPathNode *node)
{
	if (node) {
		while (node->next)
			node = node->next;
	}

	return node;
}

static HgPathNode *
_hg_path_node_real_path_last(HgPathNode *node)
{
	HgPathNode *last = _hg_path_node_last(node);

	if (last) {
		while (last->type == HG_PATH_MATRIX)
			last = last->prev;
	}

	return last;
}

static gboolean
_hg_path_node_find(HgPathNode *node,
		   HgPathType  type)
{
	gboolean retval = FALSE;

	while (node) {
		if (node->type == type)
			return TRUE;
		node = node->next;
	}

	return retval;
}

static void
_hg_path_append_node(HgPath     *path,
		     HgPathNode *node)
{
	if (path->last_node == NULL) {
		path->last_node = path->node = node;
		path->last_node->prev = NULL;
		path->last_node->next = NULL;
	} else {
		path->last_node->next = node;
		node->prev = path->last_node;
		path->last_node = _hg_path_node_last(node);
		path->last_node->next = NULL;
	}
}

/*
 * Public Functions
 */
HgPathNode *
hg_path_node_new(HgMemPool  *pool,
		 HgPathType  type,
		 gdouble     x,
		 gdouble     y)
{
	HgPathNode *node;

	/* HgPathNode object may goes into somewhere out of the memory management.
	 * so it may be a good idea to lock this until free it apparently.
	 */
	node = hg_mem_alloc_with_flags(pool,
				       sizeof (HgPathNode),
				       HG_FL_RESTORABLE | HG_FL_LOCK);
	if (node == NULL)
		return NULL;

	node->object.id = HG_OBJECT_ID;
	node->object.state = hg_mem_pool_get_default_access_mode(pool);
	node->object.vtable = &__hg_path_node_vtable;

	node->type = type;
	node->x = x;
	node->y = y;
	node->prev = NULL;
	node->next = NULL;

	return node;
}

void
hg_path_node_free(HgPathNode *node)
{
	HgMemObject *obj;

	while (node) {
		hg_mem_get_object__inline(node, obj);
		hg_mem_set_unlock(obj);
		node = node->next;
	}
}

HgPath *
hg_path_new(HgMemPool *pool)
{
	HgPath *retval;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc(pool, sizeof (HgPath));
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	retval->object.vtable = &__hg_path_vtable;

	retval->node = NULL;
	retval->last_node = NULL;

	return retval;
}

gboolean
hg_path_copy(HgPath *src, HgPath *dest)
{
	HgPathNode *node, *n;

	g_return_val_if_fail (src != NULL, FALSE);
	g_return_val_if_fail (dest != NULL, FALSE);

	hg_path_clear(dest, TRUE);
	for (n = src->node; n != NULL; n = n->next) {
		node = hg_object_copy((HgObject *)n);
		if (node == NULL)
			return FALSE;
		_hg_path_append_node(dest, node);
	}

	return TRUE;
}

gboolean
hg_path_find(HgPath     *path,
	     HgPathType  type)
{
	g_return_val_if_fail (path != NULL, FALSE);

	return _hg_path_node_find(path->node, type);
}

gboolean
hg_path_compute_current_point(HgPath  *path,
			      gdouble *x,
			      gdouble *y)
{
	HgPathNode *node;
	gboolean currentpoint = FALSE;

	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (x != NULL, FALSE);
	g_return_val_if_fail (y != NULL, FALSE);

	for (node = path->node; node != NULL; node = node->next) {
		switch (node->type) {
		    case HG_PATH_CURVETO:
			    if (!currentpoint) {
				    g_warning("[BUG] no current point found in %d.", node->type);
				    return FALSE;
			    }
			    if (node->next == NULL ||
				node->next->next == NULL) {
				    g_warning("[BUG] Invalid path found in %d.", node->type);
				    return FALSE;
			    }
			    node = node->next->next;
		    case HG_PATH_LINETO:
			    if (!currentpoint) {
				    g_warning("[BUG] no current point found in %d.", node->type);
				    return FALSE;
			    }
		    case HG_PATH_MOVETO:
			    *x = node->x;
			    *y = node->y;
			    currentpoint = TRUE;
			    break;
		    case HG_PATH_RCURVETO:
			    if (node->next == NULL ||
				node->next->next == NULL) {
				    g_warning("[BUG] Invalid path found in %d.", node->type);
				    return FALSE;
			    }
			    node = node->next->next;
		    case HG_PATH_RMOVETO:
		    case HG_PATH_RLINETO:
			    if (!currentpoint) {
				    g_warning("[BUG] no current point found in %d.", node->type);
				    return FALSE;
			    }
			    *x = *x + node->x;
			    *y = *y + node->y;
			    break;
		    case HG_PATH_ARC:
			    if (node->next == NULL ||
				node->next->next == NULL) {
				    g_warning("[BUG] Invalid path found in %d.\n", node->type);
				    return FALSE;
			    }
			    G_STMT_START {
				    gdouble dbx, dby, dr, dangle2;

				    dbx = node->x;
				    dby = node->y;
				    dr = node->next->x;
				    dangle2 = node->next->next->y;
				    *x = cos(dangle2) * dr + dbx;
				    *y = sin(dangle2) * dr + dby;
				    node = node->next->next;
				    currentpoint = TRUE;
			    } G_STMT_END;
			    break;
		    case HG_PATH_CLOSE:
		    case HG_PATH_MATRIX:
			    break;
		    default:
			    g_warning("[BUG] Unknown path %d was given.", node->type);
			    return FALSE;
		}
	}

	return currentpoint;
}

gboolean
hg_path_get_bbox(HgPath     *path,
		 gboolean    ignore_moveto,
		 HgPathBBox *bbox)
{
	HgPathNode *node, *last = NULL;
	HgPathBBox prev;
	gboolean flag = FALSE, currentpoint = FALSE;
	gdouble dx = 0, dy = 0;

	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (bbox != NULL, FALSE);

	for (node = path->node; node != NULL; node = node->next) {
		last = node;
		switch (node->type) {
		    case HG_PATH_LINETO:
			    flag = TRUE;
			    if (!currentpoint)
				    return FALSE;
		    case HG_PATH_MOVETO:
			    if (node == path->node) {
				    /* set an initial bbox */
				    bbox->llx = node->x;
				    bbox->lly = node->y;
			    } else {
				    if (node->x < bbox->llx)
					    bbox->llx = node->x;
				    if (node->y < bbox->lly)
					    bbox->lly = node->y;
			    }
			    if (node->x > bbox->urx)
				    bbox->urx = node->x;
			    if (node->y > bbox->ury)
				    bbox->ury = node->y;
			    memcpy(&prev, bbox, sizeof (HgPathBBox));
			    dx = node->x;
			    dy = node->y;
			    currentpoint = TRUE;
			    break;
		    case HG_PATH_RLINETO:
			    flag = TRUE;
		    case HG_PATH_RMOVETO:
			    if (!currentpoint)
				    return FALSE;
			    if ((dx + node->x) < bbox->llx)
				    bbox->llx = node->x + dx;
			    if ((dy + node->y) < bbox->lly)
				    bbox->lly = node->y + dy;
			    if ((dx + node->x) > bbox->urx)
				    bbox->urx = node->x + dx;
			    if ((dy + node->y) > bbox->ury)
				    bbox->ury = node->y + dy;
			    memcpy(&prev, bbox, sizeof (HgPathBBox));
			    dx += node->x;
			    dy += node->y;
			    break;
		    case HG_PATH_CURVETO:
			    if (!currentpoint)
				    return FALSE;
			    if (node->x < bbox->llx)
				    bbox->llx = node->x;
			    if (node->y < bbox->lly)
				    bbox->lly = node->y;
			    if (node->x < bbox->urx)
				    bbox->urx = node->x;
			    if (node->y < bbox->ury)
				    bbox->ury = node->y;
			    memcpy(&prev, bbox, sizeof (HgPathBBox));
			    flag = TRUE;
			    dx = node->x;
			    dy = node->y;
			    currentpoint = TRUE;
			    break;
		    case HG_PATH_RCURVETO:
			    if (!currentpoint)
				    return FALSE;
			    if ((dx + node->x) < bbox->llx)
				    bbox->llx = node->x + dx;
			    if ((dy + node->y) < bbox->lly)
				    bbox->lly = node->y + dy;
			    if ((dx + node->x) < bbox->urx)
				    bbox->urx = node->x + dx;
			    if ((dy + node->y) < bbox->ury)
				    bbox->ury = node->y + dy;
			    memcpy(&prev, bbox, sizeof (HgPathBBox));
			    dx += node->x;
			    dy += node->y;
			    flag = TRUE;
			    break;
		    case HG_PATH_CLOSE:
			    flag = TRUE;
			    break;
		    case HG_PATH_ARC:
			    flag = TRUE;
			    if (node->next == NULL || node->next->next == NULL) {
				    g_warning("[BUG] Invalid path for arc.");
				    break;
			    }
			    G_STMT_START {
				    gdouble dx1, dy1, dx2, dy2, dbx, dby, dr, dangle1, dangle2;

				    dbx = node->x;
				    dby = node->y;
				    dr = node->next->x;
				    dangle1 = node->next->next->x;
				    dangle2 = node->next->next->y;
				    dx1 = cos(dangle1) * dr + dbx;
				    dy1 = sin(dangle1) * dr + dby;
				    dx2 = cos(dangle2) * dr + dbx;
				    dy2 = sin(dangle2) * dr + dby;
				    if (node == path->node) {
					    /* set an initial bbox */
					    bbox->llx = dx1;
					    bbox->lly = dy1;
				    } else {
					    if (dx1 < bbox->llx)
						    bbox->llx = dx1;
					    if (dy1 < bbox->lly)
						    bbox->lly = dy1;
				    }
				    if (dx1 > bbox->urx)
					    bbox->urx = dx1;
				    if (dy1 > bbox->ury)
					    bbox->ury = dy1;
				    if (dx2 < bbox->llx)
					    bbox->llx = dx2;
				    if (dy2 < bbox->llx)
					    bbox->lly = dx2;
				    if (dx2 > bbox->urx)
					    bbox->urx = dx2;
				    if (dy2 > bbox->ury)
					    bbox->ury = dy2;
				    memcpy(&prev, bbox, sizeof (HgPathBBox));
				    dx = dx2;
				    dy = dy2;
				    currentpoint = TRUE;
				    node = node->next->next;
			    } G_STMT_END;
			    break;
		    case HG_PATH_MATRIX:
			    /* this isn't used to calculate bbox at all */
			    break;
		    default:
			    g_warning("[BUG] Unknown path type %d to examine current bbox.", node->type);
			    return FALSE;
		}
	}
	if (ignore_moveto &&
	    last != NULL &&
	    (last->type == HG_PATH_MOVETO || last->type == HG_PATH_RMOVETO) &&
	    flag) {
		memcpy(bbox, &prev, sizeof (HgPathBBox));
	}

	return currentpoint;
}

gboolean
hg_path_clear(HgPath   *path,
	      gboolean  free_segment)
{
	g_return_val_if_fail (path != NULL, FALSE);

	if (free_segment && path->node)
		hg_path_node_free(path->node);

	path->node = NULL;
	path->last_node = NULL;

	return TRUE;
}

gboolean
hg_path_close(HgPath *path)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;
	if (path->last_node == NULL) {
		/* nothing to do */
		return TRUE;
	} else {
		HgPathNode *last = _hg_path_node_real_path_last(path->last_node);

		if (last->type == HG_PATH_CLOSE) {
			/* nothing to do */
			return TRUE;
		}
	}
	node = hg_path_node_new(obj->pool, HG_PATH_CLOSE, 0, 0);
	if (node == NULL)
		return FALSE;

	_hg_path_append_node(path, node);

	return TRUE;
}

gboolean
hg_path_moveto(HgPath  *path,
	       gdouble  x,
	       gdouble  y)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;
	node = hg_path_node_new(obj->pool, HG_PATH_MOVETO, x, y);
	if (node == NULL)
		return FALSE;

	_hg_path_append_node(path, node);

	return TRUE;
}

gboolean
hg_path_lineto(HgPath  *path,
	       gdouble  x,
	       gdouble  y)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;
	node = hg_path_node_new(obj->pool, HG_PATH_LINETO, x, y);
	if (node == NULL)
		return FALSE;

	_hg_path_append_node(path, node);

	return TRUE;
}

gboolean
hg_path_rlineto(HgPath  *path,
		gdouble  x,
		gdouble  y)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;
	node = hg_path_node_new(obj->pool, HG_PATH_RLINETO, x, y);
	if (node == NULL)
		return FALSE;

	_hg_path_append_node(path, node);

	return TRUE;
}

gboolean
hg_path_curveto(HgPath  *path,
		gdouble  x1,
		gdouble  y1,
		gdouble  x2,
		gdouble  y2,
		gdouble  x3,
		gdouble  y3)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;

	/* first path */
	node = hg_path_node_new(obj->pool, HG_PATH_CURVETO, x1, y1);
	if (node == NULL)
		return FALSE;
	_hg_path_append_node(path, node);

	/* second path */
	node = hg_path_node_new(obj->pool, HG_PATH_CURVETO, x2, y2);
	if (node == NULL)
		return FALSE;
	_hg_path_append_node(path, node);

	/* third path */
	node = hg_path_node_new(obj->pool, HG_PATH_CURVETO, x3, y3);
	if (node == NULL)
		return FALSE;
	_hg_path_append_node(path, node);

	return TRUE;
}

gboolean
hg_path_arc(HgPath  *path,
	    gdouble  x,
	    gdouble  y,
	    gdouble  r,
	    gdouble  angle1,
	    gdouble  angle2)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;

	node = hg_path_node_new(obj->pool, HG_PATH_ARC, x, y);
	if (node == NULL)
		return FALSE;
	_hg_path_append_node(path, node);
	node = hg_path_node_new(obj->pool, HG_PATH_ARC, r, 0);
	if (node == NULL)
		return FALSE;
	_hg_path_append_node(path, node);
	node = hg_path_node_new(obj->pool, HG_PATH_ARC, angle1, angle2);
	if (node == NULL)
		return FALSE;
	_hg_path_append_node(path, node);

	return TRUE;
}

gboolean
hg_path_matrix(HgPath  *path,
	       gdouble  xx,
	       gdouble  yx,
	       gdouble  xy,
	       gdouble  yy,
	       gdouble  x0,
	       gdouble  y0)
{
	HgPathNode *node;
	HgMemObject *obj;

	g_return_val_if_fail (path != NULL, FALSE);

	hg_mem_get_object__inline(path, obj);
	if (obj == NULL)
		return FALSE;

	if (path->last_node &&
	    path->last_node->type == HG_PATH_MATRIX) {
		/* it could be optimized */
		path->last_node->prev->prev->x = xx;
		path->last_node->prev->prev->y = yx;
		path->last_node->prev->x = xy;
		path->last_node->prev->y = yy;
		path->last_node->x = x0;
		path->last_node->y = y0;
	} else {
		node = hg_path_node_new(obj->pool, HG_PATH_MATRIX, xx, yx);
		if (node == NULL)
			return FALSE;
		_hg_path_append_node(path, node);
		node = hg_path_node_new(obj->pool, HG_PATH_MATRIX, xy, yy);
		if (node == NULL)
			return FALSE;
		_hg_path_append_node(path, node);
		node = hg_path_node_new(obj->pool, HG_PATH_MATRIX, x0, y0);
		if (node == NULL)
			return FALSE;
		_hg_path_append_node(path, node);
	}

	return TRUE;
}
