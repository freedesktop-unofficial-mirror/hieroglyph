/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator.h
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
#ifndef __HIEROGLYPH_HGOPERATOR_H__
#define __HIEROGLYPH_HGOPERATOR_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

#define HG_QOPER(_v_)				\
	(hg_quark_new(HG_TYPE_OPER, (_v_)))
#define HG_IS_QOPER(_v_)				\
	(hg_quark_get_type((_v_)) == HG_TYPE_OPER)

typedef gboolean (* hg_operator_func_t) (hg_vm_t  *vm,
					 GError  **error);


gboolean     hg_operator_init    (void);
void         hg_operator_tini    (void);
gboolean     hg_operator_invoke  (hg_quark_t          qoper,
				  hg_vm_t            *vm,
				  GError            **error);
const gchar *hg_operator_get_name(hg_quark_t          qoper);
gboolean     hg_operator_register(hg_dict_t          *dict,
				  hg_name_t          *name,
				  hg_vm_langlevel_t   lang_level);

G_END_DECLS

#endif /* __HIEROGLYPH_HGOPERATOR_H__ */
