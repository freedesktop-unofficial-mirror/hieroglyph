/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem-private.h
 * Copyright (C) 2010-2011 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGMEM_PRIVATE_H__
#define __HIEROGLYPH_HGMEM_PRIVATE_H__

#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

struct _hg_mem_t {
	hg_mem_vtable_t     *allocator;
	hg_mem_type_t        type;
	hg_int_t             id;
	hg_allocator_data_t *data;
	hg_gc_func_t         gc_func;
	hg_pointer_t         gc_data;
	GHashTable          *reference_table;
	hg_bool_t            enable_gc:1;
};

HG_END_DECLS

#endif /* __HIEROGLYPH_HGMEM_PRIVATE_H__ */
