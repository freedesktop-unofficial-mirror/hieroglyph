/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * lbstack.h
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
#ifndef __LIBRETTO_STACK_H__
#define __LIBRETTO_STACK_H__

#include <hieroglyph/hgtypes.h>
#include <libretto/lbtypes.h>

G_BEGIN_DECLS


LibrettoStack *libretto_stack_new  (HgMemPool     *pool,
				    guint          max_depth);
guint          libretto_stack_depth(LibrettoStack *stack);
gboolean       libretto_stack_push (LibrettoStack *stack,
				    HgValueNode   *node);
HgValueNode   *libretto_stack_pop  (LibrettoStack *stack);
void           libretto_stack_clear(LibrettoStack *stack);
HgValueNode   *libretto_stack_index(LibrettoStack *stack,
				    guint          index_from_top);
void           libretto_stack_roll (LibrettoStack *stack,
				    guint          n_block,
				    gint32         n_times);

/* internal use */
void     _libretto_stack_use_stack_validator(LibrettoStack *stack,
					     gboolean       flag);
gboolean _libretto_stack_push               (LibrettoStack *stack,
					     HgValueNode   *node);

G_END_DECLS

#endif /* __LIBRETTO_STACK_H__ */
