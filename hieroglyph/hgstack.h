/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.h
 * Copyright (C) 2005-2006 Akira TAGOH
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
#ifndef __HG_STACK_H__
#define __HG_STACK_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


HgStack       *hg_stack_new  (HgMemPool    *pool,
			      guint         max_depth);
guint          hg_stack_depth(HgStack      *stack);
gboolean       hg_stack_push (HgStack      *stack,
			      HgValueNode  *node);
HgValueNode   *hg_stack_pop  (HgStack      *stack);
void           hg_stack_clear(HgStack      *stack);
HgValueNode   *hg_stack_index(HgStack      *stack,
			      guint         index_from_top);
void           hg_stack_roll (HgStack      *stack,
			      guint         n_block,
			      gint32        n_times);
void           hg_stack_dump (HgStack      *stack,
			      HgFileObject *file);

/* internal use */
void     _hg_stack_use_stack_validator(HgStack     *stack,
				       gboolean     flag);
gboolean _hg_stack_push               (HgStack     *stack,
				       HgValueNode *node);

G_END_DECLS

#endif /* __HG_STACK_H__ */
