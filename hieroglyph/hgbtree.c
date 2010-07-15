/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbtree.c
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

#include <string.h>
#include "hgerror.h"
#include "hgbtree-private.h"
#include "hgbtree.h"


G_INLINE_FUNC hg_quark_t _hg_btree_node_new                  (hg_mem_t                 *mem,
                                                              hg_quark_t                qtree,
                                                              gpointer                 *ret,
                                                              gpointer                 *ret_key,
                                                              gpointer                 *ret_val,
                                                              gpointer                 *ret_nodes);
G_INLINE_FUNC void       _hg_btree_node_free                 (hg_mem_t                 *mem,
                                                              hg_quark_t                qnode);
G_INLINE_FUNC gboolean   _hg_btree_node_insert               (hg_mem_t                 *mem,
                                                              hg_quark_t                qnode,
                                                              hg_quark_t               *qkey,
                                                              hg_quark_t               *qval,
                                                              hg_quark_t               *qright_node,
                                                              GError                   **error);
G_INLINE_FUNC void       _hg_btree_node_insert_data          (hg_btree_node_t          *node,
                                                              hg_quark_t               *qkeys,
                                                              hg_quark_t               *qvals,
                                                              hg_quark_t               *qnodes,
                                                              hg_quark_t                key,
                                                              hg_quark_t                val,
                                                              gsize                     pos,
                                                              hg_quark_t               *qright_node);
G_INLINE_FUNC void       _hg_btree_node_balance              (hg_btree_node_t          *node,
                                                              hg_btree_t               *tree,
                                                              hg_quark_t               *qnode_keys,
                                                              hg_quark_t               *qnode_vals,
                                                              hg_quark_t               *qnode_nodes,
                                                              hg_quark_t               *qkey,
                                                              hg_quark_t               *qval,
                                                              gsize                     pos,
                                                              hg_quark_t               *qright_node,
                                                              GError                   **error);
G_INLINE_FUNC gboolean   _hg_btree_node_remove               (hg_mem_t                 *mem,
                                                              hg_quark_t                qnode,
                                                              hg_quark_t               *qkey,
                                                              hg_quark_t               *qval,
                                                              gboolean                 *need_restore,
                                                              GError                   **error);
G_INLINE_FUNC gboolean   _hg_btree_node_remove_data          (hg_btree_node_t          *node,
                                                              hg_quark_t               *qkeys,
                                                              hg_quark_t               *qvals,
                                                              hg_quark_t               *qnodes,
                                                              gsize                     pos,
                                                              GError                   **error);
G_INLINE_FUNC gboolean   _hg_btree_node_restore              (hg_btree_node_t          *node,
                                                              hg_quark_t               *qkeys,
                                                              hg_quark_t               *qvals,
                                                              hg_quark_t               *qnodes,
                                                              gsize                     pos,
                                                              GError                   **error);
G_INLINE_FUNC void       _hg_btree_node_restore_right_balance(hg_btree_node_t          *node,
                                                              hg_quark_t               *qkeys,
                                                              hg_quark_t               *qvals,
                                                              hg_quark_t               *qnodes,
                                                              gsize                     pos,
                                                              GError                   **error);
G_INLINE_FUNC void       _hg_btree_node_restore_left_balance (hg_btree_node_t          *node,
                                                              hg_quark_t               *qkeys,
                                                              hg_quark_t               *qvals,
                                                              hg_quark_t               *qnodes,
                                                              gsize                     pos,
                                                              GError                   **error);
G_INLINE_FUNC gboolean   _hg_btree_node_combine              (hg_btree_node_t          *node,
                                                              hg_quark_t               *qkeys,
                                                              hg_quark_t               *qvals,
                                                              hg_quark_t               *qnodes,
                                                              gsize                     pos,
                                                              GError                   **error);
G_INLINE_FUNC void       _hg_btree_node_foreach              (hg_mem_t                 *mem,
                                                              hg_quark_t                qnode,
                                                              hg_btree_traverse_func_t  func,
                                                              gpointer                  data,
                                                              GError                   **error);


/*< private >*/
G_INLINE_FUNC hg_quark_t
_hg_btree_node_new(hg_mem_t   *mem,
		   hg_quark_t  qtree,
		   gpointer   *ret,
		   gpointer   *ret_key,
		   gpointer   *ret_val,
		   gpointer   *ret_nodes)
{
	hg_quark_t retval, *keys = NULL, *vals = NULL, *nodes = NULL;
	hg_btree_node_t *node;
	hg_btree_t *tree;

	hg_return_val_if_lock_fail (tree, mem, qtree, Qnil);

	retval = hg_mem_alloc(mem, sizeof (hg_btree_node_t), (gpointer *)&node);
	if (retval != Qnil) {
		memset(node, 0, sizeof (hg_btree_node_t));

		node->mem = mem;
		node->qparent = qtree;
		node->qkey = hg_mem_alloc(mem, sizeof (hg_quark_t) * tree->size * 2, (gpointer *)&keys);
		node->qval = hg_mem_alloc(mem, sizeof (hg_quark_t) * tree->size * 2, (gpointer *)&vals);
		node->qnodes = hg_mem_alloc(mem, sizeof (hg_quark_t) * tree->size * 2 + 1, (gpointer *)&nodes);
		node->n_data = 0;
		hg_return_val_after_eval_if_fail (node->qkey != Qnil &&
						  node->qval != Qnil &&
						  node->qnodes != Qnil,
						  Qnil, _hg_btree_node_free(mem, retval));

		if (ret)
			*ret = node;
		else
			hg_mem_unlock_object(mem, retval);
		if (ret_key)
			*ret_key = keys;
		else
			hg_mem_unlock_object(mem, node->qkey);
		if (ret_val)
			*ret_val = vals;
		else
			hg_mem_unlock_object(mem, node->qval);
		if (ret_nodes)
			*ret_nodes = nodes;
		else
			hg_mem_unlock_object(mem, node->qnodes);
	}
	hg_mem_unlock_object(mem, qtree);

	return retval;
}

G_INLINE_FUNC void
_hg_btree_node_free(hg_mem_t   *mem,
		    hg_quark_t  qnode)
{
	hg_btree_node_t *node;

	if (qnode == Qnil)
		return;

	hg_return_if_lock_fail (node, mem, qnode);

	hg_mem_free(mem, node->qkey);
	hg_mem_free(mem, node->qval);
	hg_mem_free(mem, node->qnodes);
	hg_mem_free(mem, qnode);
}

G_INLINE_FUNC gboolean
_hg_btree_node_insert(hg_mem_t    *mem,
		      hg_quark_t   qnode,
		      hg_quark_t  *qkey,
		      hg_quark_t  *qval,
		      hg_quark_t  *qright_node,
		      GError     **error)
{
	gsize i;
	gboolean retval = FALSE;
	hg_btree_node_t *qnode_node = NULL;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;
	hg_btree_t *tree = NULL;

	if (qnode == Qnil) {
		*qright_node = Qnil;
		return FALSE;
	}
	HG_BTREE_NODE_LOCK (mem, qnode, qnode, "", error);

	if (qnode_keys[qnode_node->n_data - 1] >= *qkey) {
		for (i = 0; i < qnode_node->n_data && qnode_keys[i] < *qkey; i++);
		if (i < qnode_node->n_data && qnode_keys[i] == *qkey) {
#if defined(HG_DEBUG) && defined(HG_BTREE_DEBUG)
			g_print("Inserting val %" G_GSIZE_FORMAT " with key %" G_GSIZE_FORMAT " at index %" G_GSIZE_FORMAT " on node %p\n", *qval, *qkey, i, qnode_nodes);
#endif
			qnode_keys[i] = *qkey;
			qnode_vals[i] = *qval;

			retval = TRUE;
			goto finalize;
		}
	} else {
		i = qnode_node->n_data;
	}
	retval = _hg_btree_node_insert(mem, qnode_nodes[i], qkey, qval, qright_node, error);
	if (*error) {
		retval = FALSE;
		goto finalize;
	}
	if (!retval) {
		tree = HG_MEM_LOCK (mem, qnode_node->qparent, error);
		if (tree == NULL) {
			retval = FALSE;
			goto finalize;
		}

		if (qnode_node->n_data < (tree->size * 2)) {
			_hg_btree_node_insert_data(qnode_node, qnode_keys, qnode_vals, qnode_nodes, *qkey, *qval, i, qright_node);
			retval = TRUE;
		} else {
			_hg_btree_node_balance(qnode_node, tree, qnode_keys, qnode_vals, qnode_nodes, qkey, qval, i, qright_node, error);
			retval = FALSE;
		}
	}
  finalize:
	if (tree)
		hg_mem_unlock_object(mem, qnode_node->qparent);

	HG_BTREE_NODE_UNLOCK (mem, qnode, qnode);

	return retval;
}

G_INLINE_FUNC void
_hg_btree_node_insert_data(hg_btree_node_t *node,
			   hg_quark_t      *qkeys,
			   hg_quark_t      *qvals,
			   hg_quark_t      *qnodes,
			   hg_quark_t       qkey,
			   hg_quark_t       qval,
			   gsize            pos,
			   hg_quark_t      *qright_node)
{
	gsize i;

	for (i = node->n_data; i > pos; i--) {
		qkeys[i] = qkeys[i - 1];
		qvals[i] = qvals[i - 1];
		qnodes[i + 1] = qnodes[i];
	}
#if defined(HG_DEBUG) && defined(HG_BTREE_DEBUG)
	g_print("Inserting val %" G_GSIZE_FORMAT " with key %" G_GSIZE_FORMAT " at index %" G_GSIZE_FORMAT " on node %p\n", qval, qkey, pos, qnodes);
#endif
	qkeys[pos] = qkey;
	qvals[pos] = qval;
	qnodes[pos + 1] = *qright_node;
	node->n_data++;
}

G_INLINE_FUNC void
_hg_btree_node_balance(hg_btree_node_t  *node,
		       hg_btree_t       *tree,
		       hg_quark_t       *qnode_keys,
		       hg_quark_t       *qnode_vals,
		       hg_quark_t       *qnode_nodes,
		       hg_quark_t       *qkey,
		       hg_quark_t       *qval,
		       gsize             pos,
		       hg_quark_t       *qright_node,
		       GError          **error)
{
	gsize i, node_size;
	hg_quark_t q, *qnewnode_keys, *qnewnode_vals, *qnewnode_nodes;
	hg_btree_node_t *new_node;

	if (pos <= tree->size)
		node_size = tree->size;
	else
		node_size = tree->size + 1;
	q = _hg_btree_node_new(node->mem, node->qparent,
			       (gpointer *)&new_node,
			       (gpointer *)&qnewnode_keys,
			       (gpointer *)&qnewnode_vals,
			       (gpointer *)&qnewnode_nodes);
	if (q == Qnil) {
		g_set_error(error, HG_ERROR, ENOMEM,
			    "%s: Out of memory.", __PRETTY_FUNCTION__);
		return;
	}
	for (i = node_size + 1; i <= tree->size * 2; i++) {
		qnewnode_keys[i - node_size - 1] = qnode_keys[i - 1];
		qnewnode_vals[i - node_size - 1] = qnode_vals[i - 1];
		qnewnode_nodes[i - node_size] = qnode_nodes[i];
	}
	new_node->n_data = tree->size * 2 - node_size;
	node->n_data = node_size;
	if (pos <= tree->size) {
		_hg_btree_node_insert_data(node,
					   qnode_keys,
					   qnode_vals,
					   qnode_nodes,
					   *qkey, *qval,
					   pos, qright_node);
	} else {
		_hg_btree_node_insert_data(new_node,
					   qnewnode_keys,
					   qnewnode_vals,
					   qnewnode_nodes,
					   *qkey, *qval,
					   pos - node_size, qright_node);
	}
	*qkey = qnode_keys[node->n_data - 1];
	*qval = qnode_vals[node->n_data - 1];
	qnewnode_nodes[0] = qnode_nodes[node->n_data];
	node->n_data--;
	*qright_node = q;

	hg_mem_unlock_object(node->mem, new_node->qkey);
	hg_mem_unlock_object(node->mem, new_node->qval);
	hg_mem_unlock_object(node->mem, new_node->qnodes);
	hg_mem_unlock_object(node->mem, q);
}

G_INLINE_FUNC gboolean
_hg_btree_node_remove(hg_mem_t    *mem,
		      hg_quark_t   qnode,
		      hg_quark_t  *qkey,
		      hg_quark_t  *qval,
		      gboolean    *need_restore,
		      GError     **error)
{
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL, q;
	hg_btree_node_t *qnode_node, *p;
	gsize i;
	gboolean retval = FALSE;

	if (qnode == Qnil)
		return FALSE;

	HG_BTREE_NODE_LOCK (mem, qnode, qnode, "", error);

	if (qnode_keys[qnode_node->n_data - 1] >= *qkey) {
		for (i = 0; i < qnode_node->n_data && qnode_keys[i] < *qkey; i++);
	} else {
		i = qnode_node->n_data;
	}
	if (i < qnode_node->n_data && qnode_keys[i] == *qkey) {
		retval = TRUE;
		if (qnode_nodes[i + 1] != Qnil) {
			hg_quark_t *qtkey = NULL, *qtval = NULL, *qtnode = NULL;
			hg_quark_t qqnode = Qnil;

			qtnode = &qnode_nodes[i + 1];
			q = Qnil;
			do {
				if (q != Qnil)
					hg_mem_unlock_object(qnode_node->mem, q);
				q = qtnode[0];
				if (qqnode != Qnil)
					hg_mem_unlock_object(qnode_node->mem, qqnode);
				p = hg_mem_lock_object(qnode_node->mem, q);
				if (p == NULL) {
					g_set_error(error, HG_ERROR, EINVAL,
						    "%s: Invalid quark to obtain the node in the nodes container object: mem: %p, quark: %" G_GSIZE_FORMAT " in the node %p(q: %" G_GSIZE_FORMAT ")",
						    __PRETTY_FUNCTION__, mem, q, qtnode, qqnode != Qnil ? qqnode : qnode_nodes[i + 1]);
					retval = FALSE;
					goto finalize;
				}
				qqnode = p->qnodes;
				qtnode = hg_mem_lock_object(qnode_node->mem, qqnode);
			} while (qtnode[0] != Qnil);

			qtkey = HG_MEM_LOCK (qnode_node->mem, p->qkey, error);
			if (qtkey == NULL) {
				retval = FALSE;
				goto finalize;
			}
			qtval = HG_MEM_LOCK (qnode_node->mem, p->qval, error);
			if (qtval == NULL) {
				retval = FALSE;
				goto finalize;
			}

			qnode_keys[i] = *qkey = qtkey[0];
			qnode_vals[i] = *qval = qtval[0];

			hg_mem_unlock_object(qnode_node->mem, p->qkey);
			hg_mem_unlock_object(qnode_node->mem, p->qval);
			hg_mem_unlock_object(qnode_node->mem, p->qnodes);
			hg_mem_unlock_object(qnode_node->mem, q);

			retval = _hg_btree_node_remove(qnode_node->mem, qnode_nodes[i + 1], qkey, qval, need_restore, error);
			if (*error) {
				retval = FALSE;
				goto finalize;
			}
			if (*need_restore)
				*need_restore = _hg_btree_node_restore(qnode_node, qnode_keys, qnode_vals, qnode_nodes, i + 1, error);
		} else {
			*need_restore = _hg_btree_node_remove_data(qnode_node, qnode_keys, qnode_vals, qnode_nodes, i, error);
			if (*error) {
				retval = FALSE;
				goto finalize;
			}
		}
	} else {
		retval = _hg_btree_node_remove(qnode_node->mem, qnode_nodes[i], qkey, qval, need_restore, error);
		if (*error) {
			retval = FALSE;
			goto finalize;
		}
		if (*need_restore)
			*need_restore = _hg_btree_node_restore(qnode_node, qnode_keys, qnode_vals, qnode_nodes, i, error);
	}
  finalize:
	HG_BTREE_NODE_UNLOCK (mem, qnode, qnode);

	return retval;
}

G_INLINE_FUNC gboolean
_hg_btree_node_remove_data(hg_btree_node_t  *node,
			   hg_quark_t       *qkeys,
			   hg_quark_t       *qvals,
			   hg_quark_t       *qnodes,
			   gsize             pos,
			   GError          **error)
{
	hg_btree_t *tree;
	gsize size;

	hg_return_val_with_gerror_if_lock_fail (tree,
						node->mem,
						node->qparent,
						error,
						FALSE);
	size = tree->size;
	hg_mem_unlock_object(node->mem, node->qparent);

	while (++pos < node->n_data) {
		qkeys[pos - 1] = qkeys[pos];
		qvals[pos - 1] = qvals[pos];
		qnodes[pos] = qnodes[pos + 1];
	}

	return --(node->n_data) < size;
}

G_INLINE_FUNC gboolean
_hg_btree_node_restore(hg_btree_node_t  *node,
		       hg_quark_t       *qkeys,
		       hg_quark_t       *qvals,
		       hg_quark_t       *qnodes,
		       gsize             pos,
		       GError          **error)
{
	hg_btree_t *tree;
	hg_btree_node_t *tnode;
	gsize size;

	hg_return_val_with_gerror_if_lock_fail (tree,
						node->mem,
						node->qparent,
						error,
						FALSE);
	size = tree->size;
	hg_mem_unlock_object(node->mem, node->qparent);

	if (pos > 0) {
		gsize ndata;

		hg_return_val_with_gerror_if_lock_fail (tnode,
							node->mem,
							qnodes[pos - 1],
							error,
							FALSE);
		ndata = tnode->n_data;
		hg_mem_unlock_object(node->mem, qnodes[pos - 1]);

		if (ndata > size)
			_hg_btree_node_restore_right_balance(node, qkeys, qvals, qnodes, pos, error);
		else
			return _hg_btree_node_combine(node, qkeys, qvals, qnodes, pos, error);
	} else {
		gsize ndata;

		hg_return_val_with_gerror_if_lock_fail (tnode,
							node->mem,
							qnodes[1],
							error,
							FALSE);
		ndata = tnode->n_data;
		hg_mem_unlock_object(node->mem, qnodes[1]);

		if (ndata > size)
			_hg_btree_node_restore_left_balance(node, qkeys, qvals, qnodes, 1, error);
		else
			return _hg_btree_node_combine(node, qkeys, qvals, qnodes, 1, error);
	}

	return FALSE;
}

G_INLINE_FUNC void
_hg_btree_node_restore_right_balance(hg_btree_node_t  *node,
				     hg_quark_t       *qkeys,
				     hg_quark_t       *qvals,
				     hg_quark_t       *qnodes,
				     gsize             pos,
				     GError          **error)
{
	gsize i;
	hg_quark_t qleft, qright;
	hg_btree_node_t *ql_node = NULL, *qr_node = NULL;
	hg_quark_t *ql_keys = NULL, *ql_vals = NULL, *ql_nodes = NULL;
	hg_quark_t *qr_keys = NULL, *qr_vals = NULL, *qr_nodes = NULL;

	qleft = qnodes[pos - 1];
	qright = qnodes[pos];

	HG_BTREE_NODE_LOCK (node->mem, qleft, ql, "left", error);
	HG_BTREE_NODE_LOCK (node->mem, qright, qr, "right", error);

	for (i = qr_node->n_data; i > 0; i--) {
		qr_keys[i] = qr_keys[i - 1];
		qr_vals[i] = qr_vals[i - 1];
		qr_nodes[i + 1] = qr_nodes[i];
	}
	qr_nodes[1] = qr_nodes[0];
	qr_node->n_data++;
	qr_keys[0] = qkeys[pos - 1];
	qr_vals[0] = qvals[pos - 1];
	qkeys[pos - 1] = ql_keys[ql_node->n_data - 1];
	qvals[pos - 1] = ql_vals[ql_node->n_data - 1];
	qr_nodes[0] = ql_nodes[ql_node->n_data];
	ql_node->n_data--;

	HG_BTREE_NODE_UNLOCK (node->mem, qright, qr);
	HG_BTREE_NODE_UNLOCK (node->mem, qleft, ql);
}

G_INLINE_FUNC void
_hg_btree_node_restore_left_balance(hg_btree_node_t *node,
				    hg_quark_t       *qkeys,
				    hg_quark_t       *qvals,
				    hg_quark_t       *qnodes,
				    gsize             pos,
				    GError          **error)
{
	gsize i;
	hg_quark_t qleft, qright;
	hg_btree_node_t *ql_node = NULL, *qr_node = NULL;
	hg_quark_t *ql_keys = NULL, *ql_vals = NULL, *ql_nodes = NULL;
	hg_quark_t *qr_keys = NULL, *qr_vals = NULL, *qr_nodes = NULL;

	qleft = qnodes[pos - 1];
	qright = qnodes[pos];

	HG_BTREE_NODE_LOCK (node->mem, qleft, ql, "left", error);
	HG_BTREE_NODE_LOCK (node->mem, qright, qr, "right", error);

	ql_node->n_data++;
	ql_keys[ql_node->n_data - 1] = qkeys[pos - 1];
	ql_vals[ql_node->n_data - 1] = qvals[pos - 1];
	ql_nodes[ql_node->n_data] = qr_nodes[0];
	qkeys[pos - 1] = qr_keys[0];
	qvals[pos - 1] = qr_vals[0];
	qr_nodes[0] = qr_nodes[1];
	qr_node->n_data--;
	for (i = 1; i <= qr_node->n_data; i++) {
		qr_keys[i - 1] = qr_keys[i];
		qr_vals[i - 1] = qr_vals[i];
		qr_nodes[i] = qr_nodes[i + 1];
	}

	HG_BTREE_NODE_UNLOCK (node->mem, qright, qr);
	HG_BTREE_NODE_UNLOCK (node->mem, qleft, ql);
}

G_INLINE_FUNC gboolean
_hg_btree_node_combine(hg_btree_node_t  *node,
		       hg_quark_t       *qkeys,
		       hg_quark_t       *qvals,
		       hg_quark_t       *qnodes,
		       gsize             pos,
		       GError          **error)
{
	gsize i;
	hg_quark_t qleft, qright;
	hg_btree_node_t *ql_node = NULL, *qr_node = NULL;
	hg_quark_t *ql_keys = NULL, *ql_vals = NULL, *ql_nodes = NULL;
	hg_quark_t *qr_keys = NULL, *qr_vals = NULL, *qr_nodes = NULL;
	gboolean need_restore = FALSE;

	qleft = qnodes[pos - 1];
	qright = qnodes[pos];

	HG_BTREE_NODE_LOCK (node->mem, qleft, ql, "left", error);
	HG_BTREE_NODE_LOCK (node->mem, qright, qr, "right", error);

	ql_node->n_data++;
	ql_keys[ql_node->n_data - 1] = qkeys[pos - 1];
	ql_vals[ql_node->n_data - 1] = qvals[pos - 1];
	ql_nodes[ql_node->n_data] = qr_nodes[0];
	for (i = 1; i <= qr_node->n_data; i++) {
		ql_node->n_data++;
		ql_keys[ql_node->n_data - 1] = qr_keys[i - 1];
		ql_vals[ql_node->n_data - 1] = qr_vals[i - 1];
		ql_nodes[ql_node->n_data] = qr_nodes[i];
	}
	need_restore = _hg_btree_node_remove_data(node, qkeys, qvals, qnodes, pos - 1, error);
	/* 'right' page could be freed here though, it might be referred from
	 * somewhere. so the actual destroying the page relies on GC.
	 */

	HG_BTREE_NODE_UNLOCK (node->mem, qright, qr);
	HG_BTREE_NODE_UNLOCK (node->mem, qleft, ql);

	return need_restore;
}

G_INLINE_FUNC void
_hg_btree_node_foreach(hg_mem_t                  *mem,
		       hg_quark_t                 qnode,
		       hg_btree_traverse_func_t   func,
		       gpointer                   data,
		       GError                   **error)
{
	if (qnode != Qnil) {
		gsize i;
		hg_btree_node_t *qnode_node = NULL;
		hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;

		HG_BTREE_NODE_LOCK (mem, qnode, qnode, "", error);

		for (i = 0; i < qnode_node->n_data; i++) {
			_hg_btree_node_foreach(qnode_node->mem, qnode_nodes[i], func, data, error);
			if (*error)
				return;
			if (!func(qnode_node->mem, qnode_keys[i], qnode_vals[i], data, error))
				return;
		}
		_hg_btree_node_foreach(qnode_node->mem, qnode_nodes[qnode_node->n_data], func, data, error);

		HG_BTREE_NODE_UNLOCK (mem, qnode, qnode);
	}
}

static gboolean
_hg_btree_count_traverse(hg_mem_t    *mem,
			 hg_quark_t   qkey,
			 hg_quark_t   qval,
			 gpointer     data,
			 GError     **error)
{
	gsize *len = data;

	(*len)++;

	return TRUE;
}

/*< public >*/
/**
 * hg_btree_new:
 * @mem:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_btree_new(hg_mem_t *mem,
	     gsize     size,
	     gpointer *ret)
{
	hg_quark_t retval;
	hg_btree_t *tree = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (size > 0 && size <= G_MAXINT16, Qnil);

	retval = hg_mem_alloc(mem, sizeof (hg_btree_t), (gpointer *)&tree);
	if (retval != Qnil) {
		tree->self = retval;
		tree->mem = mem;
		tree->size = size;
		tree->root = Qnil;

		if (ret)
			*ret = tree;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_btree_destroy:
 * @tree:
 *
 * FIXME
 */
void
hg_btree_destroy(hg_mem_t   *mem,
		 hg_quark_t  qtree)
{
	hg_btree_t *tree;

	hg_return_if_fail (mem != NULL);

	if (qtree == Qnil)
		return;
	hg_return_if_lock_fail (tree, mem, qtree);
	_hg_btree_node_free(tree->mem, tree->root);
	hg_mem_free(tree->mem, tree->self);
}

/**
 * hg_btree_add:
 * @tree:
 * @qkey:
 * @qval:
 * @error:
 *
 * FIXME
 */
void
hg_btree_add(hg_btree_t  *tree,
	     hg_quark_t   qkey,
	     hg_quark_t   qval,
	     GError     **error)
{
	gboolean inserted;
	hg_btree_node_t *qnode_node;
	hg_quark_t new_node = Qnil, qnode, *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;
	GError *err = NULL;

	hg_return_with_gerror_if_fail (tree != NULL, error);

	inserted = _hg_btree_node_insert(tree->mem, tree->root, &qkey, &qval, &new_node, &err);
	if (err)
		goto finalize;
	if (!inserted) {
		qnode = _hg_btree_node_new(tree->mem, tree->self,
					   (gpointer *)&qnode_node,
					   (gpointer *)&qnode_keys,
					   (gpointer *)&qnode_vals,
					   (gpointer *)&qnode_nodes);
		if (qnode == Qnil) {
			g_set_error(&err, HG_ERROR, ENOMEM,
				    "%s: Out of memory.", __PRETTY_FUNCTION__);
			goto finalize;
		}
		qnode_node->n_data = 1;
		qnode_keys[0] = qkey;
		qnode_vals[0] = qval;
		qnode_nodes[0] = tree->root;
		qnode_nodes[1] = new_node;
		tree->root = qnode;

		HG_BTREE_NODE_UNLOCK_NO_LABEL (tree->mem, qnode, qnode);
	}
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)", err->message, err->code);
		}
		g_error_free(err);
	}
}

/**
 * hg_btree_remove:
 * @tree:
 * @qkey:
 * @error:
 *
 * FIXME
 */
gboolean
hg_btree_remove(hg_btree_t  *tree,
		hg_quark_t   qkey,
		GError     **error)
{
	gboolean removed = FALSE, need_restore = FALSE;
	hg_quark_t qval = Qnil, qroot = Qnil;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (tree != NULL, FALSE, error);

	removed = _hg_btree_node_remove(tree->mem, tree->root, &qkey, &qval, &need_restore, &err);
	if (err)
		goto finalize;
	if (removed) {
		hg_btree_node_t *root_node;

		qroot = tree->root;
		hg_return_val_with_gerror_if_lock_fail (root_node,
							tree->mem,
							tree->root,
							error,
							FALSE);

		if (root_node->n_data == 0) {
			hg_quark_t *root_nodes;

			root_nodes = HG_MEM_LOCK (root_node->mem,
						  root_node->qnodes,
						  error);
			if (root_nodes == NULL) {
				goto qfinalize;
			}
			tree->root = root_nodes[0];
			/* 'page' page could be freed here though, it might be
			 * referred from somewhere. so the actual destroying
			 * the page relies on GC.
			 */
			hg_mem_unlock_object(root_node->mem, root_node->qnodes);
		}
	  qfinalize:
		hg_mem_unlock_object(tree->mem, qroot);
	}
  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)", err->message, err->code);
		}
		g_error_free(err);
	}

	return removed;
}

/**
 * hg_btree_find:
 * @tree:
 * @qkey:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_btree_find(hg_btree_t  *tree,
	      hg_quark_t   qkey,
	      GError     **error)
{
	hg_btree_node_t *qnode_node = NULL;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;
	hg_quark_t retval = Qnil, qnode;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (tree != NULL, Qnil, error);

	qnode = tree->root;
	HG_BTREE_NODE_LOCK (tree->mem, qnode, qnode, "", &err);
	while (qnode_node != NULL) {
		hg_quark_t q;
		gsize i;

		if (qnode_keys[qnode_node->n_data - 1] >= qkey) {
			for (i = 0; i < qnode_node->n_data && qnode_keys[i] < qkey; i++);
			if (i < qnode_node->n_data && qnode_keys[i] == qkey) {
				retval = qnode_vals[i];
				break;
			}
		} else {
			i = qnode_node->n_data;
		}
		q = qnode_nodes[i];
		HG_BTREE_NODE_UNLOCK_NO_LABEL (tree->mem, qnode, qnode);
		qnode = q;
		HG_BTREE_NODE_LOCK (tree->mem, qnode, qnode, "", &err);
	}

	HG_BTREE_NODE_UNLOCK (tree->mem, qnode, qnode);

	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)", err->message, err->code);
		}
		g_error_free(err);
	}

	return retval;
}

#if 0
gpointer
hg_btree_find_near(HgBTree *tree,
		   gpointer key)
{
	HgBTreePage *page, *prev = NULL;
	guint16 i = 0, prev_pos = 0;

	g_return_val_if_fail (tree != NULL, NULL);

	page = tree->root;
	while (page != NULL) {
		if (page->key[page->n_data - 1] >= key) {
			for (i = 0; i < page->n_data && page->key[i] < key; i++);
			if (i < page->n_data && page->key[i] == key)
				return page->val[i];
			prev = page;
			prev_pos = i;
		} else {
			i = page->n_data;
		}
		page = page->page[i];
	}
	if (prev) {
		if (prev_pos < prev->n_data) {
			/* prev->val[i - 1] should be less than key */
			return prev->val[prev_pos];
		}
		/* no items is bigger than key found. */
	}

	return NULL;
}
#endif

/**
 * hg_btree_foreach:
 * @tree:
 * @func:
 * @data:
 * @error:
 *
 * FIXME
 */
void
hg_btree_foreach(hg_btree_t                *tree,
		 hg_btree_traverse_func_t   func,
		 gpointer                   data,
		 GError                   **error)
{
	GError *err = NULL;

	hg_return_with_gerror_if_fail (tree != NULL, error);
	hg_return_with_gerror_if_fail (func != NULL, error);

	_hg_btree_node_foreach(tree->mem, tree->root, func, data, &err);
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)", err->message, err->code);
		}
		g_error_free(err);
	}
}

/**
 * hg_btree_length:
 * @tree:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_btree_length(hg_btree_t  *tree,
		GError     **error)
{
	gsize retval = 0;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (tree != NULL, 0, error);

	hg_btree_foreach(tree, _hg_btree_count_traverse, &retval, &err);
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)", err->message, err->code);
		}
		g_error_free(err);
	}

	return retval;
}
