/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-ffit.h
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#ifndef __HG_ALLOCATOR_FFIT_H__
#define __HG_ALLOCATOR_FFIT_H__

#ifdef __HG_ALLOCATOR_H__
#error Cannot use another allocator at the same time with this
#endif

#define __HG_ALLOCATOR_H__

#include <hieroglyph/hgtypes.h>

#define hg_allocator_new	hg_allocator_ffit_new
#define hg_allocator_destroy	hg_allocator_ffit_destroy

G_BEGIN_DECLS

HgAllocator *hg_allocator_ffit_new    (void);
void         hg_allocator_ffit_destroy(HgAllocator *allocator);

G_END_DECLS

#endif /* __HG_ALLOCATOR_FFIT_H__ */
