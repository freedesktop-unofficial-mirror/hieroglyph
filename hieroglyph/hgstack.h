/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.h
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
#ifndef __HIEROGLYPH_HGSTACK_H__
#define __HIEROGLYPH_HGSTACK_H__

#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgarray.h>

G_BEGIN_DECLS

#define HG_STACK_INIT						\
	(hg_object_register(HG_TYPE_STACK,			\
			    hg_object_stack_get_vtable()))

typedef struct _hg_stack_t	hg_stack_t;



hg_object_vtable_t *hg_object_stack_get_vtable(void) G_GNUC_CONST;
hg_stack_t         *hg_stack_new              (hg_mem_t                  *mem,
                                               gsize                      max_depth,
					       hg_vm_t                   *vm);
void                hg_stack_free             (hg_stack_t                *stack);
void                hg_stack_set_validation   (hg_stack_t                *stack,
                                               gboolean                   flag);
gsize               hg_stack_depth            (hg_stack_t                *stack);
gboolean            hg_stack_set_max_depth    (hg_stack_t                *stack,
					       gsize                      depth);
gboolean            hg_stack_push             (hg_stack_t                *stack,
                                               hg_quark_t                 quark);
hg_quark_t          hg_stack_pop              (hg_stack_t                *stack,
                                               GError                   **error);
void                hg_stack_drop             (hg_stack_t                *stack,
					       GError                   **error);
void                hg_stack_clear            (hg_stack_t                *stack);
hg_quark_t          hg_stack_index            (hg_stack_t                *stack,
                                               gsize                      index,
                                               GError                   **error);
void                hg_stack_roll             (hg_stack_t                *stack,
                                               gsize                      n_blocks,
                                               gssize                     n_times,
                                               GError                   **error);
void                hg_stack_foreach          (hg_stack_t                *stack,
                                               hg_array_traverse_func_t   func,
                                               gpointer                   data,
					       gboolean                   is_forwarded,
                                               GError                   **error);

G_END_DECLS

#endif /* __HIEROGLYPH_HGSTACK_H__ */
