/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.h
 * Copyright (C) 2005-2007 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGSTACK_H__
#define __HIEROGLYPH_HGSTACK_H__

#include <hieroglyph/hgtypes.h>


G_BEGIN_DECLS

hg_stack_t  *hg_stack_new   (hg_vm_t     *vm,
			     gsize        depth) G_GNUC_WARN_UNUSED_RESULT;
void         hg_stack_free  (hg_vm_t     *vm,
                             hg_stack_t  *stack);
gboolean     hg_stack_push  (hg_stack_t  *stack,
                             hg_object_t *object);
hg_object_t *hg_stack_pop   (hg_stack_t  *stack);
void         hg_stack_clear (hg_stack_t  *stack);
gsize        hg_stack_length(hg_stack_t  *stack);
hg_object_t *hg_stack_index (hg_stack_t  *stack,
                             gsize        index);
gboolean     hg_stack_roll  (hg_stack_t  *stack,
                             gsize        n_block,
                             gssize       n_times);
void         hg_stack_dump  (hg_vm_t     *vm,
                             hg_stack_t  *stack);

G_END_DECLS

#endif /* __HIEROGLYPH_HGSTACK_H__ */
