/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.c
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
#include "config.h"
#endif

#include "hgerror.h"
#include "hgmem.h"
#include "hgstring.h"
#include "hgdict-private.h"
#include "hgdict.h"

#include "hgdict.proto.h"

#define HG_DICT_NODE_SIZE	5


typedef struct _hg_dict_item_t {
	hg_quark_t qkey;
	hg_quark_t qval;
} hg_dict_item_t;
typedef struct _hg_dict_compare_data_t {
	hg_dict_t               *opposite_dict;
	hg_quark_compare_func_t  func;
	hg_pointer_t             user_data;
	hg_bool_t                result;
} hg_dict_compare_data_t;


HG_PROTO_VTABLE_ACL (dict);
HG_DEFINE_VTABLE_WITH (dict, NULL,
		       _hg_object_dict_set_acl,
		       _hg_object_dict_get_acl);
HG_DEFINE_VTABLE_WITH (dict_node, NULL, NULL, NULL);

static hg_usize_t __hg_dict_node_size = HG_DICT_NODE_SIZE;

/*< private >*/
static hg_usize_t
_hg_object_dict_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_dict_t));
}

static hg_uint_t
_hg_object_dict_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_dict_initialize(hg_object_t *object,
			   va_list      args)
{
	hg_dict_t *dict = (hg_dict_t *)object;

	dict->qroot = Qnil;
	dict->length = 0;
	dict->allocated_size = va_arg(args, hg_usize_t);
	dict->raise_dictfull = va_arg(args, hg_bool_t);

	return TRUE;
}

static hg_quark_t
_hg_object_dict_copy(hg_object_t              *object,
		     hg_quark_iterate_func_t   func,
		     hg_pointer_t              user_data,
		     hg_pointer_t             *ret)
{
	hg_return_val_if_fail (object->type == HG_TYPE_DICT, Qnil, HG_e_typecheck);

	return object->self;
}

static hg_char_t *
_hg_object_dict_to_cstr(hg_object_t             *object,
			hg_quark_iterate_func_t  func,
			hg_pointer_t             user_data)
{
	hg_return_val_if_fail (object->type == HG_TYPE_DICT, NULL, HG_e_typecheck);

	return g_strdup("-dict-");
}

static hg_bool_t
_hg_object_dict_gc_mark(hg_object_t *object)
{
	hg_dict_t *dict;

	hg_return_val_if_fail (object->type == HG_TYPE_DICT, FALSE, HG_e_VMerror);

	dict = (hg_dict_t *)object;
	hg_debug(HG_MSGCAT_GC, "dict: marking a root dict node.");

	return hg_mem_gc_mark(dict->o.mem, dict->qroot);
}

static hg_bool_t
_hg_dict_traverse_compare(hg_mem_t     *mem,
			  hg_quark_t    qkey,
			  hg_quark_t    qval,
			  hg_pointer_t  data)
{
	hg_dict_compare_data_t *cdata = data;
	hg_quark_t q;

	if ((q = hg_dict_lookup(cdata->opposite_dict, qkey)) == Qnil) {
		cdata->result = FALSE;

		return FALSE;
	}
	if (!cdata->func(qval, q, cdata->user_data)) {
		cdata->result = FALSE;

		return FALSE;
	}

	return TRUE;
}

static hg_bool_t
_hg_object_dict_compare(hg_object_t             *o1,
			hg_object_t             *o2,
			hg_quark_compare_func_t  func,
			hg_pointer_t             user_data)
{
	hg_dict_compare_data_t data;

	hg_return_val_if_fail (o1->type == HG_TYPE_DICT, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (o2->type == HG_TYPE_DICT, FALSE, HG_e_typecheck);

	if (hg_dict_length((hg_dict_t *)o1) != hg_dict_length((hg_dict_t *)o2))
		return FALSE;

	data.opposite_dict = (hg_dict_t *)o2;
	data.func = func;
	data.user_data = user_data;
	data.result = TRUE;

	hg_dict_foreach((hg_dict_t *)o1, _hg_dict_traverse_compare, &data);

	return data.result;
}

static void
_hg_object_dict_set_acl(hg_object_t *object,
			hg_int_t     readable,
			hg_int_t     writable,
			hg_int_t     executable,
			hg_int_t     editable)
{
	if (readable != 0) {
		if (readable > 0)
			object->acl |= HG_ACL_READABLE;
		else
			object->acl &= ~HG_ACL_READABLE;
	}
	if (writable != 0) {
		if (writable > 0)
			object->acl |= HG_ACL_WRITABLE;
		else
			object->acl &= ~HG_ACL_WRITABLE;
	}
	if (executable != 0) {
		if (executable > 0)
			object->acl |= HG_ACL_EXECUTABLE;
		else
			object->acl &= ~HG_ACL_EXECUTABLE;
	}
	if (editable != 0) {
		if (editable > 0)
			object->acl |= HG_ACL_ACCESSIBLE;
		else
			object->acl &= ~HG_ACL_ACCESSIBLE;
	}
}

static hg_quark_acl_t
_hg_object_dict_get_acl(hg_object_t *object)
{
	return object->acl;
}

static hg_usize_t
_hg_object_dict_node_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_dict_node_t));
}

static hg_uint_t
_hg_object_dict_node_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_dict_node_initialize(hg_object_t *object,
				va_list      args)
{
	hg_dict_node_t *dnode = (hg_dict_node_t *)object;
	hg_quark_t *keys = NULL, *vals = NULL, *nodes = NULL;
	hg_pointer_t *ret_key, *ret_val, *ret_nodes;
	hg_usize_t i;

	ret_key = va_arg(args, hg_pointer_t *);
	ret_val = va_arg(args, hg_pointer_t *);
	ret_nodes = va_arg(args, hg_pointer_t *);

	dnode->qkey = hg_mem_alloc(object->mem,
				   sizeof (hg_quark_t) * __hg_dict_node_size * 2,
				   (hg_pointer_t *)&keys);
	dnode->qval = hg_mem_alloc(object->mem,
				   sizeof (hg_quark_t) * __hg_dict_node_size * 2,
				   (hg_pointer_t *)&vals);
	dnode->qnodes = hg_mem_alloc(object->mem,
				     sizeof (hg_quark_t) * __hg_dict_node_size * 2 + 1,
				     (hg_pointer_t *)&nodes);
	dnode->n_data = 0;
	hg_return_val_after_eval_if_fail (dnode->qkey != Qnil &&
					  dnode->qval != Qnil &&
					  dnode->qnodes != Qnil,
					  FALSE, _hg_dict_node_free(object->mem, object->self),
					  HG_e_VMerror);

	for (i = 0; i < __hg_dict_node_size * 2; i++) {
		((hg_quark_t *)keys)[i] = Qnil;
		((hg_quark_t *)vals)[i] = Qnil;
		((hg_quark_t *)nodes)[i] = Qnil;
	}
	((hg_quark_t *)nodes)[i] = Qnil;

	if (ret_key)
		*ret_key = keys;
	else
		hg_mem_unlock_object(object->mem, dnode->qkey);
	if (ret_val)
		*ret_val = vals;
	else
		hg_mem_unlock_object(object->mem, dnode->qval);
	if (ret_nodes)
		*ret_nodes = nodes;
	else
		hg_mem_unlock_object(object->mem, dnode->qnodes);

	return TRUE;
}

static hg_quark_t
_hg_object_dict_node_copy(hg_object_t              *object,
			  hg_quark_iterate_func_t   func,
			  hg_pointer_t              user_data,
			  hg_pointer_t             *ret)
{
	hg_return_val_if_fail (object->type == HG_TYPE_DICT_NODE, Qnil, HG_e_typecheck);

	return object->self;
}

static hg_char_t *
_hg_object_dict_node_to_cstr(hg_object_t             *object,
			     hg_quark_iterate_func_t  func,
			     hg_pointer_t             user_data)
{
	hg_return_val_if_fail (object->type == HG_TYPE_DICT_NODE, NULL, HG_e_typecheck);

	return g_strdup("-dnode-");
}

static hg_bool_t
_hg_object_dict_node_gc_mark(hg_object_t *object)
{
	hg_dict_node_t *dnode = (hg_dict_node_t *)object;
	hg_usize_t i;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;

	hg_return_val_if_fail (object->type == HG_TYPE_DICT_NODE, FALSE, HG_e_typecheck);

	hg_debug(HG_MSGCAT_GC, "dictnode: marking key container");
	if (!hg_mem_gc_mark(dnode->o.mem, dnode->qkey))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "dictnode: marking value container");
	if (!hg_mem_gc_mark(dnode->o.mem, dnode->qval))
		return FALSE;
	hg_debug(HG_MSGCAT_GC, "dictnode: marking node container");
	if (!hg_mem_gc_mark(dnode->o.mem, dnode->qnodes))
		return FALSE;

	qnode_keys = hg_mem_lock_object(dnode->o.mem, dnode->qkey);
	qnode_vals = hg_mem_lock_object(dnode->o.mem, dnode->qval);
	qnode_nodes = hg_mem_lock_object(dnode->o.mem, dnode->qnodes);
	if (qnode_keys == NULL ||
	    qnode_vals == NULL ||
	    qnode_nodes == NULL) {
		hg_critical("%s: Invalid quark to obtain the actual object in dnode",
			    __PRETTY_FUNCTION__);
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		goto qfinalize;
	}

	for (i = 0; i < dnode->n_data; i++) {
		hg_mem_t *m;

		hg_debug(HG_MSGCAT_GC, "dictnode: marking node[%ld]", i);
		if (!hg_mem_gc_mark(dnode->o.mem, qnode_nodes[i]))
			goto qfinalize;
		hg_debug(HG_MSGCAT_GC, "dictnode: marking key[%ld]", i);
		m = hg_mem_spool_get(hg_quark_get_mem_id(qnode_keys[i]));
		if (!hg_mem_gc_mark(m, qnode_keys[i]))
			goto qfinalize;
		hg_debug(HG_MSGCAT_GC, "dictnode: marking value[%ld]", i);
		m = hg_mem_spool_get(hg_quark_get_mem_id(qnode_vals[i]));
		if (!hg_mem_gc_mark(m, qnode_vals[i]))
			goto qfinalize;
	}
	hg_debug(HG_MSGCAT_GC, "dictnode: marking node[%ld]", dnode->n_data);
	if (!hg_mem_gc_mark(dnode->o.mem, qnode_nodes[dnode->n_data]))
		goto qfinalize;
  qfinalize:
	if (qnode_keys)
		hg_mem_unlock_object(dnode->o.mem, dnode->qkey);
	if (qnode_vals)
		hg_mem_unlock_object(dnode->o.mem, dnode->qval);
	if (qnode_nodes)
		hg_mem_unlock_object(dnode->o.mem, dnode->qnodes);

	return HG_ERROR_IS_SUCCESS0 ();
}

static hg_bool_t
_hg_object_dict_node_compare(hg_object_t             *o1,
			     hg_object_t             *o2,
			     hg_quark_compare_func_t  func,
			     hg_pointer_t             user_data)
{
	/* shouldn't be reached */
	return FALSE;
}

HG_INLINE_FUNC hg_quark_t
_hg_dict_node_new(hg_mem_t     *mem,
		  hg_pointer_t *ret,
		  hg_pointer_t *ret_key,
		  hg_pointer_t *ret_val,
		  hg_pointer_t *ret_nodes)
{
	hg_quark_t retval;
	hg_dict_node_t *dnode;

	retval = hg_object_new(mem, (hg_pointer_t *)&dnode, HG_TYPE_DICT_NODE, 0,
			       ret_key, ret_val, ret_nodes);
	if (retval != Qnil) {
		if (ret)
			*ret = dnode;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

HG_INLINE_FUNC void
_hg_dict_node_free(hg_mem_t   *mem,
		   hg_quark_t  qdata)
{
	hg_dict_node_t *dnode;

	if (qdata == Qnil)
		return;

	hg_return_if_lock_fail (dnode, mem, qdata);

	hg_mem_free(mem, dnode->qkey);
	hg_mem_free(mem, dnode->qval);
	hg_mem_free(mem, dnode->qnodes);
	hg_mem_free(mem, qdata);
}

HG_INLINE_FUNC hg_bool_t
_hg_dict_node_insert(hg_mem_t   *mem,
		     hg_quark_t  qnode,
		     hg_quark_t *qkey,
		     hg_quark_t *qval,
		     hg_quark_t *qright_node)
{
	hg_usize_t i;
	hg_bool_t retval = FALSE;
	hg_dict_node_t *qnode_node = NULL;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;

	if (qnode == Qnil) {
		*qright_node = Qnil;
		return FALSE;
	}
	HG_DICT_NODE_LOCK (mem, qnode, qnode, "");

	if (qnode_keys[qnode_node->n_data - 1] >= *qkey) {
		for (i = 0; i < qnode_node->n_data && qnode_keys[i] < *qkey; i++);
		if (i < qnode_node->n_data && qnode_keys[i] == *qkey) {
			hg_debug(HG_MSGCAT_DICT, "Inserting val %lx with key %lx at index %lx on node %p\n", *qval, *qkey, i, qnode_nodes);
			qnode_keys[i] = *qkey;
			qnode_vals[i] = *qval;

			retval = TRUE;
			goto finalize;
		}
	} else {
		i = qnode_node->n_data;
	}
	retval = _hg_dict_node_insert(mem, qnode_nodes[i], qkey, qval, qright_node);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		retval = FALSE;
		goto finalize;
	}
	if (!retval) {
		if (qnode_node->n_data < (__hg_dict_node_size * 2)) {
			_hg_dict_node_insert_data(qnode_node, qnode_keys, qnode_vals, qnode_nodes, *qkey, *qval, i, qright_node);
			retval = TRUE;
		} else {
			_hg_dict_node_balance(qnode_node, qnode_keys, qnode_vals, qnode_nodes, qkey, qval, i, qright_node);
			retval = FALSE;
		}
	}
  finalize:
	HG_DICT_NODE_UNLOCK (mem, qnode, qnode);

	return retval;
}

HG_INLINE_FUNC void
_hg_dict_node_insert_data(hg_dict_node_t *node,
			  hg_quark_t     *qkeys,
			  hg_quark_t     *qvals,
			  hg_quark_t     *qnodes,
			  hg_quark_t      qkey,
			  hg_quark_t      qval,
			  hg_usize_t      pos,
			  hg_quark_t     *qright_node)
{
	hg_usize_t i;

	for (i = node->n_data; i > pos; i--) {
		qkeys[i] = qkeys[i - 1];
		qvals[i] = qvals[i - 1];
		qnodes[i + 1] = qnodes[i];
	}
	hg_debug(HG_MSGCAT_DICT, "Inserting val %lx with key %lx at index %lx on node %p\n", qval, qkey, pos, qnodes);
	qkeys[pos] = qkey;
	qvals[pos] = qval;
	qnodes[pos + 1] = *qright_node;
	node->n_data++;
}

HG_INLINE_FUNC void
_hg_dict_node_balance(hg_dict_node_t *node,
		      hg_quark_t     *qnode_keys,
		      hg_quark_t     *qnode_vals,
		      hg_quark_t     *qnode_nodes,
		      hg_quark_t     *qkey,
		      hg_quark_t     *qval,
		      hg_usize_t      pos,
		      hg_quark_t     *qright_node)
{
	hg_usize_t i, node_size;
	hg_quark_t q, *qnewnode_keys, *qnewnode_vals, *qnewnode_nodes;
	hg_dict_node_t *new_node;

	if (pos <= __hg_dict_node_size)
		node_size = __hg_dict_node_size;
	else
		node_size = __hg_dict_node_size + 1;
	q = _hg_dict_node_new(node->o.mem,
			      (hg_pointer_t *)&new_node,
			      (hg_pointer_t *)&qnewnode_keys,
			      (hg_pointer_t *)&qnewnode_vals,
			      (hg_pointer_t *)&qnewnode_nodes);
	if (q == Qnil) {
		return;
	}
	for (i = node_size + 1; i <= __hg_dict_node_size * 2; i++) {
		qnewnode_keys[i - node_size - 1] = qnode_keys[i - 1];
		qnewnode_vals[i - node_size - 1] = qnode_vals[i - 1];
		qnewnode_nodes[i - node_size] = qnode_nodes[i];
	}
	new_node->n_data = __hg_dict_node_size * 2 - node_size;
	node->n_data = node_size;
	if (pos <= __hg_dict_node_size) {
		_hg_dict_node_insert_data(node,
					  qnode_keys,
					  qnode_vals,
					  qnode_nodes,
					  *qkey, *qval,
					  pos, qright_node);
	} else {
		_hg_dict_node_insert_data(new_node,
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

	hg_mem_unlock_object(node->o.mem, new_node->qkey);
	hg_mem_unlock_object(node->o.mem, new_node->qval);
	hg_mem_unlock_object(node->o.mem, new_node->qnodes);
	hg_mem_unlock_object(node->o.mem, q);
	hg_mem_unref(node->o.mem, q);
}

HG_INLINE_FUNC hg_bool_t
_hg_dict_node_remove(hg_mem_t   *mem,
		     hg_quark_t  qnode,
		     hg_quark_t *qkey,
		     hg_quark_t *qval,
		     hg_bool_t  *need_restore)
{
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL, q;
	hg_dict_node_t *qnode_node, *p;
	hg_usize_t i;
	hg_bool_t retval = FALSE;

	if (qnode == Qnil)
		return FALSE;

	HG_DICT_NODE_LOCK (mem, qnode, qnode, "");

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
					hg_mem_unlock_object(qnode_node->o.mem, q);
				q = qtnode[0];
				if (qqnode != Qnil)
					hg_mem_unlock_object(qnode_node->o.mem, qqnode);
				p = hg_mem_lock_object(qnode_node->o.mem, q);
				if (p == NULL) {
					hg_warning("%s: Invalid quark to obtain the node in the nodes container object: mem: %p, quark: %lx in the node %p(q: %lx)",
						    __PRETTY_FUNCTION__, mem, q, qtnode, qqnode != Qnil ? qqnode : qnode_nodes[i + 1]);
					retval = FALSE;
					goto finalize;
				}
				qqnode = p->qnodes;
				qtnode = hg_mem_lock_object(qnode_node->o.mem, qqnode);
			} while (qtnode[0] != Qnil);

			qtkey = HG_MEM_LOCK (qnode_node->o.mem, p->qkey);
			if (qtkey == NULL) {
				retval = FALSE;
				goto finalize;
			}
			qtval = HG_MEM_LOCK (qnode_node->o.mem, p->qval);
			if (qtval == NULL) {
				retval = FALSE;
				goto finalize;
			}

			qnode_keys[i] = *qkey = qtkey[0];
			qnode_vals[i] = *qval = qtval[0];

			hg_mem_unlock_object(qnode_node->o.mem, p->qkey);
			hg_mem_unlock_object(qnode_node->o.mem, p->qval);
			hg_mem_unlock_object(qnode_node->o.mem, p->qnodes);
			hg_mem_unlock_object(qnode_node->o.mem, q);

			retval = _hg_dict_node_remove(qnode_node->o.mem, qnode_nodes[i + 1], qkey, qval, need_restore);
			if (!HG_ERROR_IS_SUCCESS0 ()) {
				retval = FALSE;
				goto finalize;
			}
			if (*need_restore)
				*need_restore = _hg_dict_node_restore(qnode_node, qnode_keys, qnode_vals, qnode_nodes, i + 1);
		} else {
			*need_restore = _hg_dict_node_remove_data(qnode_node, qnode_keys, qnode_vals, qnode_nodes, i);
			if (!HG_ERROR_IS_SUCCESS0 ()) {
				retval = FALSE;
				goto finalize;
			}
		}
	} else {
		retval = _hg_dict_node_remove(qnode_node->o.mem, qnode_nodes[i], qkey, qval, need_restore);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			retval = FALSE;
			goto finalize;
		}
		if (*need_restore)
			*need_restore = _hg_dict_node_restore(qnode_node, qnode_keys, qnode_vals, qnode_nodes, i);
	}
  finalize:
	HG_DICT_NODE_UNLOCK (mem, qnode, qnode);

	return retval;
}

HG_INLINE_FUNC hg_bool_t
_hg_dict_node_remove_data(hg_dict_node_t *node,
			  hg_quark_t     *qkeys,
			  hg_quark_t     *qvals,
			  hg_quark_t     *qnodes,
			  hg_usize_t      pos)
{
	while (++pos < node->n_data) {
		qkeys[pos - 1] = qkeys[pos];
		qvals[pos - 1] = qvals[pos];
		qnodes[pos] = qnodes[pos + 1];
	}

	return --(node->n_data) < __hg_dict_node_size;
}

HG_INLINE_FUNC hg_bool_t
_hg_dict_node_restore(hg_dict_node_t *node,
		      hg_quark_t     *qkeys,
		      hg_quark_t     *qvals,
		      hg_quark_t     *qnodes,
		      hg_usize_t      pos)
{
	hg_dict_node_t *tnode;

	if (pos > 0) {
		hg_usize_t ndata;

		hg_return_val_if_lock_fail (tnode,
					    node->o.mem,
					    qnodes[pos - 1],
					    FALSE);
		ndata = tnode->n_data;
		hg_mem_unlock_object(node->o.mem, qnodes[pos - 1]);

		if (ndata > __hg_dict_node_size)
			_hg_dict_node_restore_right_balance(node, qkeys, qvals, qnodes, pos);
		else
			return _hg_dict_node_combine(node, qkeys, qvals, qnodes, pos);
	} else {
		hg_usize_t ndata;

		hg_return_val_if_lock_fail (tnode,
					    node->o.mem,
					    qnodes[1],
					    FALSE);
		ndata = tnode->n_data;
		hg_mem_unlock_object(node->o.mem, qnodes[1]);

		if (ndata > __hg_dict_node_size)
			_hg_dict_node_restore_left_balance(node, qkeys, qvals, qnodes, 1);
		else
			return _hg_dict_node_combine(node, qkeys, qvals, qnodes, 1);
	}

	return FALSE;
}

HG_INLINE_FUNC void
_hg_dict_node_restore_right_balance(hg_dict_node_t *node,
				    hg_quark_t     *qkeys,
				    hg_quark_t     *qvals,
				    hg_quark_t     *qnodes,
				    hg_usize_t      pos)
{
	hg_usize_t i;
	hg_quark_t qleft, qright;
	hg_dict_node_t *ql_node = NULL, *qr_node = NULL;
	hg_quark_t *ql_keys = NULL, *ql_vals = NULL, *ql_nodes = NULL;
	hg_quark_t *qr_keys = NULL, *qr_vals = NULL, *qr_nodes = NULL;

	qleft = qnodes[pos - 1];
	qright = qnodes[pos];

	HG_DICT_NODE_LOCK (node->o.mem, qleft, ql, "left");
	HG_DICT_NODE_LOCK (node->o.mem, qright, qr, "right");

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

	HG_DICT_NODE_UNLOCK (node->o.mem, qright, qr);
	HG_DICT_NODE_UNLOCK (node->o.mem, qleft, ql);
}

HG_INLINE_FUNC void
_hg_dict_node_restore_left_balance(hg_dict_node_t *node,
				   hg_quark_t     *qkeys,
				   hg_quark_t     *qvals,
				   hg_quark_t     *qnodes,
				   hg_usize_t      pos)
{
	hg_usize_t i;
	hg_quark_t qleft, qright;
	hg_dict_node_t *ql_node = NULL, *qr_node = NULL;
	hg_quark_t *ql_keys = NULL, *ql_vals = NULL, *ql_nodes = NULL;
	hg_quark_t *qr_keys = NULL, *qr_vals = NULL, *qr_nodes = NULL;

	qleft = qnodes[pos - 1];
	qright = qnodes[pos];

	HG_DICT_NODE_LOCK (node->o.mem, qleft, ql, "left");
	HG_DICT_NODE_LOCK (node->o.mem, qright, qr, "right");

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

	HG_DICT_NODE_UNLOCK (node->o.mem, qright, qr);
	HG_DICT_NODE_UNLOCK (node->o.mem, qleft, ql);
}

HG_INLINE_FUNC hg_bool_t
_hg_dict_node_combine(hg_dict_node_t *node,
		      hg_quark_t     *qkeys,
		      hg_quark_t     *qvals,
		      hg_quark_t     *qnodes,
		      hg_usize_t      pos)
{
	hg_usize_t i;
	hg_quark_t qleft, qright;
	hg_dict_node_t *ql_node = NULL, *qr_node = NULL;
	hg_quark_t *ql_keys = NULL, *ql_vals = NULL, *ql_nodes = NULL;
	hg_quark_t *qr_keys = NULL, *qr_vals = NULL, *qr_nodes = NULL;
	hg_bool_t need_restore = FALSE;

	qleft = qnodes[pos - 1];
	qright = qnodes[pos];

	HG_DICT_NODE_LOCK (node->o.mem, qleft, ql, "left");
	HG_DICT_NODE_LOCK (node->o.mem, qright, qr, "right");

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
	need_restore = _hg_dict_node_remove_data(node, qkeys, qvals, qnodes, pos - 1);
	/* 'right' page could be freed here though, it might be referred from
	 * somewhere. so the actual destroying the page relies on GC.
	 */

	HG_DICT_NODE_UNLOCK (node->o.mem, qright, qr);
	HG_DICT_NODE_UNLOCK (node->o.mem, qleft, ql);

	return need_restore;
}

HG_INLINE_FUNC void
_hg_dict_node_foreach(hg_mem_t                *mem,
		      hg_quark_t               qnode,
		      hg_dict_traverse_func_t  func,
		      hg_pointer_t             data)
{
	if (qnode != Qnil) {
		hg_usize_t i;
		hg_dict_node_t *qnode_node = NULL;
		hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;

		HG_DICT_NODE_LOCK (mem, qnode, qnode, "");

		for (i = 0; i < qnode_node->n_data; i++) {
			_hg_dict_node_foreach(qnode_node->o.mem, qnode_nodes[i], func, data);
			if (!HG_ERROR_IS_SUCCESS0 ())
				goto error;
			if (!func(qnode_node->o.mem, qnode_keys[i], qnode_vals[i], data))
				goto error;
		}
		_hg_dict_node_foreach(qnode_node->o.mem, qnode_nodes[qnode_node->n_data], func, data);

	  error:
		HG_DICT_NODE_UNLOCK (mem, qnode, qnode);
	}
}

static hg_bool_t
_hg_dict_traverse_first_item(hg_mem_t     *mem,
			     hg_quark_t    qkey,
			     hg_quark_t    qval,
			     hg_pointer_t  data)
{
	hg_dict_item_t *x = data;

	x->qkey = qkey;
	x->qval = qval;

	return FALSE;
}

/*< public >*/
void
_hg_dict_node_set_size(hg_usize_t size)
{
	/* testing purpose */
	__hg_dict_node_size = size;
}

/**
 * hg_dict_new:
 * @mem:
 * @size:
 * @raise_dictfull:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_dict_new(hg_mem_t     *mem,
	    hg_usize_t    size,
	    hg_bool_t     raise_dictfull,
	    hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_dict_t *dict = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (size < (HG_DICT_MAX_SIZE + 1), Qnil, HG_e_limitcheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	retval = hg_object_new(mem, (hg_pointer_t *)&dict, HG_TYPE_DICT, 0, size, raise_dictfull);
	if (retval != Qnil) {
		if (ret)
			*ret = dict;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_dict_add:
 * @dict:
 * @qkey:
 * @qval:
 * @force:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_dict_add(hg_dict_t  *dict,
	    hg_quark_t  qkey,
	    hg_quark_t  qval,
	    hg_bool_t   force)
{
	hg_bool_t inserted;
	hg_dict_node_t *qnode_node;
	hg_quark_t new_node = Qnil, qnode, *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;
	hg_quark_t qmasked;
	hg_mem_t *m;

	hg_return_val_if_fail (dict != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (dict->o.type == HG_TYPE_DICT, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (!HG_IS_QSTRING (qkey), FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	if (!force &&
	    hg_mem_spool_get_type(dict->o.mem) == HG_MEM_TYPE_GLOBAL) {
		if (!(hg_quark_is_simple_object(qkey) ||
		      hg_quark_get_type(qkey) == HG_TYPE_OPER) &&
		    hg_mem_spool_get_type(hg_mem_spool_get(hg_quark_get_mem_id(qkey))) == HG_MEM_TYPE_LOCAL) {
			hg_debug(HG_MSGCAT_DICT, "Unable to store the object allocated in the local memory into the global memory");
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_invalidaccess);

			return FALSE;
		}
		if (!(hg_quark_is_simple_object(qval) ||
		      hg_quark_get_type(qval) == HG_TYPE_OPER) &&
		    hg_mem_spool_get_type(hg_mem_spool_get(hg_quark_get_mem_id(qval))) == HG_MEM_TYPE_LOCAL) {
			hg_debug(HG_MSGCAT_DICT, "Unable to store the object allocated in the local memory into the global memory");
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_invalidaccess);

			return FALSE;
		}
	}
	if (dict->raise_dictfull &&
	    hg_dict_length(dict) == hg_dict_maxlength(dict) &&
	    hg_dict_lookup(dict, qkey) == Qnil) {
		hg_debug(HG_MSGCAT_DICT, "no more spaces in the dict");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_dictfull);

		return FALSE;
	}

	qmasked = hg_quark_get_hash(qkey);
	inserted = _hg_dict_node_insert(dict->o.mem, dict->qroot,
					&qmasked, &qval, &new_node);
	if (!HG_ERROR_IS_SUCCESS0 ())
		return FALSE;
	if (!inserted) {
		qnode = _hg_dict_node_new(dict->o.mem,
					  (hg_pointer_t *)&qnode_node,
					  (hg_pointer_t *)&qnode_keys,
					  (hg_pointer_t *)&qnode_vals,
					  (hg_pointer_t *)&qnode_nodes);
		if (qnode == Qnil) {
			return FALSE;
		}
		qnode_node->n_data = 1;
		qnode_keys[0] = qmasked;
		qnode_vals[0] = qval;
		qnode_nodes[0] = dict->qroot;
		qnode_nodes[1] = new_node;
		dict->qroot = qnode;

		HG_DICT_NODE_UNLOCK_NO_LABEL (dict->o.mem, qnode, qnode);
		hg_mem_unref(dict->o.mem, qnode);
	}
	if (!hg_quark_is_simple_object(qkey) &&
	    hg_quark_get_type(qkey) != HG_TYPE_OPER) {
		m = hg_mem_spool_get(hg_quark_get_mem_id(qkey));
		hg_mem_unref(m, qkey);
	}
	if (!hg_quark_is_simple_object(qval) &&
	    hg_quark_get_type(qval) != HG_TYPE_OPER) {
		m = hg_mem_spool_get(hg_quark_get_mem_id(qval));
		hg_mem_unref(m, qval);
	}
	dict->length++;

	return TRUE;
}

/**
 * hg_dict_remove:
 * @dict:
 * @qkey:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_dict_remove(hg_dict_t  *dict,
	       hg_quark_t  qkey)
{
	hg_bool_t removed = FALSE, need_restore = FALSE;
	hg_quark_t qval = Qnil, qroot = Qnil;
	hg_quark_t qmasked;

	hg_return_val_if_fail (dict != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (dict->o.type == HG_TYPE_DICT, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (!HG_IS_QSTRING (qkey), FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	qmasked = hg_quark_get_hash(qkey);
	removed = _hg_dict_node_remove(dict->o.mem, dict->qroot,
				       &qmasked, &qval, &need_restore);
	if (!HG_ERROR_IS_SUCCESS0 ())
		goto finalize;
	if (removed) {
		hg_dict_node_t *root_node;

		qroot = dict->qroot;
		hg_return_val_if_lock_fail (root_node,
					    dict->o.mem,
					    dict->qroot,
					    FALSE);
		if (root_node->n_data == 0) {
			hg_quark_t *root_nodes;

			root_nodes = HG_MEM_LOCK (root_node->o.mem,
						  root_node->qnodes);
			if (root_nodes == NULL) {
				goto qfinalize;
			}
			dict->qroot = root_nodes[0];
			/* 'page' page could be freed here though, it might be
			 * referred from somewhere. so the actual destroying
			 * the page relies on GC.
			 */
			hg_mem_unlock_object(root_node->o.mem, root_node->qnodes);
		}
	  qfinalize:
		hg_mem_unlock_object(dict->o.mem, qroot);

		dict->length--;
	}
  finalize:

	return removed;
}

/**
 * hg_dict_lookup:
 * @dict:
 * @qkey:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_dict_lookup(hg_dict_t  *dict,
	       hg_quark_t  qkey)
{
	hg_dict_node_t *qnode_node = NULL;
	hg_quark_t retval = Qnil, qnode, qmasked;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;

	hg_return_val_if_fail (dict != NULL, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (dict->o.type == HG_TYPE_DICT, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (!HG_IS_QSTRING (qkey), Qnil, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	qmasked = hg_quark_get_hash(qkey);
	qnode = dict->qroot;

	if (qnode == Qnil)
		return Qnil;

	HG_DICT_NODE_LOCK (dict->o.mem, qnode, qnode, "");
	while (qnode_node != NULL) {
		hg_quark_t q;
		hg_usize_t i;

		if (qnode_keys[qnode_node->n_data - 1] >= qmasked) {
			for (i = 0; i < qnode_node->n_data && qnode_keys[i] < qmasked; i++);
			if (i < qnode_node->n_data && qnode_keys[i] == qmasked) {
				retval = qnode_vals[i];
				break;
			}
		} else {
			i = qnode_node->n_data;
		}
		q = qnode_nodes[i];
		HG_DICT_NODE_UNLOCK_NO_LABEL (dict->o.mem, qnode, qnode);
		qnode = q;
		if (qnode == Qnil) {
			/* escape the loop here to avoid a warning at follow. */
			return Qnil;
		}
		HG_DICT_NODE_LOCK (dict->o.mem, qnode, qnode, "");
	}

	HG_DICT_NODE_UNLOCK (dict->o.mem, qnode, qnode);

	return retval;
}

/**
 * hg_dict_length:
 * @dict:
 *
 * FIXME
 *
 * Returns:
 */
hg_usize_t
hg_dict_length(hg_dict_t *dict)
{
	hg_return_val_if_fail (dict != NULL, 0, HG_e_typecheck);
	hg_return_val_if_fail (dict->o.type == HG_TYPE_DICT, 0, HG_e_typecheck);

	return dict->length;
}

/**
 * hg_dict_maxlength:
 * @dict:
 *
 * FIXME
 *
 * Returns:
 */
hg_usize_t
hg_dict_maxlength(hg_dict_t *dict)
{
	hg_return_val_if_fail (dict != NULL, 0, HG_e_typecheck);
	hg_return_val_if_fail (dict->o.type == HG_TYPE_DICT, 0, HG_e_typecheck);

	return dict->allocated_size;
}

/**
 * hg_dict_foreach:
 * @dict:
 * @func:
 * @data:
 *
 * FIXME
 */
void
hg_dict_foreach(hg_dict_t               *dict,
		hg_dict_traverse_func_t  func,
		hg_pointer_t             data)
{
	hg_return_if_fail (dict != NULL, HG_e_typecheck);
	hg_return_if_fail (dict->o.type == HG_TYPE_DICT, HG_e_typecheck);
	hg_return_if_fail (func != NULL, HG_e_VMerror);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	_hg_dict_node_foreach(dict->o.mem, dict->qroot, func, data);
}

/**
 * hg_dict_first_item:
 * @dict:
 * @qkey:
 * @qval:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_dict_first_item(hg_dict_t  *dict,
		   hg_quark_t *qkey,
		   hg_quark_t *qval)
{
	hg_dict_item_t x;

	hg_return_val_if_fail (dict != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (dict->o.type == HG_TYPE_DICT, FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	_hg_dict_node_foreach(dict->o.mem, dict->qroot, _hg_dict_traverse_first_item, &x);
	if (HG_ERROR_IS_SUCCESS0 ()) {
		if (qkey)
			*qkey = x.qkey;
		if (qval)
			*qval = x.qval;
	}

	return TRUE;
}
