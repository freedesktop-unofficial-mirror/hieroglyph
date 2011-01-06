/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack-private.h
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
#ifndef __HIEROGLYPH_HGSTACK_PRIVATE_H__
#define __HIEROGLYPH_HGSTACK_PRIVATE_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

typedef struct _hg_slist_t	hg_slist_t;

struct _hg_slist_t {
	hg_quark_t  self;
	hg_quark_t  data;
	hg_slist_t *next;
};
struct _hg_stack_t {
	hg_object_t  o;
	hg_quark_t   self;
	hg_vm_t     *vm;
	hg_slist_t  *last_stack;
	gsize        max_depth;
	gsize        depth;
	gboolean     validate_depth;
};

G_END_DECLS

#endif /* __HIEROGLYPH_HGSTACK_PRIVATE_H__ */
