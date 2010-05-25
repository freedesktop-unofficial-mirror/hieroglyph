/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmacros.h
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
#ifndef __HIEROGLYPH_HGMACROS_H__
#define __HIEROGLYPH_HGMACROS_H__

#include <glib.h>

G_BEGIN_DECLS

#if defined(GNOME_ENABLE_DEBUG) || defined(DEBUG)
#define HG_DEBUG
#endif /* GNOME_ENABLE_DEBUG || DEBUG */

#define HGPOINTER_TO_QUARK(p)	((hg_quark_t)(gulong)(p))
#define HGQUARK_TO_POINTER(q)	((gpointer)(gulong)(q))

#define hg_mem_aligned_to(x,y)			\
	(((x) + (y) - 1) & ~((y) - 1))
#define hg_mem_aligned_size(x)			\
	hg_mem_aligned_to(x, ALIGNOF_VOID_P)


G_END_DECLS

#endif /* __HIEROGLYPH_HGMACROS_H__ */
