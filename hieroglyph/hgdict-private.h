/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict-private.h
 * Copyright (C) 2006-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGDICT_PRIVATE_H__
#define __HIEROGLYPH_HGDICT_PRIVATE_H__

#include "hgerror.h"
#include "hgquark.h"
#include "hgmem.h"
#include "hgobject.h"

G_BEGIN_DECLS

#define HG_DICT_MAX_SIZE	65535
#define HG_DICT_NODE_LOCK(_mem_,_node_quark_,_nodeprefix_,_comment_,_error_) \
	G_STMT_START {							\
		_nodeprefix_ ## _keys = NULL;				\
		_nodeprefix_ ## _vals = NULL;				\
		_nodeprefix_ ## _nodes = NULL;				\
		_nodeprefix_ ## _node = hg_mem_lock_object((_mem_), (_node_quark_)); \
		if (_nodeprefix_ ## _node == NULL) {			\
			g_set_error((_error_), HG_ERROR, HG_VM_e_VMerror, \
				    "%s: Invalid quark to obtain the " _comment_ "node object: mem: %p, quark: %" G_GSIZE_FORMAT, \
				    __PRETTY_FUNCTION__, (_mem_), (_node_quark_)); \
			goto _nodeprefix_ ## _finalize;			\
		}							\
		_nodeprefix_ ## _keys = hg_mem_lock_object(_nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qkey); \
		if (_nodeprefix_ ## _keys == NULL) {			\
			g_set_error((_error_), HG_ERROR, HG_VM_e_VMerror, \
				    "%s: Invalid quark to obtain the " _comment_ "keys container object: mem: %p, quark: %" G_GSIZE_FORMAT, \
				    __PRETTY_FUNCTION__, _nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qkey); \
			goto _nodeprefix_ ## _finalize;			\
		}							\
		_nodeprefix_ ## _vals = hg_mem_lock_object(_nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qval); \
		if (_nodeprefix_ ## _vals == NULL) {			\
			g_set_error((_error_), HG_ERROR, HG_VM_e_VMerror, \
				    "%s: Invalid quark to obtain the " _comment_ "vals container object: mem: %p, quark: %" G_GSIZE_FORMAT, \
				    __PRETTY_FUNCTION__, _nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qval); \
			goto _nodeprefix_ ## _finalize;			\
		}							\
		_nodeprefix_ ## _nodes = hg_mem_lock_object(_nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qnodes); \
		if (_nodeprefix_ ## _nodes == NULL) {			\
			g_set_error((_error_), HG_ERROR, HG_VM_e_VMerror, \
				    "%s: Invalid quark to obtain the " _comment_ "nodes container object: mem: %p, quark: %" G_GSIZE_FORMAT, \
				    __PRETTY_FUNCTION__, _nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qnodes); \
			goto _nodeprefix_ ## _finalize;			\
		}							\
	} G_STMT_END

#define HG_DICT_NODE_UNLOCK_NO_LABEL(_mem_,_node_quark_,_nodeprefix_)	\
	G_STMT_START {							\
		if (_nodeprefix_ ## _keys)				\
			hg_mem_unlock_object(_nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qkey);	\
		if (_nodeprefix_ ## _vals)				\
			hg_mem_unlock_object(_nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qval);	\
		if (_nodeprefix_ ## _nodes)				\
			hg_mem_unlock_object(_nodeprefix_ ## _node->o.mem, _nodeprefix_ ## _node->qnodes); \
		if (_nodeprefix_ ## _node)				\
			hg_mem_unlock_object((_mem_), (_node_quark_));	\
	} G_STMT_END
#define HG_DICT_NODE_UNLOCK(_mem_,_node_quark_,_nodeprefix_)		\
	G_STMT_START {							\
		_nodeprefix_ ## _finalize:				\
		HG_DICT_NODE_UNLOCK_NO_LABEL (_mem_,_node_quark_,_nodeprefix_); \
	} G_STMT_END


struct _hg_dict_node_t {
	hg_object_t o;
	hg_quark_t  qkey;
	hg_quark_t  qval;
	hg_quark_t  qnodes;
	gsize       n_data;
};

void _hg_dict_node_set_size(gsize size);

G_END_DECLS

#endif /* __HIEROGLYPH_HGDICT_PRIVATE_H__ */
