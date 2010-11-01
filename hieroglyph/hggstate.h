/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggsate.h
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
#ifndef __HIEROGLYPH_HGGSTATE_H__
#define __HIEROGLYPH_HGGSTATE_H__

#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgpath.h>

G_BEGIN_DECLS

#define HG_GSTATE_INIT						\
	(hg_object_register(HG_TYPE_GSTATE,			\
			    hg_object_gstate_get_vtable()))
#define HG_IS_GSTATE(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_GSTATE)

typedef struct _hg_gstate_t	hg_gstate_t;

struct _hg_gstate_t {
	hg_object_t o;
	hg_quark_t  qpath;
};


hg_object_vtable_t *hg_object_gstate_get_vtable(void);
hg_quark_t          hg_gstate_new              (hg_mem_t    *mem,
                                                gpointer    *ret);
void                hg_gstate_set_path         (hg_gstate_t *gstate,
                                                hg_quark_t   qpath);
hg_quark_t          hg_gstate_get_path         (hg_gstate_t *gstate);


G_END_DECLS

#endif /* __HIEROGLYPH_HGGSTATE_H__ */
