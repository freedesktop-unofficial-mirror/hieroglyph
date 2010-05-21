/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgtypes.h
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
#ifndef __HIEROGLYPH_HGTYPES_H__
#define __HIEROGLYPH_HGTYPES_H__

#include <hieroglyph/hgmacros.h>

G_BEGIN_DECLS

#define Qnil	-1

/* hgmem.h */
typedef struct _hg_mem_t	hg_mem_t;
typedef struct _hg_mem_vtable_t	hg_mem_vtable_t;
/* hgutils.h */
typedef guint32			hg_quark_t;


/* hgmem.h */
struct _hg_mem_vtable_t {
	gboolean   (* initialize)    (hg_mem_t   *mem);
	void       (* finalize)      (hg_mem_t   *mem);
	gboolean   (* resize_heap)   (hg_mem_t   *mem);
	hg_quark_t (* alloc)         (hg_mem_t   *mem,
				      gsize       size);
	void       (* free)          (hg_mem_t   *mem,
				      hg_quark_t  data);
	gpointer   (* lock_object)   (hg_mem_t   *mem,
				      hg_quark_t  data);
	void       (* unlock_object) (hg_mem_t   *mem,
				      hg_quark_t  data);
};


G_END_DECLS

#endif /* __HIEROGLYPH_HGTYPES_H__ */
