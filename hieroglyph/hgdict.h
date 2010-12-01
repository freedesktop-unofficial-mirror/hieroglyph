/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.h
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
#ifndef __HIEROGLYPH_HGDICT_H__
#define __HIEROGLYPH_HGDICT_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

#define HG_DICT_INIT							\
	G_STMT_START {							\
		hg_object_register(HG_TYPE_DICT,			\
				   hg_object_dict_get_vtable());	\
		hg_object_register(HG_TYPE_DICT_NODE,			\
				   hg_object_dict_node_get_vtable());	\
	} G_STMT_END

#define HG_IS_QDICT(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_DICT)
#define HG_IS_QDICT_NODE(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_DICT_NODE)

typedef struct _hg_bs_dict_t	hg_bs_dict_t;

struct _hg_bs_dict_t {
};
struct _hg_dict_t {
	hg_object_t o;
	hg_quark_t  qroot;
	gsize       length;
	gsize       allocated_size;
	gboolean    raise_dictfull:1;
};


hg_object_vtable_t *hg_object_dict_get_vtable     (void) G_GNUC_CONST;
hg_object_vtable_t *hg_object_dict_node_get_vtable(void) G_GNUC_CONST;
hg_quark_t          hg_dict_new                   (hg_mem_t                 *mem,
						   gsize                     size,
						   gboolean                  raise_dictfull,
						   gpointer                 *ret);
gboolean            hg_dict_add                   (hg_dict_t                *dict,
						   hg_quark_t                qkey,
						   hg_quark_t                qval,
						   gboolean                  force,
						   GError                  **error);
gboolean            hg_dict_remove                (hg_dict_t                *dict,
						   hg_quark_t                qkey,
						   GError                  **error);
hg_quark_t          hg_dict_lookup                (hg_dict_t                *dict,
						   hg_quark_t                qkey,
						   GError                  **error);
gsize               hg_dict_length                (hg_dict_t                *dict);
gsize               hg_dict_maxlength             (hg_dict_t                *dict);
void                hg_dict_foreach               (hg_dict_t                *dict,
						   hg_dict_traverse_func_t   func,
						   gpointer                  data,
						   GError                  **error);
gboolean            hg_dict_first_item            (hg_dict_t                *dict,
						   hg_quark_t               *qkey,
						   hg_quark_t               *qval,
						   GError                  **error);


G_END_DECLS

#endif /* __HIEROGLYPH_HGDICT_H__ */
