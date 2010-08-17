/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator.h
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
#ifndef __HIEROGLYPH_HGALLOCATOR_H__
#define __HIEROGLYPH_HGALLOCATOR_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

typedef struct _hg_allocator_bitmap_t		hg_allocator_bitmap_t;
typedef struct _hg_allocator_block_t		hg_allocator_block_t;
typedef struct _hg_allocator_private_t		hg_allocator_private_t;
typedef struct _hg_allocator_snapshot_private_t	hg_allocator_snapshot_private_t;


hg_mem_vtable_t *hg_allocator_get_vtable(void) G_GNUC_CONST;

G_END_DECLS

#endif /* __HIEROGLYPH_HGALLOCATOR_H__ */
