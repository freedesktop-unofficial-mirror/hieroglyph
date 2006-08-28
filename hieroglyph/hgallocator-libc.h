/* 
 * hgallocator-libc.h
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
#ifndef __HG_ALLOCATOR_LIBC_H__
#define __HG_ALLOCATOR_LIBC_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

HgAllocatorVTable *hg_allocator_libc_get_vtable(void) G_GNUC_CONST;

G_END_DECLS

#endif /* __HG_ALLOCATOR_LIBC_H__ */
