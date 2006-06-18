/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmacros.h
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
#ifndef __HG_MACROS_H__
#define __HG_MACROS_H__

#include <glib.h>

G_BEGIN_DECLS

#define HG_MEM_HEADER	0x48474d4f
#define HG_OBJECT_ID	0x48474f4f
#define HG_CHECK_MAGIC_CODE(_obj, _magic)		\
	((_obj)->magic == (_magic))
#define HG_SET_MAGIC_CODE(_obj, _magic)			\
	((_obj)->magic = (_magic))

#define HG_STACK_INIT					\
	gpointer __hg_stack_mark;			\
							\
	hg_mem_init_stack_start(&__hg_stack_mark)
#define HG_SET_STACK_END			\
	gpointer __hg_stack_mark;		\
						\
	hg_mem_set_stack_end(&__hg_stack_mark)
#define HG_SET_STACK_END_AGAIN			\
	hg_mem_set_stack_end(&__hg_stack_mark)
#define HG_MEM_INIT				\
	HG_STACK_INIT;				\
	hg_mem_init()

#define HG_MEM_ALIGNMENT	4

G_END_DECLS

#endif /* __HG_MACROS_H__ */
