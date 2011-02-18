/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.h
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGDICT_H__
#define __HIEROGLYPH_HGDICT_H__

#include <hieroglyph/hgobject.h>

HG_BEGIN_DECLS

#define HG_DICT_INIT							\
	HG_STMT_START {							\
		hg_object_register(HG_TYPE_DICT,			\
				   hg_object_dict_get_vtable());	\
		hg_object_register(HG_TYPE_DICT_NODE,			\
				   hg_object_dict_node_get_vtable());	\
	} HG_STMT_END

#define HG_IS_QDICT(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_DICT)
#define HG_IS_QDICT_NODE(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_DICT_NODE)

typedef struct _hg_dict_t		hg_dict_t;
typedef struct _hg_dict_node_t		hg_dict_node_t;
typedef hg_cb_BOOL__QUARK_QUARK_t	hg_dict_traverse_func_t;

struct _hg_dict_t {
	hg_object_t o;
	hg_quark_t  qroot;
	hg_usize_t  length;
	hg_usize_t  allocated_size;
	hg_bool_t   raise_dictfull:1;
};


hg_object_vtable_t *hg_object_dict_get_vtable     (void) G_GNUC_CONST;
hg_object_vtable_t *hg_object_dict_node_get_vtable(void) G_GNUC_CONST;
hg_quark_t          hg_dict_new                   (hg_mem_t                *mem,
                                                   hg_usize_t               size,
                                                   hg_bool_t                raise_dictfull,
                                                   hg_pointer_t            *ret);
hg_bool_t           hg_dict_add                   (hg_dict_t               *dict,
                                                   hg_quark_t               qkey,
                                                   hg_quark_t               qval,
                                                   hg_bool_t                force);
hg_bool_t           hg_dict_remove                (hg_dict_t               *dict,
                                                   hg_quark_t               qkey);
hg_quark_t          hg_dict_lookup                (hg_dict_t               *dict,
                                                   hg_quark_t               qkey);
hg_usize_t          hg_dict_length                (hg_dict_t               *dict);
hg_usize_t          hg_dict_maxlength             (hg_dict_t               *dict);
void                hg_dict_foreach               (hg_dict_t               *dict,
                                                   hg_dict_traverse_func_t  func,
                                                   hg_pointer_t             data);
hg_bool_t           hg_dict_first_item            (hg_dict_t               *dict,
                                                   hg_quark_t              *qkey,
                                                   hg_quark_t              *qval);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGDICT_H__ */
